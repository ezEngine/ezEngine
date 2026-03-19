#include <EnginePluginKraut/EnginePluginKrautPCH.h>

#include <EnginePluginKraut/KrautTreeAsset/KrautTreeContext.h>
#include <EnginePluginKraut/KrautTreeAsset/KrautTreeView.h>
#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezKrautTreeViewContext::ezKrautTreeViewContext(ezKrautTreeContext* pKrautTreeContext)
  : ezEngineProcessViewContext(pKrautTreeContext)
{
  m_pKrautTreeContext = pKrautTreeContext;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(ezVec3(1, 1, 1), ezVec3::MakeZero(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezKrautTreeViewContext::~ezKrautTreeViewContext() = default;

bool ezKrautTreeViewContext::UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -ezVec3(5, -2, 3));
}

ezViewHandle ezKrautTreeViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("Kraut Tree Editor - View", pView);
  pView->SetCameraUsageHint(ezCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  pView->SetCameraUsageHint(ezCameraUsageHint::Thumbnail);
  return pView->GetHandle();
}

void ezKrautTreeViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  ezEngineProcessViewContext::SetCamera(pMsg);

  ezStringBuilder sText;

  // Distance from camera to tree (tree preview is always at world origin)
  const float fDistance = m_Camera.GetPosition().GetLength();
  sText.AppendFormat("Distance: \t{0}m\t\n", ezArgF(fDistance, 1));

  // Determine which regular LOD would be auto-selected at this distance
  ezInt32 iAutoLod = -1; // -1 = beyond all LOD distances (would not render in auto mode)
  ezInt32 iLodOverride = -1;

  auto hGenRes = m_pKrautTreeContext->GetResource();
  if (hGenRes.IsValid())
  {
    ezResourceLock<ezKrautGeneratorResource> pGenRes(hGenRes, ezResourceAcquireMode::AllowLoadingFallback);
    const auto& pDesc = pGenRes->GetDescriptor();
    if (pDesc != nullptr)
    {
      const float fDistSqr = fDistance * fDistance;
      float fPrevMaxDist = 0.0f;

      for (ezUInt32 n = 0; n < 5; ++n)
      {
        const Kraut::LodDesc& lodDesc = pDesc->m_LodDesc[n];
        if (lodDesc.m_Mode != Kraut::LodMode::Full)
          break;

        const float fMaxDist = lodDesc.m_uiLodDistance * pDesc->m_fLodDistanceScale * pDesc->m_fUniformScaling;
        if (fDistSqr >= (fPrevMaxDist * fPrevMaxDist) && fDistSqr < (fMaxDist * fMaxDist))
        {
          iAutoLod = (ezInt32)n;
          break;
        }
        fPrevMaxDist = fMaxDist;
      }
    }
  }

  // Get the active LOD override from the Kraut tree component
  ezWorld* pWorld = m_pKrautTreeContext->GetWorld();
  if (pWorld != nullptr)
  {
    EZ_LOCK(pWorld->GetReadMarker());
    ezKrautTreeComponent* pTree = nullptr;
    if (pWorld->TryGetComponent(m_pKrautTreeContext->GetKrautComponentHandle(), pTree))
      iLodOverride = pTree->m_iLodOverride;
  }

  if (iLodOverride == -1)
  {
    // Automatic LOD selection: the auto-selected LOD is also the active one
    if (iAutoLod < 0)
      sText.AppendFormat("Active LOD: \tNone (hidden)");
    else
      sText.AppendFormat("Active LOD: \tLOD {}", iAutoLod);
  }
  else
  {
    if (iLodOverride == 0)
      sText.AppendFormat("Active LOD: \tFull Detail (fixed)\n");
    else
      sText.AppendFormat("Active LOD: \tLOD {} (fixed)\n", iLodOverride - 1);

    // Fixed LOD: show both the auto-selected and the actually forced LOD
    if (iAutoLod < 0)
      sText.AppendFormat("Auto LOD: \tNone (hidden)");
    else
      sText.AppendFormat("Auto LOD: \tLOD {}", iAutoLod);
  }

  ezDebugRenderer::DrawInfoText(m_hView, ezDebugTextPlacement::BottomLeft, "KrautStats", sText);
}
