#include <PCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

ezUInt32 ezRemoteEngineProcessViewContext::s_uiActiveViewID = 0;
ezRemoteEngineProcessViewContext* ezRemoteEngineProcessViewContext::s_pActiveRemoteViewContext = nullptr;

ezRemoteEngineProcessViewContext::ezRemoteEngineProcessViewContext(ezEngineProcessDocumentContext* pContext)
    : ezEngineProcessViewContext(pContext)
{
}

ezRemoteEngineProcessViewContext::~ezRemoteEngineProcessViewContext()
{
  if (s_pActiveRemoteViewContext == this)
  {
    s_pActiveRemoteViewContext = nullptr;

    ezView* pView = nullptr;
    if (ezRenderWorld::TryGetView(m_hView, pView))
    {
      pView->SetWorld(nullptr);
    }
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

    s_pActiveRemoteViewContext = this;

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
    // Redraw(false);

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
    auto pHoloFramework = ezMixedRealityFramework::GetSingleton();
    if (pHoloFramework)
    {
      if (pMsg2->m_bUseCameraTransformOnDevice)
      {
        ezMat4 m = pMsg2->m_ViewMatrix;
        m.SetRow(1, -pMsg2->m_ViewMatrix.GetRow(2));
        m.SetRow(2, pMsg2->m_ViewMatrix.GetRow(1));

        ezTransform tf;
        tf.SetFromMat4(m);
        tf.m_vScale.Set(0.5f / (1.0f + tf.m_vPosition.GetLength()));
        tf.m_vPosition.SetZero();
        pHoloFramework->SetAdditionalCameraTransform(tf);
      }
      else
      {
        pHoloFramework->SetAdditionalCameraTransform(ezTransform::Identity());
      }
    }
#endif
  }
}

ezViewHandle ezRemoteEngineProcessViewContext::CreateView()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezViewHandle();
}
