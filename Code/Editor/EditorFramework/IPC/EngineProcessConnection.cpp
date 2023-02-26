#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Dialogs/RemoteConnectionDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GuiFoundation/UIServices/QtWaitForOperationDlg.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>


EZ_IMPLEMENT_SINGLETON(ezEditorEngineProcessConnection);

ezEvent<const ezEditorEngineProcessConnection::Event&> ezEditorEngineProcessConnection::s_Events;

ezEditorEngineProcessConnection::ezEditorEngineProcessConnection()
  : m_SingletonRegistrar(this)
{
  m_bProcessShouldBeRunning = false;
  m_bProcessCrashed = false;
  m_bClientIsConfigured = false;

  m_IPC.m_Events.AddEventHandler(ezMakeDelegate(&ezEditorEngineProcessConnection::HandleIPCEvent, this));
}

ezEditorEngineProcessConnection::~ezEditorEngineProcessConnection()
{
  m_IPC.m_Events.RemoveEventHandler(ezMakeDelegate(&ezEditorEngineProcessConnection::HandleIPCEvent, this));
}

void ezEditorEngineProcessConnection::SendDocumentOpenMessage(const ezAssetDocument* pDocument, bool bOpen)
{
  EZ_PROFILE_SCOPE("SendDocumentOpenMessage");

  if (!pDocument)
    return;

  // it is important to have up-to-date lookup tables in the engine process, because document contexts might try to
  // load resources, and if the file redirection does not happen correctly, derived resource types may not be created as they should
  ezAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();

  ezDocumentOpenMsgToEngine m;
  m.m_DocumentGuid = pDocument->GetGuid();
  m.m_bDocumentOpen = bOpen;
  m.m_sDocumentType = pDocument->GetDocumentTypeDescriptor()->m_sDocumentTypeName;
  m.m_DocumentMetaData = pDocument->GetCreateEngineMetaData();

  SendMessage(&m);
}

void ezEditorEngineProcessConnection::HandleIPCEvent(const ezProcessCommunicationChannel::Event& e)
{
  if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezSyncWithProcessMsgToEditor>())
  {
    const ezSyncWithProcessMsgToEditor* msg = static_cast<const ezSyncWithProcessMsgToEditor*>(e.m_pMessage);
    m_uiRedrawCountReceived = msg->m_uiRedrawCount;
  }
  if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineDocumentMsg>())
  {
    const ezEditorEngineDocumentMsg* pMsg = static_cast<const ezEditorEngineDocumentMsg*>(e.m_pMessage);

    ezAssetDocument* pDocument = m_DocumentByGuid[pMsg->m_DocumentGuid];

    if (pDocument)
    {
      pDocument->HandleEngineMessage(pMsg);
    }
  }
  else if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineMsg>())
  {
    Event ee;
    ee.m_pMsg = static_cast<const ezEditorEngineMsg*>(e.m_pMessage);
    ee.m_Type = Event::Type::ProcessMessage;

    s_Events.Broadcast(ee);
  }
}

void ezEditorEngineProcessConnection::UIServicesTickEventHandler(const ezQtUiServices::TickEvent& e)
{
  if (e.m_Type == ezQtUiServices::TickEvent::Type::EndFrame)
  {
    if (!IsProcessCrashed())
    {
      ezSyncWithProcessMsgToEngine sm;
      sm.m_uiRedrawCount = m_uiRedrawCountSent + 1;
      SendMessage(&sm);

      if (m_uiRedrawCountSent > m_uiRedrawCountReceived)
      {
        WaitForMessage(ezGetStaticRTTI<ezSyncWithProcessMsgToEditor>(), ezTime::Seconds(2.0)).IgnoreResult();
      }

      ++m_uiRedrawCountSent;
    }
  }
}

ezEditorEngineConnection* ezEditorEngineProcessConnection::CreateEngineConnection(ezAssetDocument* pDocument)
{
  ezEditorEngineConnection* pConnection = new ezEditorEngineConnection(pDocument);

  m_DocumentByGuid[pDocument->GetGuid()] = pDocument;

  SendDocumentOpenMessage(pDocument, true);

  return pConnection;
}

void ezEditorEngineProcessConnection::DestroyEngineConnection(ezAssetDocument* pDocument)
{
  SendDocumentOpenMessage(pDocument, false);

  m_DocumentByGuid.Remove(pDocument->GetGuid());

  delete pDocument->GetEditorEngineConnection();
}

void ezEditorEngineProcessConnection::Initialize(const ezRTTI* pFirstAllowedMessageType)
{
  EZ_PROFILE_SCOPE("Initialize");
  if (m_IPC.IsClientAlive())
    return;

  ezLog::Dev("Starting Client Engine Process");

  EZ_ASSERT_DEBUG(m_TickEventSubscriptionID == 0, "A previous subscription is still in place. ShutdownProcess not called?");
  m_TickEventSubscriptionID = ezQtUiServices::s_TickEvent.AddEventHandler(ezMakeDelegate(&ezEditorEngineProcessConnection::UIServicesTickEventHandler, this));

  m_bProcessShouldBeRunning = true;
  m_bProcessCrashed = false;
  m_bClientIsConfigured = false;

  QStringList args;
  if (m_bProcessShouldWaitForDebugger)
  {
    args << "-debug";
  }

  if (!m_sRenderer.IsEmpty())
  {
    args << "-renderer";
    args << m_sRenderer.GetData();
  }

  {
    ezStringBuilder sWndCfgPath = ezApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    sWndCfgPath.AppendPath("Window.ddl");

    args << "-wnd";
    args << sWndCfgPath.GetData();
  }

  // set up the EditorEngineProcess telemetry server on a different port
  {
    args << "-TelemetryPort";
    args << ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-TelemetryPort", 0, "1050");
  }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  const char* EditorEngineProcessExecutableName = "EditorEngineProcess.exe";
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
  const char* EditorEngineProcessExecutableName = "EditorEngineProcess";
#else
#  error Platform not supported
#endif


  if (m_IPC.StartClientProcess(EditorEngineProcessExecutableName, args, false, pFirstAllowedMessageType).Failed())
  {
    m_bProcessCrashed = true;
  }
  else
  {
    Event e;
    e.m_Type = Event::Type::ProcessStarted;
    s_Events.Broadcast(e);
  }
}

void ezEditorEngineProcessConnection::ActivateRemoteProcess(const ezAssetDocument* pDocument, ezUInt32 uiViewID)
{
  // make sure process is started
  if (!ConnectToRemoteProcess())
    return;

  // resend entire document
  {
    // open document message
    {
      ezDocumentOpenMsgToEngine msg;
      msg.m_DocumentGuid = pDocument->GetGuid();
      msg.m_bDocumentOpen = true;
      msg.m_sDocumentType = pDocument->GetDocumentTypeDescriptor()->m_sDocumentTypeName;
      m_pRemoteProcess->SendMessage(&msg);
    }

    if (pDocument->GetDynamicRTTI()->IsDerivedFrom<ezAssetDocument>())
    {
      ezAssetDocument* pAssetDoc = (ezAssetDocument*)pDocument;
      ezDocumentOpenResponseMsgToEditor response;
      response.m_DocumentGuid = pDocument->GetGuid();
      pAssetDoc->HandleEngineMessage(&response);
    }
  }

  // send activation message
  {
    ezActivateRemoteViewMsgToEngine msg;
    msg.m_DocumentGuid = pDocument->GetGuid();
    msg.m_uiViewID = uiViewID;
    m_pRemoteProcess->SendMessage(&msg);
  }
}

bool ezEditorEngineProcessConnection::ConnectToRemoteProcess()
{
  if (m_pRemoteProcess != nullptr)
  {
    if (m_pRemoteProcess->IsConnected())
      return true;

    ShutdownRemoteProcess();
  }

  ezQtRemoteConnectionDlg dlg(QApplication::activeWindow());

  if (dlg.exec() == QDialog::Rejected)
    return false;

  m_pRemoteProcess = EZ_DEFAULT_NEW(ezEditorProcessRemoteCommunicationChannel);
  m_pRemoteProcess->ConnectToServer(dlg.GetResultingAddress().toUtf8().data()).IgnoreResult();

  ezQtWaitForOperationDlg waitDialog(QApplication::activeWindow());
  waitDialog.m_OnIdle = [this]() -> bool
  {
    if (m_pRemoteProcess->IsConnected())
      return false;

    m_pRemoteProcess->TryConnect();
    return true;
  };

  const int iRet = waitDialog.exec();

  if (iRet == QDialog::Accepted)
  {
    // Send project setup.
    ezSetupProjectMsgToEngine msg;
    msg.m_sProjectDir = ezToolsProject::GetSingleton()->GetProjectDirectory();
    msg.m_FileSystemConfig = m_FileSystemConfig;
    msg.m_PluginConfig = m_PluginConfig;
    msg.m_sFileserveAddress = dlg.GetResultingFsAddress().toUtf8().data();
    msg.m_sAssetProfile = ezAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName();

    m_pRemoteProcess->SendMessage(&msg);
  }

  return iRet == QDialog::Accepted;
}


void ezEditorEngineProcessConnection::ShutdownRemoteProcess()
{
  if (m_pRemoteProcess != nullptr)
  {
    ezLog::Info("Shutting down Remote Engine Process");
    m_pRemoteProcess->CloseConnection();

    m_pRemoteProcess = nullptr;
  }
}

void ezEditorEngineProcessConnection::ShutdownProcess()
{
  if (!m_bProcessShouldBeRunning)
    return;

  ShutdownRemoteProcess();

  ezLog::Info("Shutting down Engine Process");

  if (m_TickEventSubscriptionID != 0)
    ezQtUiServices::s_TickEvent.RemoveEventHandler(m_TickEventSubscriptionID);

  m_bClientIsConfigured = false;
  m_bProcessShouldBeRunning = false;
  m_IPC.CloseConnection();

  Event e;
  e.m_Type = Event::Type::ProcessShutdown;
  s_Events.Broadcast(e);
}

void ezEditorEngineProcessConnection::SendMessage(ezProcessMessage* pMessage)
{
  m_IPC.SendMessage(pMessage);

  if (m_pRemoteProcess)
  {
    m_pRemoteProcess->SendMessage(pMessage);
  }
}

ezResult ezEditorEngineProcessConnection::WaitForMessage(const ezRTTI* pMessageType, ezTime timeout, ezProcessCommunicationChannel::WaitForMessageCallback* pCallback)
{
  EZ_PROFILE_SCOPE(pMessageType->GetTypeName());
  return m_IPC.WaitForMessage(pMessageType, timeout, pCallback);
}

ezResult ezEditorEngineProcessConnection::WaitForDocumentMessage(const ezUuid& assetGuid, const ezRTTI* pMessageType, ezTime timeout, ezProcessCommunicationChannel::WaitForMessageCallback* pCallback /*= nullptr*/)
{
  if (!m_bProcessShouldBeRunning)
  {
    return EZ_FAILURE; // if the process is not running, we can't wait for a message
  }
  EZ_ASSERT_DEBUG(pMessageType->IsDerivedFrom(ezGetStaticRTTI<ezEditorEngineDocumentMsg>()), "The type of the message to wait for must be a document message.");
  struct WaitData
  {
    ezUuid m_AssetGuid;
    ezProcessCommunicationChannel::WaitForMessageCallback* m_pCallback;
  };

  WaitData data;
  data.m_AssetGuid = assetGuid;
  data.m_pCallback = pCallback;

  ezProcessCommunicationChannel::WaitForMessageCallback callback = [&data](ezProcessMessage* pMsg) -> bool
  {
    ezEditorEngineDocumentMsg* pMsg2 = ezDynamicCast<ezEditorEngineDocumentMsg*>(pMsg);
    if (pMsg2 && data.m_AssetGuid == pMsg2->m_DocumentGuid)
    {
      if (data.m_pCallback && data.m_pCallback->IsValid() && !(*data.m_pCallback)(pMsg))
      {
        return false;
      }
      return true;
    }
    return false;
  };

  return m_IPC.WaitForMessage(pMessageType, timeout, &callback);
}

ezResult ezEditorEngineProcessConnection::RestartProcess()
{
  EZ_PROFILE_SCOPE("RestartProcess");
  EZ_LOG_BLOCK("Restarting Engine Process");

  ezQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Reloading Engine Process...", ezTime::Seconds(5));

  ShutdownProcess();

  Initialize(ezGetStaticRTTI<ezSetupProjectMsgToEngine>());

  if (m_bProcessCrashed)
  {
    ezLog::Error("Engine process crashed during startup.");
    ShutdownProcess();
    return EZ_FAILURE;
  }

  ezLog::Dev("Waiting for IPC connection");

  if (m_IPC.WaitForConnection(ezTime()).Failed())
  {
    ezLog::Error("Engine process did not connect. Engine process output:\n{}", m_IPC.GetStdoutContents());
    ShutdownProcess();
    return EZ_FAILURE;
  }

  {
    // Send project setup.
    ezSetupProjectMsgToEngine msg;
    msg.m_sProjectDir = ezToolsProject::GetSingleton()->GetProjectDirectory();
    msg.m_FileSystemConfig = m_FileSystemConfig;
    msg.m_PluginConfig = m_PluginConfig;
    msg.m_sAssetProfile = ezAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName();

    SendMessage(&msg);
  }

  ezLog::Dev("Waiting for Engine Process response");

  if (WaitForMessage(ezGetStaticRTTI<ezProjectReadyMsgToEditor>(), ezTime()).Failed())
  {
    ezLog::Error("Failed to restart the engine process. Engine Process Output:\n", m_IPC.GetStdoutContents());
    ShutdownProcess();
    return EZ_FAILURE;
  }

  ezLog::Dev("Transmitting open documents to Engine Process");

  ezHybridArray<ezAssetDocument*, 6> docs;
  docs.Reserve(m_DocumentByGuid.GetCount());

  // Resend all open documents. Make sure to send main documents before child documents.
  for (auto it = m_DocumentByGuid.GetIterator(); it.IsValid(); ++it)
  {
    docs.PushBack(it.Value());
  }
  docs.Sort([](const ezAssetDocument* a, const ezAssetDocument* b)
    {
    if (a->IsMainDocument() != b->IsMainDocument())
      return a->IsMainDocument();
    return a < b; });

  for (ezAssetDocument* pDoc : docs)
  {
    SendDocumentOpenMessage(pDoc, true);
  }

  ezAssetCurator::GetSingleton()->InvalidateAssetsWithTransformState(ezAssetInfo::TransformState::TransformError);

  ezLog::Success("Engine Process is running");

  m_bClientIsConfigured = true;

  Event e;
  e.m_Type = Event::Type::ProcessRestarted;
  s_Events.Broadcast(e);

  return EZ_SUCCESS;
}

void ezEditorEngineProcessConnection::Update()
{
  if (!m_bProcessShouldBeRunning)
    return;

  if (!m_IPC.IsClientAlive())
  {
    ShutdownProcess();
    m_bProcessCrashed = true;

    Event e;
    e.m_Type = Event::Type::ProcessCrashed;
    s_Events.Broadcast(e);

    return;
  }

  m_IPC.ProcessMessages();

  if (m_pRemoteProcess)
  {
    m_pRemoteProcess->ProcessMessages();
  }
}

void ezEditorEngineConnection::SendMessage(ezEditorEngineDocumentMsg* pMessage)
{
  EZ_ASSERT_DEV(this != nullptr, "No connection between editor and engine was created. This typically happens when an asset document does "
                                 "not enable the engine-connection through the constructor of ezAssetDocument.");

  pMessage->m_DocumentGuid = m_pDocument->GetGuid();

  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(pMessage);
}

void ezEditorEngineConnection::SendHighlightObjectMessage(ezViewHighlightMsgToEngine* pMessage)
{
  // without this check there will be so many messages, that the editor comes to a crawl (< 10 FPS)
  // This happens because Qt sends hundreds of mouse-move events and since each 'SendMessageToEngine'
  // requires a round-trip to the engine process, doing this too often will be sloooow

  static ezUuid LastHighlightGuid;

  if (LastHighlightGuid == pMessage->m_HighlightObject)
    return;

  LastHighlightGuid = pMessage->m_HighlightObject;
  SendMessage(pMessage);
}
