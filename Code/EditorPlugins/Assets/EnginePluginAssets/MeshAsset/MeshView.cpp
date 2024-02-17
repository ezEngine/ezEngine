#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MeshAsset/MeshContext.h>
#include <EnginePluginAssets/MeshAsset/MeshView.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Resources/Buffer.h>

ezMeshViewContext::ezMeshViewContext(ezMeshContext* pMeshContext)
  : ezEngineProcessViewContext(pMeshContext)
{
  m_pContext = pMeshContext;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(ezVec3(1, 1, 1), ezVec3::MakeZero(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezMeshViewContext::~ezMeshViewContext() = default;

bool ezMeshViewContext::UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -ezVec3(5, -2, 3));
}


ezViewHandle ezMeshViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("Mesh Editor - View", pView);
  pView->SetCameraUsageHint(ezCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void ezMeshViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    ezEngineProcessViewContext::DrawSimpleGrid();
  }

  ezEngineProcessViewContext::SetCamera(pMsg);

  auto hMesh = m_pContext->GetMesh();
  if (hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(hMesh, ezResourceAcquireMode::AllowLoadingFallback);
    ezResourceLock<ezMeshBufferResource> pMeshBuffer(pMesh->GetMeshBuffer(), ezResourceAcquireMode::AllowLoadingFallback);

    auto& bufferDesc = ezGALDevice::GetDefaultDevice()->GetBuffer(pMeshBuffer->GetVertexBuffer())->GetDescription();

    ezUInt32 uiNumVertices = bufferDesc.m_uiTotalSize / bufferDesc.m_uiStructSize;
    ezUInt32 uiNumTriangles = pMeshBuffer->GetPrimitiveCount();
    ezBoundingBox bbox = ezBoundingBox::MakeFromMinMax(ezVec3(-1), ezVec3(1));

    if (pMeshBuffer->GetBounds().IsValid())
    {
      bbox = pMeshBuffer->GetBounds().GetBox();
    }

    ezUInt32 uiNumUVs = 0;
    ezUInt32 uiNumColors = 0;
    for (auto& vertexStream : pMeshBuffer->GetVertexDeclaration().m_VertexStreams)
    {
      if (vertexStream.m_Semantic >= ezGALVertexAttributeSemantic::TexCoord0 && vertexStream.m_Semantic <= ezGALVertexAttributeSemantic::TexCoord9)
      {
        ++uiNumUVs;
      }
      else if (vertexStream.m_Semantic >= ezGALVertexAttributeSemantic::Color0 && vertexStream.m_Semantic <= ezGALVertexAttributeSemantic::Color7)
      {
        ++uiNumColors;
      }
    }

    ezStringBuilder sText;
    sText.AppendFormat("Triangles: \t{}\t\n", uiNumTriangles);
    sText.AppendFormat("Vertices: \t{}\t\n", uiNumVertices);
    sText.AppendFormat("UV Channels: \t{}\t\n", uiNumUVs);
    sText.AppendFormat("Color Channels: \t{}\t\n", uiNumColors);
    sText.AppendFormat("Bytes Per Vertex: \t{}\t\n", bufferDesc.m_uiStructSize);
    sText.AppendFormat("Bounding Box: \twidth={0}, depth={1}, height={2}\t", ezArgF(bbox.GetHalfExtents().x * 2, 2),
      ezArgF(bbox.GetHalfExtents().y * 2, 2), ezArgF(bbox.GetHalfExtents().z * 2, 2));

    ezDebugRenderer::DrawInfoText(m_hView, ezDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
