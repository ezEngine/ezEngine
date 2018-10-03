#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Dialogs/RemoteConnectionDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/Communication/Implementation/IpcChannelEnet.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <GuiFoundation/UIServices/QtWaitForOperationDlg.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#include <QProcess>

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

void ezEditorEngineProcessConnection::SendDocumentOpenMessage(const ezDocument* pDocument, bool bOpen)
{
  EZ_PROFILE("SendDocumentOpenMessage");

  if (!pDocument)
    return;

  ezDocumentOpenMsgToEngine m;
  m.m_DocumentGuid = pDocument->GetGuid();
  m.m_bDocumentOpen = bOpen;
  m.m_sDocumentType = pDocument->GetDocumentTypeDescriptor()->m_sDocumentTypeName;

  SendMessage(&m);
}

void ezEditorEngineProcessConnection::HandleIPCEvent(const ezProcessCommunicationChannel::Event& e)
{
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
  EZ_PROFILE("Initialize");
  if (m_IPC.IsClientAlive())
    return;

  ezLog::Dev("Starting Client Engine Process");

  m_bProcessShouldBeRunning = true;
  m_bProcessCrashed = false;
  m_bClientIsConfigured = false;

  QStringList args;
  if (m_bProcessShouldWaitForDebugger)
  {
    args << "-debug";
  }

  {
    ezStringBuilder sWndCfgPath = ezApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    sWndCfgPath.AppendPath("Window.ddl");

    args << "-wnd";
    args << sWndCfgPath.GetData();
  }

  if (m_IPC.StartClientProcess("EditorEngineProcess.exe", args, false, pFirstAllowedMessageType).Failed())
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

void ezEditorEngineProcessConnection::ActivateRemoteProcess(const ezDocument* pDocument, ezUInt32 uiViewID)
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
  m_pRemoteProcess->ConnectToServer(dlg.GetResultingAddress().toUtf8().data());

  ezQtWaitForOperationDlg waitDialog(QApplication::activeWindow());
  waitDialog.m_OnIdle = [this]() -> bool {
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
    msg.m_sAssetPlatformConfig = ezAssetCurator::GetSingleton()->GetActivePlatformConfig()->GetConfigName();

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

ezResult ezEditorEngineProcessConnection::WaitForMessage(const ezRTTI* pMessageType, ezTime tTimeout,
                                                         ezProcessCommunicationChannel::WaitForMessageCallback* pCallback)
{
  EZ_PROFILE(pMessageType->GetTypeName());
  return m_IPC.WaitForMessage(pMessageType, tTimeout, pCallback);
}

ezResult
ezEditorEngineProcessConnection::WaitForDocumentMessage(const ezUuid& assetGuid, const ezRTTI* pMessageType, ezTime tTimeout,
                                                        ezProcessCommunicationChannel::WaitForMessageCallback* pCallback /*= nullptr*/)
{
  if (!m_bProcessShouldBeRunning)
  {
    return EZ_FAILURE; // if the process is not running, we can't wait for a message
  }
  EZ_ASSERT_DEBUG(pMessageType->IsDerivedFrom(ezGetStaticRTTI<ezEditorEngineDocumentMsg>()),
                  "The type of the message to wait for must be a document message.");
  struct WaitData
  {
    ezUuid m_AssetGuid;
    ezProcessCommunicationChannel::WaitForMessageCallback* m_pCallback;
  };

  WaitData data;
  data.m_AssetGuid = assetGuid;
  data.m_pCallback = pCallback;

  ezProcessCommunicationChannel::WaitForMessageCallback callback = [&data](ezProcessMessage* pMsg) -> bool {
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

  return m_IPC.WaitForMessage(pMessageType, tTimeout, &callback);
}

ezResult ezEditorEngineProcessConnection::RestartProcess()
{
  EZ_PROFILE("RestartProcess");
  EZ_LOG_BLOCK("Restarting Engine Process");

  ShutdownProcess();

  Initialize(ezGetStaticRTTI<ezSetupProjectMsgToEngine>());

  if (m_bProcessCrashed)
  {
    ezLog::Error("Engine process crashed during startup.");
    ShutdownProcess();
    return EZ_FAILURE;
  }

  {
    // Send project setup.
    ezSetupProjectMsgToEngine msg;
    msg.m_sProjectDir = ezToolsProject::GetSingleton()->GetProjectDirectory();
    msg.m_FileSystemConfig = m_FileSystemConfig;
    msg.m_PluginConfig = m_PluginConfig;
    msg.m_sAssetPlatformConfig = ezAssetCurator::GetSingleton()->GetActivePlatformConfig()->GetConfigName();

    ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }

  ezLog::Dev("Waiting for Engine Process response");

  if (ezEditorEngineProcessConnection::GetSingleton()->WaitForMessage(ezGetStaticRTTI<ezProjectReadyMsgToEditor>(), ezTime()).Failed())
  {
    ezLog::Error("Failed to restart the engine process");
    ShutdownProcess();
    return EZ_FAILURE;
  }

  ezLog::Dev("Transmitting open documents to Engine Process");

  // resend all open documents
  for (auto it = m_DocumentByGuid.GetIterator(); it.IsValid(); ++it)
  {
    SendDocumentOpenMessage(it.Value(), true);
  }

  ezLog::Success("Engine Process is running");

  m_bClientIsConfigured = true;
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
