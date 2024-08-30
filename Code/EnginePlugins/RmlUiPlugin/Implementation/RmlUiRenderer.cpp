#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RmlUiPlugin/Implementation/RmlUiRenderData.h>
#include <RmlUiPlugin/Implementation/RmlUiRenderer.h>

#include <RendererCore/../../../Data/Plugins/Shaders/RmlUiConstants.h>

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
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/RmlUi.ezShader");
  }

  // constant buffer storage
  {
    m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezRmlUiConstants>();
  }

  // quad index buffer
  {
    ezUInt32 indices[] = {0, 1, 2, 0, 2, 3};

    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezUInt32);
    desc.m_uiTotalSize = EZ_ARRAY_SIZE(indices) * desc.m_uiStructSize;
    desc.m_BufferFlags = ezGALBufferUsageFlags::IndexBuffer;

    m_hQuadIndexBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc, ezMakeArrayPtr(indices).ToByteArray());
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

  ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hQuadIndexBuffer);
  m_hQuadIndexBuffer.Invalidate();
}

void ezRmlUiRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(ezGetStaticRTTI<ezRmlUiRenderData>());
}

void ezRmlUiRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(ezDefaultRenderDataCategories::GUI);
}

void ezRmlUiRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezRenderContext* pRenderContext = renderViewContext.m_pRenderContext;

  pRenderContext->BindShader(m_hShader);
  pRenderContext->BindConstantBuffer("ezRmlUiConstants", m_hConstantBuffer);

  // reset cached state
  m_mLastTransform = ezMat4::MakeIdentity();
  m_LastRect = ezRectFloat(0, 0);

  for (auto it = batch.GetIterator<ezRmlUiRenderData>(); it.IsValid(); ++it)
  {
    const ezRmlUiRenderData* pRenderData = it;

    const ezUInt32 numBatches = pRenderData->m_Batches.GetCount();
    for (ezUInt32 batchIdx = 0; batchIdx < numBatches; ++batchIdx)
    {
      const ezRmlUiInternal::Batch& rmlUiBatch = pRenderData->m_Batches[batchIdx];

      ezRmlUiConstants* pConstants = pRenderContext->GetConstantBufferData<ezRmlUiConstants>(m_hConstantBuffer);
      pConstants->UiTransform = rmlUiBatch.m_Transform;
      pConstants->UiTranslation = rmlUiBatch.m_Translation.GetAsVec4(0, 1);

      SetScissorRect(renderViewContext, rmlUiBatch.m_ScissorRect, rmlUiBatch.m_bEnableScissorRect, rmlUiBatch.m_bTransformScissorRect);

      if (rmlUiBatch.m_bTransformScissorRect)
      {
        if (m_mLastTransform != rmlUiBatch.m_Transform || m_LastRect != rmlUiBatch.m_ScissorRect)
        {
          m_mLastTransform = rmlUiBatch.m_Transform;
          m_LastRect = rmlUiBatch.m_ScissorRect;

          PrepareStencil(renderViewContext, rmlUiBatch.m_ScissorRect);
        }

        pRenderContext->SetShaderPermutationVariable("RMLUI_MODE", "RMLUI_MODE_STENCIL_TEST");
      }
      else
      {
        pRenderContext->SetShaderPermutationVariable("RMLUI_MODE", "RMLUI_MODE_NORMAL");
      }

      pRenderContext->BindMeshBuffer(rmlUiBatch.m_CompiledGeometry.m_hVertexBuffer, rmlUiBatch.m_CompiledGeometry.m_hIndexBuffer, &m_VertexDeclarationInfo, ezGALPrimitiveTopology::Triangles, rmlUiBatch.m_CompiledGeometry.m_uiTriangleCount);

      pRenderContext->BindTexture2D("BaseTexture", rmlUiBatch.m_hTexture);

      pRenderContext->DrawMeshBuffer().IgnoreResult();
    }
  }
}

void ezRmlUiRenderer::SetScissorRect(const ezRenderViewContext& renderViewContext, const ezRectFloat& rect, bool bEnable, bool bTransformRect) const
{
  ezRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  ezGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  ezRectFloat scissorRect = rect;
  if (!bEnable || bTransformRect)
  {
    scissorRect = renderViewContext.m_pViewData->m_ViewPortRect;
  }

  ezUInt32 x = static_cast<ezUInt32>(ezMath::Max(scissorRect.x, 0.0f));
  ezUInt32 y = static_cast<ezUInt32>(ezMath::Max(scissorRect.y, 0.0f));
  ezUInt32 width = static_cast<ezUInt32>(ezMath::Max(scissorRect.width, 0.0f));
  ezUInt32 height = static_cast<ezUInt32>(ezMath::Max(scissorRect.height, 0.0f));

  pGALCommandEncoder->SetScissorRect(ezRectU32(x, y, width, height));
}

void ezRmlUiRenderer::PrepareStencil(const ezRenderViewContext& renderViewContext, const ezRectFloat& rect) const
{
  ezRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  ezGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  // Clear stencil
  pGALCommandEncoder->Clear(ezColor::Black, 0, false, true, 1.0f, 0);

  // Draw quad to set stencil pixels
  pRenderContext->SetShaderPermutationVariable("RMLUI_MODE", "RMLUI_MODE_STENCIL_SET");

  ezRmlUiConstants* pConstants = pRenderContext->GetConstantBufferData<ezRmlUiConstants>(m_hConstantBuffer);
  pConstants->QuadVertexPos[0] = ezVec4(rect.x, rect.y, 0, 1);
  pConstants->QuadVertexPos[1] = ezVec4(rect.x + rect.width, rect.y, 0, 1);
  pConstants->QuadVertexPos[2] = ezVec4(rect.x + rect.width, rect.y + rect.height, 0, 1);
  pConstants->QuadVertexPos[3] = ezVec4(rect.x, rect.y + rect.height, 0, 1);

  pRenderContext->BindMeshBuffer(ezGALBufferHandle(), m_hQuadIndexBuffer, nullptr, ezGALPrimitiveTopology::Triangles, 2);
  pRenderContext->DrawMeshBuffer().IgnoreResult();
}


EZ_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Implementation_RmlUiRenderer);
