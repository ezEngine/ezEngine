#include <EnginePluginPhysX/EnginePluginPhysXPCH.h>

#include <EnginePluginPhysX/CollisionMeshAsset/CollisionMeshContext.h>
#include <EnginePluginPhysX/CollisionMeshAsset/CollisionMeshView.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezCollisionMeshViewContext::ezCollisionMeshViewContext(ezCollisionMeshContext* pMeshContext)
  : ezEngineProcessViewContext(pMeshContext)
{
  m_pContext = pMeshContext;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(ezVec3(1, 1, 1), ezVec3::MakeZero(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezCollisionMeshViewContext::~ezCollisionMeshViewContext() = default;

bool ezCollisionMeshViewContext::UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -ezVec3(5, -2, 3));
}


ezViewHandle ezCollisionMeshViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("Collision Mesh Editor - View", pView);
  pView->SetCameraUsageHint(ezCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void ezCollisionMeshViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    ezEngineProcessViewContext::DrawSimpleGrid();
  }

  ezEngineProcessViewContext::SetCamera(pMsg);

  const ezUInt32 viewHeight = pMsg->m_uiWindowHeight;

  auto hResource = m_pContext->GetMesh();
  if (hResource.IsValid())
  {
    ezResourceLock<ezPxMeshResource> pResource(hResource, ezResourceAcquireMode::AllowLoadingFallback);
    ezBoundingBox bbox = pResource->GetBounds().GetBox();
    ezUInt32 uiNumPolys = pResource->GetNumPolygons();
    ezUInt32 uiNumVertices = pResource->GetNumVertices();

    ezStringBuilder sText;
    sText.AppendFormat("Polygons: \t{}\n", uiNumPolys);
    sText.AppendFormat("Vertices: \t{}\n", uiNumVertices);
    sText.AppendFormat("Bounding Box: \twidth={0}, depth={1}, height={2}", ezArgF(bbox.GetHalfExtents().x * 2, 2),
      ezArgF(bbox.GetHalfExtents().y * 2, 2), ezArgF(bbox.GetHalfExtents().z * 2, 2));

    ezDebugRenderer::DrawInfoText(m_hView, ezDebugRenderer::ScreenPlacement::BottomLeft, "AssetStats", sText);
  }
}
