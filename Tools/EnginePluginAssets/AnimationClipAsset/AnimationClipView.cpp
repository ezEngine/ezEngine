#include <PCH.h>
#include <EnginePluginAssets/AnimationClipAsset/AnimationClipView.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Core/World/GameObject.h>
#include <Core/World/Component.h>
#include <EnginePluginAssets/AnimationClipAsset/AnimationClipContext.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>

ezAnimationClipViewContext::ezAnimationClipViewContext(ezAnimationClipContext* pContext)
  : ezEngineProcessViewContext(pContext)
{
  m_pContext = pContext;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(1, 1, 1), ezVec3::ZeroVector(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezAnimationClipViewContext::~ezAnimationClipViewContext()
{

}

bool ezAnimationClipViewContext::UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, ezVec3(1.0f, 1.0f, -1.0f));
}


ezViewHandle ezAnimationClipViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("Animation Clip Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}
