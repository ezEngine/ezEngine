#include <RmlUiPluginPCH.h>

#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RmlUiPlugin/Implementation/RmlUiRenderData.h>
#include <RmlUiPlugin/Implementation/RmlUiRenderer.h>

#include <RendererCore/../../../Data/Plugins/RmlUi/RmlUiConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiRenderer, 1, ezRTTIDefaultAllocator<ezRmlUiRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRmlUiRenderer::ezRmlUiRenderer()
{
  // load the shader
  {
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RmlUi/RmlUi.ezShader");
  }

  // constant buffer storage
  {
    m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezRmlUiConstants>();
  }

  // Setup the vertex declaration
  {
    ezVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
    si.m_Semantic = ezGALVertexAttributeSemantic::Position;
    si.m_Format = ezGALResourceFormat::XYZFloat;
    si.m_uiOffset = offsetof(ezRmlUiInternal::Vertex, m_Position);
    si.m_uiElementSize = sizeof(ezRmlUiInternal::Vertex::m_Position);
  }

  {
    ezVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
    si.m_Semantic = ezGALVertexAttributeSemantic::TexCoord0;
    si.m_Format = ezGALResourceFormat::UVFloat;
    si.m_uiOffset = offsetof(ezRmlUiInternal::Vertex, m_TexCoord);
    si.m_uiElementSize = sizeof(ezRmlUiInternal::Vertex::m_TexCoord);
  }

  {
    ezVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
    si.m_Semantic = ezGALVertexAttributeSemantic::Color0;
    si.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    si.m_uiOffset = offsetof(ezRmlUiInternal::Vertex, m_Color);
    si.m_uiElementSize = sizeof(ezRmlUiInternal::Vertex::m_Color);
  }
}

ezRmlUiRenderer::~ezRmlUiRenderer()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

void ezRmlUiRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const
{
  types.PushBack(ezGetStaticRTTI<ezRmlUiRenderData>());
}

void ezRmlUiRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& categories) const
{
  categories.PushBack(ezDefaultRenderDataCategories::GUI);
}

void ezRmlUiRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  ezGALContext* pGALContext = pRenderContext->GetGALContext();

  pRenderContext->BindShader(m_hShader);
  pRenderContext->BindConstantBuffer("ezRmlUiConstants", m_hConstantBuffer);

  for (auto it = batch.GetIterator<ezRmlUiRenderData>(); it.IsValid(); ++it)
  {
    const ezRmlUiRenderData* pRenderData = it;

    const ezUInt32 numBatches = pRenderData->m_Batches.GetCount();
    for (ezUInt32 batchIdx = 0; batchIdx < numBatches; ++batchIdx)
    {
      const ezRmlUiInternal::Batch& rmlUiBatch = pRenderData->m_Batches[batchIdx];

      ezRmlUiConstants* pConstants = pRenderContext->GetConstantBufferData<ezRmlUiConstants>(m_hConstantBuffer);
      pConstants->UiTransform = rmlUiBatch.m_Transform;

      pRenderContext->BindMeshBuffer(rmlUiBatch.m_CompiledGeometry.m_hVertexBuffer, rmlUiBatch.m_CompiledGeometry.m_hIndexBuffer, &m_VertexDeclarationInfo, ezGALPrimitiveTopology::Triangles, rmlUiBatch.m_CompiledGeometry.m_uiTriangleCount);

      //pGALContext->SetScissorRect(rmlUiBatch.m_ScissorRect);
      pRenderContext->BindTexture2D("BaseTexture", rmlUiBatch.m_CompiledGeometry.m_hTexture);      

      pRenderContext->DrawMeshBuffer();
    }
  }
}
