#include <PCH.h>
#include <EnginePluginScene/Grid/GridRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <RendererCore/Pipeline/View.h>

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

  const ezVec3 vCenter = rd.m_GlobalTransform.m_vPosition;
  const ezVec3 vTangent1 = rd.m_GlobalTransform.m_Rotation * ezVec3(1, 0, 0);
  const ezVec3 vTangent2 = rd.m_GlobalTransform.m_Rotation * ezVec3(0, 1, 0);
  const ezInt32 lines = rd.m_uiNumLines;
  const float maxExtent = lines * rd.m_fDensity;

  const ezColor cCenter = ezColor::LightSkyBlue;// ezColor::GreenYellow;
  const ezColor cTen = ezColor::LightSteelBlue;
  const ezColor cOther = ezColor::LightSlateGray;

  //static int color = 0;
  //static ezTime tLast;

  //const ezColor cols[] =
  //{
  //  // gut

  //  ezColor::Silver,
  //  ezColor::LightSteelBlue,
  //  ezColor::LightSlateGray,
  //  ezColor::DarkGray
  //};

  //if (ezTime::Now() - tLast > ezTime::Seconds(3.0))
  //{
  //  tLast = ezTime::Now();

  //  color = (color + 1) % EZ_ARRAY_SIZE(cols);

  //  ezLog::Info("New Color: %i", color);
  //}

  //cOther = cols[color];

  for (ezInt32 i = -lines; i <= lines; ++i)
  {
    ezColor cCur = cOther;

    if (i == 0)
      cCur = cCenter;
    else if (i % 10 == 0)
      cCur = cTen;

    {
      auto& v1 = m_Vertices.ExpandAndGetRef();
      auto& v2 = m_Vertices.ExpandAndGetRef();

      v1.m_color = cCur;
      v1.m_position = vCenter + vTangent1 * rd.m_fDensity * (float)i - vTangent2 * maxExtent;

      v2.m_color = cCur;
      v2.m_position = vCenter + vTangent1 * rd.m_fDensity * (float)i + vTangent2 * maxExtent;
    }

    {
      auto& v1 = m_Vertices.ExpandAndGetRef();
      auto& v2 = m_Vertices.ExpandAndGetRef();

      v1.m_color = cCur;
      v1.m_position = vCenter + vTangent2 * rd.m_fDensity * (float)i - vTangent1 * maxExtent;

      v2.m_color = cCur;
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
  pRenderData->m_GlobalBounds.SetInvalid();
  pRenderData->m_fDensity = m_pSceneContext->GetGridDensity();
  pRenderData->m_uiNumLines = 30;

  const ezCamera* cam = view.GetRenderCamera();

  if (cam->IsOrthographic())
  {
    pRenderData->m_GlobalTransform.SetIdentity();
    pRenderData->m_GlobalTransform.m_vPosition = cam->GetCenterPosition();
    pRenderData->m_GlobalTransform.m_Rotation.SetColumn(0, cam->GetCenterDirRight());
    pRenderData->m_GlobalTransform.m_Rotation.SetColumn(1, cam->GetCenterDirUp());
    pRenderData->m_GlobalTransform.m_Rotation.SetColumn(2, cam->GetCenterDirForwards());

    ezVec3& val = pRenderData->m_GlobalTransform.m_vPosition;
    val.x = ezMath::Round(val.x, pRenderData->m_fDensity);
    val.y = ezMath::Round(val.y, pRenderData->m_fDensity);
    val.z = ezMath::Round(val.z, pRenderData->m_fDensity);
  }
  else
  {
    const auto& sel = m_pSceneContext->GetSelection();

    if (sel.IsEmpty())
      return;

    pRenderData->m_GlobalTransform = m_pSceneContext->GetGridTransform();

    // grid is disabled
    if (pRenderData->m_GlobalTransform.m_Rotation.IsZero(0.001f))
      return;
  }

  pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::SimpleForeground, 0);
}





