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

    ezUInt32 uiNumVertices = 0;
    ezUInt32 uiVertexByteSize = 0;
    for (auto hBuffer : pMeshBuffer->GetVertexBuffers())
    {
      if (auto pBuffer = ezGALDevice::GetDefaultDevice()->GetBuffer(hBuffer))
      {
        auto& bufferDesc = pBuffer->GetDescription();
        uiNumVertices = ezMath::Max(uiNumVertices, bufferDesc.m_uiTotalSize / bufferDesc.m_uiStructSize);
        uiVertexByteSize += bufferDesc.m_uiStructSize;
      }
    }

    const ezUInt32 uiNumTriangles = pMeshBuffer->GetPrimitiveCount();
    ezVec3 bboxExtents = ezVec3(2);

    if (pMeshBuffer->GetBounds().IsValid())
    {
      bboxExtents = pMeshBuffer->GetBounds().m_vBoxHalfExtents * 2.0f;
    }

    auto& streamConfig = pMeshBuffer->GetVertexStreamConfig();
    const ezUInt32 uiNumUVs = streamConfig.HasTexCoord0() + streamConfig.HasTexCoord1();
    const ezUInt32 uiNumColors = streamConfig.HasColor0() + streamConfig.HasColor1();

    ezStringBuilder sText;
    sText.AppendFormat("Triangles: \t{}\t\n", uiNumTriangles);
    sText.AppendFormat("Vertices: \t{}\t\n", uiNumVertices);
    sText.AppendFormat("UV Channels: \t{}\t\n", uiNumUVs);
    sText.AppendFormat("Color Channels: \t{}\t\n", uiNumColors);
    sText.AppendFormat("Bytes Per Vertex: \t{}\t\n", uiVertexByteSize);
    sText.AppendFormat("Bounding Box: \twidth={0}, depth={1}, height={2}\t", ezArgF(bboxExtents.x, 2), ezArgF(bboxExtents.y, 2), ezArgF(bboxExtents.z, 2));

    ezDebugRenderer::DrawInfoText(m_hView, ezDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
