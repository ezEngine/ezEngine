#include <PCH.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <QMessageBox>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

ezEditorEngineProcessConnection* ezEditorEngineProcessConnection::s_pInstance = nullptr;
ezEvent<const ezEditorEngineProcessConnection::Event&> ezEditorEngineProcessConnection::s_Events;

ezEditorEngineProcessConnection::ezEditorEngineProcessConnection()
{
  EZ_ASSERT_DEV(s_pInstance == nullptr, "Incorrect use of ezEditorEngineProcessConnection");
  s_pInstance = this;
  m_bProcessShouldBeRunning = false;
  m_bProcessCrashed = false;
  m_bClientIsConfigured = false;

  m_IPC.m_Events.AddEventHandler(ezMakeDelegate(&ezEditorEngineProcessConnection::HandleIPCEvent, this));
}

ezEditorEngineProcessConnection::~ezEditorEngineProcessConnection()
{
  m_IPC.m_Events.RemoveEventHandler(ezMakeDelegate(&ezEditorEngineProcessConnection::HandleIPCEvent, this));

  s_pInstance = nullptr;
}

void ezEditorEngineProcessConnection::SendDocumentOpenMessage(const ezDocumentBase* pDocument, bool bOpen)
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
    ezDocumentWindow3D* pWindow = m_DocumentWindow3DByGuid[pMsg->m_DocumentGuid];

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

ezEditorEngineConnection* ezEditorEngineProcessConnection::CreateEngineConnection(ezDocumentWindow3D* pWindow)
{
  ezEditorEngineConnection* pConnection = new ezEditorEngineConnection(pWindow->GetDocument());

  m_DocumentWindow3DByGuid[pWindow->GetDocument()->GetGuid()] = pWindow;

  SendDocumentOpenMessage(pWindow->GetDocument(), true);

  return pConnection;
}

void ezEditorEngineProcessConnection::DestroyEngineConnection(ezDocumentWindow3D* pWindow)
{
  SendDocumentOpenMessage(pWindow->GetDocument(), false);

  m_DocumentWindow3DByGuid.Remove(pWindow->GetDocument()->GetGuid());

  delete pWindow->GetEditorEngineConnection();
}

void ezEditorEngineProcessConnection::Initialize()
{
  if (m_IPC.IsClientAlive())
    return;

  m_bProcessShouldBeRunning = true;
  m_bProcessCrashed = false;
  m_bClientIsConfigured = false;

  if (m_IPC.StartClientProcess("EditorEngineProcess.exe", m_bProcessShouldWaitForDebugger ? "-debug" : "").Failed())
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

  m_bClientIsConfigured = false;
  m_bProcessShouldBeRunning = false;
  m_IPC.CloseConnection();

  Event e;
  e.m_Type = Event::Type::ProcessShutdown;
  s_Events.Broadcast(e);
}

void ezEditorEngineProcessConnection::SendMessage(ezProcessMessage* pMessage, bool bSuperHighPriority)
{
  m_IPC.SendMessage(pMessage, bSuperHighPriority);
}

ezResult ezEditorEngineProcessConnection::WaitForMessage(const ezRTTI* pMessageType, ezTime tTimeout)
{
  return m_IPC.WaitForMessage(pMessageType, tTimeout);
}

ezResult ezEditorEngineProcessConnection::RestartProcess()
{
  ShutdownProcess();

  Initialize();

  {
    // Send project setup.
    ezSetupProjectMsgToEngine msg;
    msg.m_sProjectDir = m_FileSystemConfig.GetProjectDirectory();
    msg.m_Config = m_FileSystemConfig;
    ezEditorEngineProcessConnection::GetInstance()->SendMessage(&msg);
  }
  if (ezEditorEngineProcessConnection::GetInstance()->WaitForMessage(ezGetStaticRTTI<ezProjectReadyMsgToEditor>(), ezTime()).Failed())
  {
    ezLog::Error("Failed to restart the engine process");
    ShutdownProcess();
    return EZ_FAILURE;
  }

  // resend all open documents
  for (auto it = m_DocumentWindow3DByGuid.GetIterator(); it.IsValid(); ++it)
  {
    SendDocumentOpenMessage(it.Value()->GetDocument(), true);
  }

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

void ezEditorEngineConnection::SendMessage(ezEditorEngineDocumentMsg* pMessage, bool bSuperHighPriority)
{
  pMessage->m_DocumentGuid = m_pDocument->GetGuid();

  ezEditorEngineProcessConnection::GetInstance()->SendMessage(pMessage, bSuperHighPriority);
}

void ezEditorEngineConnection::SendObjectProperties(const ezDocumentObjectPropertyEvent& e)
{
  ezEntityMsgToEngine msg;
  msg.m_DocumentGuid = m_pDocument->GetGuid();
  msg.m_ObjectGuid = e.m_pObject->GetGuid();
  msg.m_iMsgType = ezEntityMsgToEngine::PropertyChanged;
  msg.m_sObjectType = e.m_pObject->GetTypeAccessor().GetType()->GetTypeName();

  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  ezMemoryStreamReader reader(&storage);

  // TODO: Only write a single property
  ezToolsReflectionUtils::WriteObjectToJSON(false, writer, e.m_pObject, ezJSONWriter::WhitespaceMode::All);

  ezStringBuilder sData;
  sData.ReadAll(reader);

  msg.m_sObjectData = sData;

  SendMessage(&msg);
}

void ezEditorEngineConnection::SendDocumentTreeChange(const ezDocumentObjectStructureEvent& e)
{

  ezEntityMsgToEngine msg;
  msg.m_DocumentGuid = m_pDocument->GetGuid();
  msg.m_ObjectGuid = e.m_pObject->GetGuid();
  msg.m_sObjectType = e.m_pObject->GetTypeAccessor().GetType()->GetTypeName();
  msg.m_sParentProperty = e.m_sParentProperty;
  msg.m_PropertyIndex = e.m_PropertyIndex;

  if (e.m_pPreviousParent)
    msg.m_PreviousParentGuid = e.m_pPreviousParent->GetGuid();
  if (e.m_pNewParent)
    msg.m_NewParentGuid = e.m_pNewParent->GetGuid();

  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
    {
      msg.m_iMsgType = ezEntityMsgToEngine::ObjectAdded;

      ezMemoryStreamStorage storage;
      ezMemoryStreamWriter writer(&storage);
      ezMemoryStreamReader reader(&storage);
      ezToolsReflectionUtils::WriteObjectToJSON(false, writer, e.m_pObject, ezJSONWriter::WhitespaceMode::All);

      ezStringBuilder sData;
      sData.ReadAll(reader);

      msg.m_sObjectData = sData;
      SendMessage(&msg);

      // TODO: BLA
      for (ezUInt32 i = 0; i < e.m_pObject->GetChildren().GetCount(); i++)
      {
        SendObject(e.m_pObject->GetChildren()[i]);
      }
      return;
    }
    break;

  case ezDocumentObjectStructureEvent::Type::AfterObjectMoved:
    {
      msg.m_iMsgType = ezEntityMsgToEngine::ObjectMoved;
    }
    break;

  case ezDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    {
      msg.m_iMsgType = ezEntityMsgToEngine::ObjectRemoved;
    }
    break;

  case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
  case ezDocumentObjectStructureEvent::Type::BeforeObjectAdded:
  case ezDocumentObjectStructureEvent::Type::BeforeObjectMoved:
  case ezDocumentObjectStructureEvent::Type::AfterObjectMoved2:
    return;

  default:
    EZ_REPORT_FAILURE("Unknown event type");
    return;
  }

  SendMessage(&msg);
}

void ezEditorEngineConnection::SendDocument()
{
  auto pTree = m_pDocument->GetObjectManager();

  for (auto pChild : pTree->GetRootObject()->GetChildren())
  {
    SendObject(pChild);
  }
}

void ezEditorEngineConnection::SendObject(const ezDocumentObjectBase* pObject)
{
  // TODO: BLA
  ezDocumentObjectStructureEvent msg;
  msg.m_EventType = ezDocumentObjectStructureEvent::Type::AfterObjectAdded;
  msg.m_pObject = pObject;
  msg.m_pNewParent = pObject->GetParent();
  msg.m_pPreviousParent = nullptr;
  msg.m_sParentProperty = pObject->GetParentProperty();
  msg.m_PropertyIndex = pObject->GetPropertyIndex();

  SendDocumentTreeChange(msg);
}

