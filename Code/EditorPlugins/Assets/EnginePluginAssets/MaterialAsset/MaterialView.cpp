#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MaterialAsset/MaterialContext.h>
#include <EnginePluginAssets/MaterialAsset/MaterialView.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezMaterialViewContext::ezMaterialViewContext(ezMaterialContext* pMaterialContext)
  : ezEngineProcessViewContext(pMaterialContext)
{
  m_pMaterialContext = pMaterialContext;
}

ezMaterialViewContext::~ezMaterialViewContext() {}

void ezMaterialViewContext::PositionThumbnailCamera()
{
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(+0.18f, -0.18f, 0.1f), ezVec3::ZeroVector(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezViewHandle ezMaterialViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("Material Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());
  pView->SetShaderPermutationVariable("MATERIAL_PREVIEW", "TRUE");

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}
