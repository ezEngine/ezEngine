#include <PCH.h>
#include <EditorFramework/EditorApp.moc.h>
#include <EditorFramework/IPC/ProcessCommunication.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcess/ViewContext.h>
#include <EditorEngineProcess/Application.h>



void ezEditorProcessApp::EventHandlerIPC(const ezProcessCommunication::Event& e)
{
  const ezEngineProcessMsg* pMsg = (const ezEngineProcessMsg*) e.m_pMessage;

  if (pMsg->m_iTargetID >= (ezInt32) m_ViewContexts.GetCount() ||
      m_ViewContexts[pMsg->m_iTargetID] == nullptr)
  {
    m_ViewContexts.SetCount(pMsg->m_iTargetID + 1);
    m_ViewContexts[pMsg->m_iTargetID] = new ezViewContext();
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEngineViewRedrawMsg>())
  {
    ezEngineViewRedrawMsg* pRedrawMsg = (ezEngineViewRedrawMsg*) pMsg;

    m_ViewContexts[pMsg->m_iTargetID]->SetupRenderTarget((HWND) pRedrawMsg->m_uiHWND, pRedrawMsg->m_uiWindowWidth, pRedrawMsg->m_uiWindowHeight);

    m_ViewContexts[pMsg->m_iTargetID]->Redraw(pMsg->m_iTargetID);
  }
}