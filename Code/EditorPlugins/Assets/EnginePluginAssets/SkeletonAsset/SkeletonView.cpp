#include <EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/SkeletonAsset/SkeletonContext.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonView.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezSkeletonViewContext::ezSkeletonViewContext(ezSkeletonContext* pContext)
  : ezEngineProcessViewContext(pContext)
{
  m_pContext = pContext;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(1, 1, 1), ezVec3::ZeroVector(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezSkeletonViewContext::~ezSkeletonViewContext() {}

bool ezSkeletonViewContext::UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -ezVec3(5, -2, 3));
}


ezViewHandle ezSkeletonViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("Skeleton Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void ezSkeletonViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    ezEngineProcessViewContext::DrawSimpleGrid();
  }

  ezEngineProcessViewContext::SetCamera(pMsg);

  const ezUInt32 viewHeight = pMsg->m_uiWindowHeight;

  auto hSkeleton = m_pContext->GetSkeleton();
  if (hSkeleton.IsValid())
  {
    ezResourceLock<ezSkeletonResource> pSkeleton(hSkeleton, ezResourceAcquireMode::AllowLoadingFallback);

    ezUInt32 uiNumJoints = pSkeleton->GetDescriptor().m_Skeleton.GetJointCount();

    ezStringBuilder sText;
    sText.AppendFormat("Joints: {}\n", uiNumJoints);

    ezDebugRenderer::Draw2DText(m_hView, sText, ezVec2I32(10, viewHeight - 10), ezColor::White, 16, ezDebugRenderer::HorizontalAlignment::Left,
      ezDebugRenderer::VerticalAlignment::Bottom);
  }
}
