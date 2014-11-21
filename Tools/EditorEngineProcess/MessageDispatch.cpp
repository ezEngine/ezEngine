#include <PCH.h>
#include <EditorFramework/EditorApp.moc.h>
#include <EditorFramework/IPC/ProcessCommunication.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>
#include <EditorEngineProcess/Application.h>


void ezEditorProcessApp::EventHandlerIPC(const ezProcessCommunication::Event& e)
{
  const ezEngineProcessMsg* pMsg = (const ezEngineProcessMsg*) e.m_pMessage;

  ezViewContext* pViewContext = (ezViewContext*) ezEngineProcessViewContext::GetViewContext(pMsg->m_uiViewID);

  if (pViewContext == nullptr && pMsg->m_uiViewID != 0xFFFFFFFF)
  {
    ezLog::Info("Created new View 0x%08X for document %s", pMsg->m_uiViewID, ezConversionUtils::ToString(pMsg->m_DocumentGuid).GetData());

    pViewContext = EZ_DEFAULT_NEW(ezViewContext)(pMsg->m_uiViewID, pMsg->m_DocumentGuid);

    ezEngineProcessViewContext::AddViewContext(pMsg->m_uiViewID, pViewContext);
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEngineViewRedrawMsg>())
  {
    ezEngineViewRedrawMsg* pRedrawMsg = (ezEngineViewRedrawMsg*) pMsg;

    pViewContext->SetupRenderTarget((HWND) pRedrawMsg->m_uiHWND, pRedrawMsg->m_uiWindowWidth, pRedrawMsg->m_uiWindowHeight);
    pViewContext->Redraw();
  }
}


