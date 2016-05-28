#include <PCH.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <QMessageBox>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

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
  m.m_sDocumentType = pDocument->GetDocumentTypeDescriptor().m_sDocumentTypeName;

  SendMessage(&m);
}

void ezEditorEngineProcessConnection::HandleIPCEvent(const ezProcessCommunication::Event& e)
{
  if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineDocumentMsg>())
  {
    const ezEditorEngineDocumentMsg* pMsg = static_cast<const ezEditorEngineDocumentMsg*>(e.m_pMessage);

    //EZ_ASSERT_DEBUG(m_DocumentWindow3DByGuid.Contains(pMsg->m_DocumentGuid), "The doument '%u' is not known!", pMsg->m_uiViewID);
    ezQtEngineDocumentWindow* pWindow = m_DocumentWindow3DByGuid[pMsg->m_DocumentGuid];

    if (pWindow)
    {
      pWindow->HandleEngineMessage(pMsg);
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

ezEditorEngineConnection* ezEditorEngineProcessConnection::CreateEngineConnection(ezQtEngineDocumentWindow* pWindow)
{
  ezEditorEngineConnection* pConnection = new ezEditorEngineConnection(pWindow->GetDocument());

  m_DocumentWindow3DByGuid[pWindow->GetDocument()->GetGuid()] = pWindow;

  SendDocumentOpenMessage(pWindow->GetDocument(), true);

  return pConnection;
}

void ezEditorEngineProcessConnection::DestroyEngineConnection(ezQtEngineDocumentWindow* pWindow)
{
  SendDocumentOpenMessage(pWindow->GetDocument(), false);

  m_DocumentWindow3DByGuid.Remove(pWindow->GetDocument()->GetGuid());

  delete pWindow->GetEditorEngineConnection();
}

void ezEditorEngineProcessConnection::Initialize(const ezRTTI* pFirstAllowedMessageType)
{
  if (m_IPC.IsClientAlive())
    return;

  ezLog::Info("Starting Client Engine Process");

  m_bProcessShouldBeRunning = true;
  m_bProcessCrashed = false;
  m_bClientIsConfigured = false;

  if (m_IPC.StartClientProcess("EditorEngineProcess.exe", m_bProcessShouldWaitForDebugger ? "-debug" : "", pFirstAllowedMessageType).Failed())
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

ezResult ezEditorEngineProcessConnection::WaitForMessage(const ezRTTI* pMessageType, ezTime tTimeout)
{
  return m_IPC.WaitForMessage(pMessageType, tTimeout);
}

ezResult ezEditorEngineProcessConnection::RestartProcess()
{
  EZ_LOG_BLOCK("Restarting Engine Process");

  ShutdownProcess();

  Initialize(ezGetStaticRTTI<ezSetupProjectMsgToEngine>());

  {
    // Send project setup.
    ezSetupProjectMsgToEngine msg;
    msg.m_sProjectDir = m_FileSystemConfig.GetProjectDirectory();
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
  for (auto it = m_DocumentWindow3DByGuid.GetIterator(); it.IsValid(); ++it)
  {
    SendDocumentOpenMessage(it.Value()->GetDocument(), true);
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

