#include <PCH.h>
#include <EditorFramework/EngineView/EngineView.h>
#include <QMessageBox>
#include <Foundation/Logging/Log.h>

ezEditorEngineViewProcess* ezEditorEngineViewProcess::s_pInstance = nullptr;
ezEvent<const ezEditorEngineViewProcess::Event&> ezEditorEngineViewProcess::s_Events;

ezEditorEngineViewProcess::ezEditorEngineViewProcess()
{
  EZ_ASSERT(s_pInstance == nullptr, "Incorrect use of ezEditorEngineViewProcess");
  s_pInstance = this;
  m_iNumViews = 0;
  m_iNextEngineViewID = 0;
  m_bProcessShouldBeRunning = false;
  m_bProcessCrashed = false;
}

ezEditorEngineViewProcess::~ezEditorEngineViewProcess()
{
  EZ_ASSERT(m_iNumViews == 0, "There are still views open at shutdown");

  s_pInstance = nullptr;
}

ezEditorEngineView* ezEditorEngineViewProcess::CreateEngineView()
{
  if (m_iNumViews == 0)
  {
    Initialize();
  }

  ++m_iNumViews;

  ezEditorEngineView* pView = new ezEditorEngineView();

  pView->m_iEngineViewID = m_iNextEngineViewID;

  m_EngineViewsByID[pView->m_iEngineViewID] = pView;

  m_iNextEngineViewID++;

  return pView;
}

void ezEditorEngineViewProcess::DestroyEngineView(ezEditorEngineView* pView)
{
  m_EngineViewsByID.Remove(pView->m_iEngineViewID);

  delete pView;

  --m_iNumViews;

  if (m_iNumViews == 0)
  {
    Deinitialize();
  }
}

void ezEditorEngineViewProcess::Initialize()
{
  if (m_IPC.IsClientAlive())
    return;

  m_bProcessShouldBeRunning = true;
  m_bProcessCrashed = false;

  if (m_IPC.StartClientProcess("EditorEngineView.exe").Failed())
  {
    m_bProcessCrashed = true;
    ezLog::Error("Failed to start 'EditorEngineView.exe'");
  }
  else
  {
    Event e;
    e.m_Type = Event::Type::ProcessStarted;
    s_Events.Broadcast(e);
  }
}

void ezEditorEngineViewProcess::Deinitialize()
{
  if (!m_bProcessShouldBeRunning)
    return;

  m_bProcessShouldBeRunning = false;
  m_IPC.CloseConnection();

  Event e;
  e.m_Type = Event::Type::ProcessShutdown;
  s_Events.Broadcast(e);
}

void ezEditorEngineViewProcess::SendMessage(ezProcessMessage* pMessage)
{
  m_IPC.SendMessage(pMessage);
}

void ezEditorEngineViewProcess::RestartProcess()
{
  Deinitialize();

  Initialize();
}

void ezEditorEngineViewProcess::Update()
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

void ezEditorEngineView::SendMessage(ezEngineViewMsg* pMessage)
{
  pMessage->m_iTargetID = m_iEngineViewID;

  ezEditorEngineViewProcess::GetInstance()->SendMessage(pMessage);
}
