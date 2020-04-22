#include <EnginePluginAssetsPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EnginePluginAssets/MeshAsset/MeshContext.h>
#include <EnginePluginAssets/MeshAsset/MeshView.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

ezMeshViewContext::ezMeshViewContext(ezMeshContext* pMeshContext)
  : ezEngineProcessViewContext(pMeshContext)
{
  m_pMeshContext = pMeshContext;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(ezVec3(1, 1, 1), ezVec3::ZeroVector(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezMeshViewContext::~ezMeshViewContext() {}

bool ezMeshViewContext::UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -ezVec3(5, -2, 3));
}


ezViewHandle ezMeshViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("Mesh Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void ezMeshViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  ezEngineProcessViewContext::SetCamera(pMsg);

  const ezUInt32 viewHeight = pMsg->m_uiWindowHeight;

  auto hMesh = m_pMeshContext->GetMesh();
  if (hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(hMesh, ezResourceAcquireMode::AllowLoadingFallback);
    ezResourceLock<ezMeshBufferResource> pMeshBuffer(pMesh->GetMeshBuffer(), ezResourceAcquireMode::AllowLoadingFallback);

    auto& bufferDesc = ezGALDevice::GetDefaultDevice()->GetBuffer(pMeshBuffer->GetVertexBuffer())->GetDescription();

    ezUInt32 uiNumVertices = bufferDesc.m_uiTotalSize / bufferDesc.m_uiStructSize;
    ezUInt32 uiNumTriangles = pMeshBuffer->GetPrimitiveCount();
    const ezBoundingBox& bbox = pMeshBuffer->GetBounds().GetBox();

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
    sText.AppendFormat("Triangles: {}\\n", uiNumTriangles);
    sText.AppendFormat("Vertices: {}\\n", uiNumVertices);
    sText.AppendFormat("UV Channels: {}\\n", uiNumUVs);
    sText.AppendFormat("Color Channels: {}\\n", uiNumColors);
    sText.AppendFormat("Bytes Per Vertex: {}\\n", bufferDesc.m_uiStructSize);    
    sText.AppendFormat("Bounding Box: width={0}, depth={1}, height={2}", ezArgF(bbox.GetHalfExtents().x * 2, 2),
      ezArgF(bbox.GetHalfExtents().y * 2, 2), ezArgF(bbox.GetHalfExtents().z * 2, 2));

    ezDebugRenderer::Draw2DText(m_hView, sText, ezVec2I32(10, viewHeight - 10), ezColor::White, 16,
      ezDebugRenderer::HorizontalAlignment::Left, ezDebugRenderer::VerticalAlignment::Bottom);
  }
}
