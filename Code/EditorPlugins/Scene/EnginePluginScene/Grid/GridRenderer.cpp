#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Grid/GridRenderer.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Shader/ShaderResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGridRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorGridExtractor, 1, ezRTTIDefaultAllocator<ezEditorGridExtractor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("SceneContext", GetSceneContext, SetSceneContext),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGridRenderer, 1, ezRTTIDefaultAllocator<ezGridRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezEditorGridExtractor::ezEditorGridExtractor(const char* szName)
  : ezExtractor(szName)
{
  m_pSceneContext = nullptr;
}

ezGridRenderer::ezGridRenderer()
{
  CreateVertexBuffer();
}

void ezGridRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(ezGetStaticRTTI<ezGridRenderData>());
}

void ezGridRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(ezDefaultRenderDataCategories::SimpleTransparent);
}

void ezGridRenderer::CreateVertexBuffer()
{
  if (m_VertexBuffer.IsInitialized())
    return;

  // load the shader
  {
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Debug/DebugPrimitive.ezShader");
  }

  // Create the vertex buffer
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(GridVertex);
    desc.m_uiTotalSize = s_uiBufferSize;
    desc.m_BufferFlags = ezGALBufferUsageFlags::VertexBuffer | ezGALBufferUsageFlags::Transient;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_VertexBuffer.Initialize(desc, "GridRenderer - VertexBuffer");
  }

  // Setup the vertex declaration
  {
    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = ezGALVertexAttributeSemantic::Position;
      va.m_eFormat = ezGALResourceFormat::XYZFloat;
      va.m_uiOffset = offsetof(GridVertex, m_position);
    }

    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = ezGALVertexAttributeSemantic::Color0;
      va.m_eFormat = ezGALResourceFormat::RGBAUByteNormalized;
      va.m_uiOffset = offsetof(GridVertex, m_color);
    }
  }
}

void ezGridRenderer::CreateGrid(const ezGridRenderData& rd) const
{
  m_Vertices.Clear();
  m_Vertices.Reserve(100);

  const ezVec3 vCenter = rd.m_GlobalTransform.m_vPosition;
  const ezVec3 vTangent1 = rd.m_GlobalTransform.m_qRotation * ezVec3(1, 0, 0);
  const ezVec3 vTangent2 = rd.m_GlobalTransform.m_qRotation * ezVec3(0, 1, 0);
  const ezInt32 iNumLines1 = rd.m_iLastLine1 - rd.m_iFirstLine1;
  const ezInt32 iNumLines2 = rd.m_iLastLine2 - rd.m_iFirstLine2;
  const float maxExtent1 = iNumLines1 * rd.m_fDensity;
  const float maxExtent2 = iNumLines2 * rd.m_fDensity;

  ezColor cCenter = ezColorScheme::GetColor(ezColorScheme::Blue, 6, 0.5f);
  ezColor cTen = ezColorScheme::LightUI(ezColorScheme::Gray) * 0.7f;
  ezColor cOther = ezColorScheme::LightUI(ezColorScheme::Gray) * 0.5f;

  if (rd.m_bOrthoMode)
  {
    // dimmer colors in ortho mode, to be more in the background
    cCenter *= 0.5f;
    cTen *= 0.4f;
    cOther *= 0.4f;

    // in ortho mode, the origin lines are highlighted when global space is enabled
    if (!rd.m_bGlobal)
    {
      cCenter = cTen;
    }
  }
  else
  {
    // in perspective mode, the lines through the object are highlighted when local space is enabled
    if (rd.m_bGlobal)
    {
      cCenter = cTen;
    }
  }

  const ezVec3 vCorner = vCenter + rd.m_iFirstLine1 * rd.m_fDensity * vTangent1 + rd.m_iFirstLine2 * rd.m_fDensity * vTangent2;

  for (ezInt32 i = 0; i <= iNumLines1; ++i)
  {
    const ezInt32 iLineIdx = rd.m_iFirstLine1 + i;

    ezColor cCur = cOther;

    if (iLineIdx == 0)
      cCur = cCenter;
    else if (iLineIdx % 10 == 0)
      cCur = cTen;

    auto& v1 = m_Vertices.ExpandAndGetRef();
    auto& v2 = m_Vertices.ExpandAndGetRef();

    v1.m_color = cCur;
    v1.m_position = vCorner + vTangent1 * rd.m_fDensity * (float)i;

    v2.m_color = cCur;
    v2.m_position = vCorner + vTangent1 * rd.m_fDensity * (float)i + vTangent2 * maxExtent2;
  }

  for (ezInt32 i = 0; i <= iNumLines2; ++i)
  {
    const ezInt32 iLineIdx = rd.m_iFirstLine2 + i;

    ezColor cCur = cOther;

    if (iLineIdx == 0)
      cCur = cCenter;
    else if (iLineIdx % 10 == 0)
      cCur = cTen;

    auto& v1 = m_Vertices.ExpandAndGetRef();
    auto& v2 = m_Vertices.ExpandAndGetRef();

    v1.m_color = cCur;
    v1.m_position = vCorner + vTangent2 * rd.m_fDensity * (float)i;

    v2.m_color = cCur;
    v2.m_position = vCorner + vTangent2 * rd.m_fDensity * (float)i + vTangent1 * maxExtent1;
  }
}

void ezGridRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  for (auto it = batch.GetIterator<ezGridRenderData>(); it.IsValid(); ++it)
  {
    CreateGrid(*it);

    if (m_Vertices.IsEmpty())
      return;

    ezRenderContext* pRenderContext = renderViewContext.m_pRenderContext;

    pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
    pRenderContext->BindShader(m_hShader);

    ezUInt32 uiNumLineVertices = m_Vertices.GetCount();
    const GridVertex* pLineData = m_Vertices.GetData();

    while (uiNumLineVertices > 0)
    {
      ezGALBufferHandle hBuffer = m_VertexBuffer.GetNewBuffer();
      const ezUInt32 uiNumLineVerticesInBatch = ezMath::Min<ezUInt32>(uiNumLineVertices, s_uiLineVerticesPerBatch);
      EZ_ASSERT_DEBUG(uiNumLineVerticesInBatch % 2 == 0, "Vertex count must be a multiple of 2.");

      pRenderContext->GetCommandEncoder()->UpdateBuffer(hBuffer, 0, ezMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);

      pRenderContext->BindMeshBuffer(ezMakeArrayPtr(&hBuffer, 1), ezGALBufferHandle(), m_VertexAttributes, ezGALPrimitiveTopology::Lines, uiNumLineVerticesInBatch / 2);
      pRenderContext->DrawMeshBuffer().IgnoreResult();

      uiNumLineVertices -= uiNumLineVerticesInBatch;
      pLineData += s_uiLineVerticesPerBatch;
    }
  }
}

float AdjustGridDensity(float fDensity, ezUInt32 uiWindowWidth, float fOrthoDimX, ezUInt32 uiMinPixelsDist)
{
  ezInt32 iFactor = 1;
  float fNewDensity = fDensity;

  while (true)
  {
    const float stepsAtDensity = fOrthoDimX / fNewDensity;
    const float minPixelsAtDensity = stepsAtDensity * uiMinPixelsDist;

    if (minPixelsAtDensity < uiWindowWidth)
      break;

    iFactor *= 10;
    fNewDensity = fDensity * iFactor;
  }

  return fNewDensity;
}

void ezEditorGridExtractor::Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData)
{
  if (m_pSceneContext == nullptr || m_pSceneContext->GetGridDensity() == 0.0f)
    return;

  const ezCamera* cam = view.GetCamera();
  float fDensity = m_pSceneContext->GetGridDensity();

  ezGridRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezGridRenderData>(nullptr);
  pRenderData->m_GlobalBounds = ezBoundingBoxSphere::MakeInvalid();
  pRenderData->m_bOrthoMode = cam->IsOrthographic();
  pRenderData->m_bGlobal = m_pSceneContext->IsGridInGlobalSpace();

  if (cam->IsOrthographic())
  {
    const float fAspectRatio = view.GetViewport().width / view.GetViewport().height;
    const float fDimX = cam->GetDimensionX(fAspectRatio) * 0.5f;
    const float fDimY = cam->GetDimensionY(fAspectRatio) * 0.5f;

    fDensity = AdjustGridDensity(fDensity, (ezUInt32)view.GetViewport().width, fDimX, 10);
    pRenderData->m_fDensity = fDensity;

    pRenderData->m_GlobalTransform.SetIdentity();
    pRenderData->m_GlobalTransform.m_vPosition = cam->GetCenterDirForwards() * cam->GetFarPlane() * 0.9f;

    ezMat3 mRot;
    mRot.SetColumn(0, cam->GetCenterDirRight());
    mRot.SetColumn(1, cam->GetCenterDirUp());
    mRot.SetColumn(2, cam->GetCenterDirForwards());
    pRenderData->m_GlobalTransform.m_qRotation = ezQuat::MakeFromMat3(mRot);

    const ezVec3 vBottomLeft = cam->GetCenterPosition() - cam->GetCenterDirRight() * fDimX - cam->GetCenterDirUp() * fDimY;
    const ezVec3 vTopRight = cam->GetCenterPosition() + cam->GetCenterDirRight() * fDimX + cam->GetCenterDirUp() * fDimY;

    ezPlane plane1, plane2;
    plane1 = ezPlane::MakeFromNormalAndPoint(cam->GetCenterDirRight(), ezVec3(0));
    plane2 = ezPlane::MakeFromNormalAndPoint(cam->GetCenterDirUp(), ezVec3(0));

    const float fFirstDist1 = plane1.GetDistanceTo(vBottomLeft) - fDensity;
    const float fLastDist1 = plane1.GetDistanceTo(vTopRight) + fDensity;

    const float fFirstDist2 = plane2.GetDistanceTo(vBottomLeft) - fDensity;
    const float fLastDist2 = plane2.GetDistanceTo(vTopRight) + fDensity;


    ezVec3& val = pRenderData->m_GlobalTransform.m_vPosition;
    val.x = ezMath::RoundToMultiple(val.x, pRenderData->m_fDensity);
    val.y = ezMath::RoundToMultiple(val.y, pRenderData->m_fDensity);
    val.z = ezMath::RoundToMultiple(val.z, pRenderData->m_fDensity);

    pRenderData->m_iFirstLine1 = (ezInt32)ezMath::Trunc(fFirstDist1 / fDensity);
    pRenderData->m_iLastLine1 = (ezInt32)ezMath::Trunc(fLastDist1 / fDensity);
    pRenderData->m_iFirstLine2 = (ezInt32)ezMath::Trunc(fFirstDist2 / fDensity);
    pRenderData->m_iLastLine2 = (ezInt32)ezMath::Trunc(fLastDist2 / fDensity);
  }
  else
  {
    pRenderData->m_GlobalTransform = m_pSceneContext->GetGridTransform();

    // grid is disabled
    if (pRenderData->m_GlobalTransform.m_vScale.IsZero(0.001f))
      return;

    pRenderData->m_fDensity = fDensity;

    const ezInt32 iNumLines = 50;
    pRenderData->m_iFirstLine1 = -iNumLines;
    pRenderData->m_iLastLine1 = iNumLines;
    pRenderData->m_iFirstLine2 = -iNumLines;
    pRenderData->m_iLastLine2 = iNumLines;
  }

  ref_extractedRenderData.AddRenderData(pRenderData, ezDefaultRenderDataCategories::SimpleTransparent);
}

ezResult ezEditorGridExtractor::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return EZ_SUCCESS;
}


ezResult ezEditorGridExtractor::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  return EZ_SUCCESS;
}
