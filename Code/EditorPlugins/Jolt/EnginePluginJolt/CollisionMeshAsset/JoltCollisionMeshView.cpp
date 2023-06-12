#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <EnginePluginJolt/CollisionMeshAsset/JoltCollisionMeshContext.h>
#include <EnginePluginJolt/CollisionMeshAsset/JoltCollisionMeshView.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezJoltCollisionMeshViewContext::ezJoltCollisionMeshViewContext(ezJoltCollisionMeshContext* pMeshContext)
  : ezEngineProcessViewContext(pMeshContext)
{
  m_pContext = pMeshContext;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(ezVec3(1, 1, 1), ezVec3::ZeroVector(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezJoltCollisionMeshViewContext::~ezJoltCollisionMeshViewContext() = default;

bool ezJoltCollisionMeshViewContext::UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -ezVec3(5, -2, 3));
}


ezViewHandle ezJoltCollisionMeshViewContext::CreateView()
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

void ezJoltCollisionMeshViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
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
    ezResourceLock<ezJoltMeshResource> pResource(hResource, ezResourceAcquireMode::AllowLoadingFallback);
    ezBoundingBox bbox = pResource->GetBounds().GetBox();
    ezUInt32 uiNumTris = pResource->GetNumTriangles();
    ezUInt32 uiNumVertices = pResource->GetNumVertices();
    ezUInt32 uiNumPieces = pResource->GetNumConvexParts() + (pResource->HasTriangleMesh() ? 1 : 0);

    ezStringBuilder sText;
    sText.AppendFormat("Triangles: \t{}\n", uiNumTris);
    sText.AppendFormat("Vertices: \t{}\n", uiNumVertices);
    sText.AppendFormat("Pieces: \t{}\n", uiNumPieces);
    sText.AppendFormat("Bounding Box: \twidth={0}, depth={1}, height={2}", ezArgF(bbox.GetHalfExtents().x * 2, 2),
      ezArgF(bbox.GetHalfExtents().y * 2, 2), ezArgF(bbox.GetHalfExtents().z * 2, 2));

    ezDebugRenderer::DrawInfoText(m_hView, ezDebugRenderer::ScreenPlacement::BottomLeft, "AssetStats", sText);
  }
}
