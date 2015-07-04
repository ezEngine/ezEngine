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
  m_iNumViews = 0;
  m_uiNextEngineViewID = 0;
  m_bProcessShouldBeRunning = false;
  m_bProcessCrashed = false;
  m_bClientIsConfigured = false;

  m_IPC.m_Events.AddEventHandler(ezMakeDelegate(&ezEditorEngineProcessConnection::HandleIPCEvent, this));
}

ezEditorEngineProcessConnection::~ezEditorEngineProcessConnection()
{
  EZ_ASSERT_DEV(m_iNumViews == 0, "There are still views open at shutdown");

  m_IPC.m_Events.RemoveEventHandler(ezMakeDelegate(&ezEditorEngineProcessConnection::HandleIPCEvent, this));

  s_pInstance = nullptr;
}

void ezEditorEngineProcessConnection::SendDocumentOpenMessage(ezUInt32 uiViewID, const ezUuid& guid, bool bOpen)
{
  ezDocumentOpenMsgToEngine m;
  m.m_uiViewID = uiViewID;
  m.m_DocumentGuid = guid;
  m.m_bDocumentOpen = bOpen;

  SendMessage(&m);
}

void ezEditorEngineProcessConnection::HandleIPCEvent(const ezProcessCommunication::Event& e)
{
  if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineDocumentMsg>())
  {
    const ezEditorEngineDocumentMsg* pMsg = static_cast<const ezEditorEngineDocumentMsg*>(e.m_pMessage);

    ezDocumentWindow3D* pWindow = m_EngineViewsByID[pMsg->m_uiViewID];

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
  ezEditorEngineConnection* pView = new ezEditorEngineConnection(pWindow->GetDocument(), m_uiNextEngineViewID);

  m_EngineViewsByID[pView->m_iEngineViewID] = pWindow;

  m_uiNextEngineViewID++;

  ++m_iNumViews;

  SendDocumentOpenMessage(pView->m_iEngineViewID, pWindow->GetDocument()->GetGuid(), true);

  return pView;
}

void ezEditorEngineProcessConnection::DestroyEngineConnection(ezDocumentWindow3D* pWindow)
{
  SendDocumentOpenMessage(pWindow->GetEditorEngineConnection()->m_iEngineViewID, pWindow->GetDocument()->GetGuid(), false);

  m_EngineViewsByID.Remove(pWindow->GetEditorEngineConnection()->m_iEngineViewID);

  delete pWindow->GetEditorEngineConnection();

  --m_iNumViews;
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

void ezEditorEngineProcessConnection::SendMessage(ezProcessMessage* pMessage)
{
  m_IPC.SendMessage(pMessage);
}

void ezEditorEngineProcessConnection::WaitForMessage(const ezRTTI* pMessageType)
{
  m_IPC.WaitForMessage(pMessageType);
}

void ezEditorEngineProcessConnection::RestartProcess()
{
  ShutdownProcess();

  Initialize();

  {
    // Send project setup.
    ezSetupProjectMsgToEditor msg;
    msg.m_sProjectDir = m_FileSystemConfig.GetProjectDirectory();
    msg.m_Config = m_FileSystemConfig;
    ezEditorEngineProcessConnection::GetInstance()->SendMessage(&msg);
  }
  ezEditorEngineProcessConnection::GetInstance()->WaitForMessage(ezGetStaticRTTI<ezProjectReadyMsgToEditor>());

  // resend all open documents
  for (auto it = m_EngineViewsByID.GetIterator(); it.IsValid(); ++it)
  {
    SendDocumentOpenMessage(it.Value()->GetEditorEngineConnection()->m_iEngineViewID, it.Value()->GetDocument()->GetGuid(), true);
  }

  m_bClientIsConfigured = true;
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
  pMessage->m_uiViewID = m_iEngineViewID;
  pMessage->m_DocumentGuid = m_pDocument->GetGuid();

  ezEditorEngineProcessConnection::GetInstance()->SendMessage(pMessage);
}

void ezEditorEngineConnection::SendObjectProperties(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_bEditorProperty)
    return;

  ezEntityMsgToEngine msg;
  msg.m_DocumentGuid = m_pDocument->GetGuid();
  msg.m_ObjectGuid = e.m_pObject->GetGuid();
  msg.m_iMsgType = ezEntityMsgToEngine::PropertyChanged;
  msg.m_sObjectType = e.m_pObject->GetTypeAccessor().GetType()->GetTypeName();

  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  ezMemoryStreamReader reader(&storage);

  // TODO: Only write a single property
  ezToolsReflectionUtils::WriteObjectToJSON(writer, e.m_pObject);

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
  msg.m_uiNewChildIndex = e.m_uiNewChildIndex;

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
      ezToolsReflectionUtils::WriteObjectToJSON(writer, e.m_pObject);

      ezStringBuilder sData;
      sData.ReadAll(reader);

      msg.m_sObjectData = sData;
      SendMessage(&msg);

      for (ezUInt32 i = 0; i < e.m_pObject->GetChildren().GetCount(); i++)
      {
        ezDocumentObjectStructureEvent childEvent = e;
        childEvent.m_pNewParent = e.m_pObject;
        childEvent.m_pObject = e.m_pObject->GetChildren()[i];
        childEvent.m_uiNewChildIndex = i;
        SendDocumentTreeChange(childEvent);
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
  ezDocumentObjectStructureEvent msg;
  msg.m_EventType = ezDocumentObjectStructureEvent::Type::AfterObjectAdded;
  msg.m_pObject = pObject;
  msg.m_pNewParent = pObject->GetParent();
  msg.m_pPreviousParent = nullptr;
  msg.m_uiNewChildIndex = msg.m_pNewParent->GetChildIndex((ezDocumentObjectBase*) pObject);

  SendDocumentTreeChange(msg);

  for (auto pChild : pObject->GetChildren())
  {
    SendObject(pChild);
  }
}

