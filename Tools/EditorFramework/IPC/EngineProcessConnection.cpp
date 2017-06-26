#include <PCH.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <EditorFramework/Assets/AssetDocument.h>

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
  if (!pDocument)
    return;

  ezDocumentOpenMsgToEngine m;
  m.m_DocumentGuid = pDocument->GetGuid();
  m.m_bDocumentOpen = bOpen;
  m.m_sDocumentType = pDocument->GetDocumentTypeDescriptor()->m_sDocumentTypeName;

  SendMessage(&m);
}

void ezEditorEngineProcessConnection::HandleIPCEvent(const ezProcessCommunication::Event& e)
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
  if (m_IPC.IsClientAlive())
    return;

  ezLog::Info("Starting Client Engine Process");

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

  if (m_IPC.StartClientProcess("EditorEngineProcess.exe", args, pFirstAllowedMessageType).Failed())
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

void ezEditorEngineProcessConnection::ShutdownProcess()
{
  if (!m_bProcessShouldBeRunning)
    return;

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
}

ezResult ezEditorEngineProcessConnection::WaitForMessage(const ezRTTI* pMessageType, ezTime tTimeout, ezProcessCommunication::WaitForMessageCallback* pCallback)
{
  return m_IPC.WaitForMessage(pMessageType, tTimeout, pCallback);
}

ezResult ezEditorEngineProcessConnection::WaitForDocumentMessage(const ezUuid& assetGuid, const ezRTTI* pMessageType, ezTime tTimeout, ezProcessCommunication::WaitForMessageCallback* pCallback /*= nullptr*/)
{
  EZ_ASSERT_DEBUG(pMessageType->IsDerivedFrom(ezGetStaticRTTI<ezEditorEngineDocumentMsg>()), "The type of the message to wait for must be a document message.");
  struct WaitData
  {
    ezUuid m_AssetGuid;
    ezProcessCommunication::WaitForMessageCallback* m_pCallback;
  };

  WaitData data;
  data.m_AssetGuid = assetGuid;
  data.m_pCallback = pCallback;

  ezProcessCommunication::WaitForMessageCallback callback = [&data](ezProcessMessage* pMsg)->bool
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

  return m_IPC.WaitForMessage(pMessageType, tTimeout, &callback);
}

ezResult ezEditorEngineProcessConnection::RestartProcess()
{
  EZ_LOG_BLOCK("Restarting Engine Process");

  ShutdownProcess();

  Initialize(ezGetStaticRTTI<ezSetupProjectMsgToEngine>());

  {
    // Send project setup.
    ezSetupProjectMsgToEngine msg;
    msg.m_sProjectDir = ezToolsProject::GetSingleton()->GetProjectDirectory();
    msg.m_FileSystemConfig = m_FileSystemConfig;
    msg.m_PluginConfig = m_PluginConfig;
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
}

void ezEditorEngineConnection::SendMessage(ezEditorEngineDocumentMsg* pMessage)
{
  EZ_ASSERT_DEV(this != nullptr, "No connection between editor and engine was created. This typically happens when an asset document does not enable the engine-connection through the constructor of ezAssetDocument.");

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

