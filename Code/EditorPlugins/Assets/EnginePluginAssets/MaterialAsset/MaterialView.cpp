#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MaterialAsset/MaterialContext.h>
#include <EnginePluginAssets/MaterialAsset/MaterialView.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezMaterialViewContext::ezMaterialViewContext(ezMaterialContext* pMaterialContext)
  : ezEngineProcessViewContext(pMaterialContext)
{
  m_pMaterialContext = pMaterialContext;
}

ezMaterialViewContext::~ezMaterialViewContext() = default;

void ezMaterialViewContext::PositionThumbnailCamera()
{
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(2.4f, -2.4f, 1.0f), ezVec3::MakeZero(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezViewHandle ezMaterialViewContext::CreateView()
{
  ezView* pView = CreateDefaultView("Material Editor - View");
  pView->SetShaderPermutationVariable("MATERIAL_PREVIEW", "TRUE");

  return pView->GetHandle();
}
