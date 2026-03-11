#include <EnginePluginRmlUi/EnginePluginRmlUiPCH.h>

#include <EnginePluginRmlUi/RmlUiAsset/RmlUiDocumentContext.h>
#include <EnginePluginRmlUi/RmlUiAsset/RmlUiViewContext.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezRmlUiViewContext::ezRmlUiViewContext(ezRmlUiDocumentContext* pRmlUiContext)
  : ezEngineProcessViewContext(pRmlUiContext)
{
  m_pRmlUiContext = pRmlUiContext;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.01f, 1000.0f);
  m_Camera.LookAt(ezVec3(1, 1, 1), ezVec3::MakeZero(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezRmlUiViewContext::~ezRmlUiViewContext() = default;

bool ezRmlUiViewContext::UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -ezVec3(5, -2, 3));
}

ezViewHandle ezRmlUiViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("Rml Ui Editor - View", pView);
  pView->SetCameraUsageHint(ezCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  pView->SetCameraUsageHint(ezCameraUsageHint::EditorView);
  return pView->GetHandle();
}

void ezRmlUiViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  ezEngineProcessViewContext::SetCamera(pMsg);

  /*const ezUInt32 viewHeight = pMsg->m_uiWindowHeight;

  auto hResource = m_pRmlUiContext->GetResource();
  if (hResource.IsValid())
  {
    ezResourceLock<ezRmlUiResource> pResource(hResource, ezResourceAcquireMode::AllowLoadingFallback);

    if (pResource->GetDetails().m_Bounds.IsValid())
    {
      const ezBoundingBox& bbox = pResource->GetDetails().m_Bounds.GetBox();

      ezStringBuilder sText;
      sText.PrependFormat("Bounding Box: width={0}, depth={1}, height={2}", ezArgF(bbox.GetHalfExtents().x * 2, 2),
                          ezArgF(bbox.GetHalfExtents().y * 2, 2), ezArgF(bbox.GetHalfExtents().z * 2, 2));

      ezDebugRenderer::DrawInfoText(m_hView, sText, ezVec2I32(10, viewHeight - 26), ezColor::White);
    }
  }*/
}
