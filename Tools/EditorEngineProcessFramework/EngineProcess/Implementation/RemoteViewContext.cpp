#include <PCH.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <RendererFoundation/Device/Device.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Pipeline/View.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>

ezUInt32 ezRemoteEngineProcessViewContext::s_uiActiveViewID = 0;

ezRemoteEngineProcessViewContext::ezRemoteEngineProcessViewContext(ezEngineProcessDocumentContext* pContext)
  : ezEngineProcessViewContext(pContext)
{

}

ezRemoteEngineProcessViewContext::~ezRemoteEngineProcessViewContext()
{
  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hView, pView))
  {
    pView->SetWorld(nullptr);
  }

  // make sure the base class destructor doesn't destroy the view
  m_hView.Invalidate();

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  auto pHoloFramework = ezMixedRealityFramework::GetSingleton();
  if (pHoloFramework)
  {
    // make sure the camera is not synchronized further
    if (pHoloFramework->GetCameraForPredictionSynchronization() == &m_Camera)
    {
      pHoloFramework->SetCameraForPredictionSynchronization(nullptr);
    }
  }
#endif
}

void ezRemoteEngineProcessViewContext::HandleViewMessage(const ezEditorEngineViewMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezActivateRemoteViewMsgToEngine>())
  {
    if (m_hView.IsInvalidated())
    {
      m_hView = ezEditorEngineProcessApp::GetSingleton()->CreateRemoteWindowAndView(&m_Camera);
    }

    ezView* pView = nullptr;
    if (ezRenderWorld::TryGetView(m_hView, pView))
    {
      ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
      pView->SetWorld(pDocumentContext->GetWorld());
      pView->SetCamera(&m_Camera);

      s_uiActiveViewID = pMsg->m_uiViewID;
    }
  }

  // ignore all messages for views that are currently not activated
  if (pMsg->m_uiViewID != s_uiActiveViewID)
    return;

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewRedrawMsgToEngine>())
  {
    const ezViewRedrawMsgToEngine* pMsg2 = static_cast<const ezViewRedrawMsgToEngine*>(pMsg);
    SetCamera(pMsg2);

    // skip the on-message redraw, in remote mode it will just render as fast as it can
    //Redraw(false);
  }
}

ezViewHandle ezRemoteEngineProcessViewContext::CreateView()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezViewHandle();
}
