#include <PCH.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <QMessageBox>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Document/Document.h>

ezEditorEngineProcessConnection* ezEditorEngineProcessConnection::s_pInstance = nullptr;
ezEvent<const ezEditorEngineProcessConnection::Event&> ezEditorEngineProcessConnection::s_Events;

ezEditorEngineProcessConnection::ezEditorEngineProcessConnection()
{
  EZ_ASSERT(s_pInstance == nullptr, "Incorrect use of ezEditorEngineProcessConnection");
  s_pInstance = this;
  m_iNumViews = 0;
  m_uiNextEngineViewID = 0;
  m_bProcessShouldBeRunning = false;
  m_bProcessCrashed = false;
}

ezEditorEngineProcessConnection::~ezEditorEngineProcessConnection()
{
  EZ_ASSERT(m_iNumViews == 0, "There are still views open at shutdown");

  s_pInstance = nullptr;
}

ezEditorEngineConnection* ezEditorEngineProcessConnection::CreateEngineConnection(ezDocumentBase* pDocument)
{
  if (m_iNumViews == 0)
  {
    Initialize();
  }

  ++m_iNumViews;

  ezEditorEngineConnection* pView = new ezEditorEngineConnection(pDocument, m_uiNextEngineViewID);

  m_EngineViewsByID[pView->m_iEngineViewID] = pView;

  m_uiNextEngineViewID++;

  return pView;
}

void ezEditorEngineProcessConnection::DestroyEngineConnection(ezEditorEngineConnection* pView)
{
  m_EngineViewsByID.Remove(pView->m_iEngineViewID);

  delete pView;

  --m_iNumViews;

  if (m_iNumViews == 0)
  {
    Deinitialize();
  }
}

void ezEditorEngineProcessConnection::Initialize()
{
  if (m_IPC.IsClientAlive())
    return;

  m_bProcessShouldBeRunning = true;
  m_bProcessCrashed = false;

  if (m_IPC.StartClientProcess("EditorEngineProcess.exe").Failed())
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

void ezEditorEngineProcessConnection::Deinitialize()
{
  if (!m_bProcessShouldBeRunning)
    return;

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

void ezEditorEngineProcessConnection::RestartProcess()
{
  Deinitialize();

  Initialize();
}

void ezEditorEngineProcessConnection::Update()
{
  if (!m_bProcessShouldBeRunning)
    return;

  if (!m_IPC.IsClientAlive())
  {
    Deinitialize();
    m_bProcessCrashed = true;

    Event e;
    e.m_Type = Event::Type::ProcessCrashed;
    s_Events.Broadcast(e);

    return;
  }

  m_IPC.ProcessMessages();
}

void ezEditorEngineConnection::SendMessage(ezEngineProcessMsg* pMessage)
{
  pMessage->m_uiViewID = m_iEngineViewID;
  pMessage->m_DocumentGuid = m_pDocument->GetGuid();

  ezEditorEngineProcessConnection::GetInstance()->SendMessage(pMessage);
}
