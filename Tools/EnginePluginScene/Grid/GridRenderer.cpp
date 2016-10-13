#include <EnginePluginScene/Grid/GridRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGridRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorGridExtractor, 1, ezRTTIDefaultAllocator<ezEditorGridExtractor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("SceneContext", GetSceneContext, SetSceneContext),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGridRenderer, 1, ezRTTIDefaultAllocator<ezGridRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezEditorGridExtractor::ezEditorGridExtractor()
{
  m_pSceneContext = nullptr;
}

ezGridRenderer::ezGridRenderer()
{
}

ezGridRenderer::~ezGridRenderer()
{
}

void ezGridRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezGridRenderData>());
}

void ezGridRenderer::CreateVertexBuffer()
{
  if (!m_hVertexBuffer.IsInvalidated())
    return;

  // load the shader
  {
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Debug/DebugPrimitive.ezShader");
  }

  // Create the vertex buffer
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(GridVertex);
    desc.m_uiTotalSize = BufferSize;
    desc.m_BufferType = ezGALBufferType::VertexBuffer;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hVertexBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }

  // Setup the vertex declaration
  {
    {
      ezVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = ezGALVertexAttributeSemantic::Position;
      si.m_Format = ezGALResourceFormat::XYZFloat;
      si.m_uiOffset = 0;
      si.m_uiElementSize = 12;
    }

    {
      ezVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = ezGALVertexAttributeSemantic::Color;
      si.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
      si.m_uiOffset = 12;
      si.m_uiElementSize = 4;
    }
  }
}

void ezGridRenderer::CreateGrid(const ezGridRenderData& rd)
{
  m_Vertices.Clear();
  m_Vertices.Reserve(100);

  const ezVec3 vCenter(0, 0, 2);
  const ezVec3 vTangent1(1, 0, 0);
  const ezVec3 vTangent2(0, 1, 0);
  const ezInt32 lines = 20;
  const float maxExtent = lines * rd.m_fDensity;

  for (ezInt32 i = -lines; i <= lines; ++i)
  {
    {
      auto& v1 = m_Vertices.ExpandAndGetRef();
      auto& v2 = m_Vertices.ExpandAndGetRef();

      v1.m_color = ezColor::White;
      v1.m_position = vCenter + vTangent1 * rd.m_fDensity * (float)i - vTangent2 * maxExtent;

      v2.m_color = ezColor::White;
      v2.m_position = vCenter + vTangent1 * rd.m_fDensity * (float)i + vTangent2 * maxExtent;
    }

    {
      auto& v1 = m_Vertices.ExpandAndGetRef();
      auto& v2 = m_Vertices.ExpandAndGetRef();

      v1.m_color = ezColor::White;
      v1.m_position = vCenter + vTangent2 * rd.m_fDensity * (float)i - vTangent1 * maxExtent;

      v2.m_color = ezColor::White;
      v2.m_position = vCenter + vTangent2 * rd.m_fDensity * (float)i + vTangent1 * maxExtent;
    }
  }
}

void ezGridRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch)
{
  for (auto it = batch.GetIterator<ezGridRenderData>(); it.IsValid(); ++it)
  {
    CreateGrid(*it);

    if (m_Vertices.IsEmpty())
      return;

    ezRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
    ezGALContext* pGALContext = pRenderContext->GetGALContext();

    CreateVertexBuffer();

    pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
    pRenderContext->BindShader(m_hShader);

    ezUInt32 uiNumLineVertices = m_Vertices.GetCount();
    const GridVertex* pLineData = m_Vertices.GetData();

    while (uiNumLineVertices > 0)
    {
      const ezUInt32 uiNumLineVerticesInBatch = ezMath::Min<ezUInt32>(uiNumLineVertices, LineVerticesPerBatch);
      EZ_ASSERT_DEBUG(uiNumLineVerticesInBatch % 2 == 0, "Vertex count must be a multiple of 2.");

      pGALContext->UpdateBuffer(m_hVertexBuffer, 0, ezMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray());

      pRenderContext->BindMeshBuffer(m_hVertexBuffer, ezGALBufferHandle(), &m_VertexDeclarationInfo, ezGALPrimitiveTopology::Lines, uiNumLineVerticesInBatch / 2);
      pRenderContext->DrawMeshBuffer();

      uiNumLineVertices -= uiNumLineVerticesInBatch;
      pLineData += LineVerticesPerBatch;
    }
  }
}

void ezEditorGridExtractor::Extract(const ezView& view, ezExtractedRenderData* pExtractedRenderData)
{
  if (m_pSceneContext == nullptr || m_pSceneContext->GetGridDensity() <= 0.0f)
    return;

  /// \todo Are these parameters correct?
  ezGridRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezGridRenderData>(nullptr, 0);
  pRenderData->m_GlobalTransform.SetIdentity();
  pRenderData->m_GlobalBounds.SetInvalid();
  pRenderData->m_fDensity = m_pSceneContext->GetGridDensity();

  pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::SimpleForeground, 0);
}
