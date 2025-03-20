#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshContext.h>
#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshView.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Resources/Buffer.h>

ezAnimatedMeshViewContext::ezAnimatedMeshViewContext(ezAnimatedMeshContext* pAnimatedMeshContext)
  : ezEngineProcessViewContext(pAnimatedMeshContext)
{
  m_pContext = pAnimatedMeshContext;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(ezVec3(1, 1, 1), ezVec3::MakeZero(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezAnimatedMeshViewContext::~ezAnimatedMeshViewContext() = default;

bool ezAnimatedMeshViewContext::UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -ezVec3(5, -2, 3));
}


ezViewHandle ezAnimatedMeshViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("AnimatedMesh Editor - View", pView);
  pView->SetCameraUsageHint(ezCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void ezAnimatedMeshViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    ezEngineProcessViewContext::DrawSimpleGrid();
  }

  ezEngineProcessViewContext::SetCamera(pMsg);

  auto hAnimatedMesh = m_pContext->GetAnimatedMesh();
  if (hAnimatedMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pAnimatedMesh(hAnimatedMesh, ezResourceAcquireMode::AllowLoadingFallback);
    ezResourceLock<ezMeshBufferResource> pAnimatedMeshBuffer(pAnimatedMesh->GetMeshBuffer(), ezResourceAcquireMode::AllowLoadingFallback);

    ezUInt32 uiNumVertices = 0;
    ezUInt32 uiVertexByteSize = 0;
    for (auto hBuffer : pAnimatedMeshBuffer->GetVertexBuffers())
    {
      if (auto pBuffer = ezGALDevice::GetDefaultDevice()->GetBuffer(hBuffer))
      {
        auto& bufferDesc = pBuffer->GetDescription();
        uiNumVertices = ezMath::Max(uiNumVertices, bufferDesc.m_uiTotalSize / bufferDesc.m_uiStructSize);
        uiVertexByteSize += bufferDesc.m_uiStructSize;
      }
    }

    ezUInt32 uiNumTriangles = pAnimatedMeshBuffer->GetPrimitiveCount();
    ezVec3 bboxExtents = ezVec3(2);

    if (pAnimatedMeshBuffer->GetBounds().IsValid())
    {
      bboxExtents = pAnimatedMeshBuffer->GetBounds().m_vBoxHalfExtents * 2.0f;
    }

    auto& streamConfig = pAnimatedMeshBuffer->GetVertexStreamConfig();
    const ezUInt32 uiNumUVs = streamConfig.HasTexCoord0() + streamConfig.HasTexCoord1();
    const ezUInt32 uiNumColors = streamConfig.HasColor0() + streamConfig.HasColor1();

    ezStringBuilder sText;
    sText.AppendFormat("Bones: \t{}\n", pAnimatedMesh->m_Bones.GetCount());
    sText.AppendFormat("Triangles: \t{}\n", uiNumTriangles);
    sText.AppendFormat("Vertices: \t{}\n", uiNumVertices);
    sText.AppendFormat("UV Channels: \t{}\n", uiNumUVs);
    sText.AppendFormat("Color Channels: \t{}\n", uiNumColors);
    sText.AppendFormat("Bytes Per Vertex: \t{}\n", uiVertexByteSize);
    sText.AppendFormat("Bounding Box: \twidth={0}, depth={1}, height={2}\t", ezArgF(bboxExtents.x, 2), ezArgF(bboxExtents.y, 2), ezArgF(bboxExtents.z, 2));

    ezDebugRenderer::DrawInfoText(m_hView, ezDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
