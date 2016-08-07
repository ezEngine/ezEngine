#include <PCH.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <QMessageBox>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
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

    //EZ_ASSERT_DEBUG(m_DocumentWindow3DByGuid.Contains(pMsg->m_DocumentGuid), "The document '%u' is not known!", pMsg->m_uiViewID);
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

ezResult ezEditorEngineProcessConnection::RestartProcess()
{
  EZ_LOG_BLOCK("Restarting Engine Process");

  ShutdownProcess();

  Initialize(ezGetStaticRTTI<ezSetupProjectMsgToEngine>());

  {
    // Send project setup.
    ezSetupProjectMsgToEngine msg;
    msg.m_sProjectDir = ezApplicationConfig::GetProjectDirectory();
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
  pMessage->m_DocumentGuid = m_pDocument->GetGuid();

  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(pMessage);
}

