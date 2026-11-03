#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Trail/ParticleTypeTrail.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeTrail, 1, ezRTTIDefaultAllocator<ezParticleTypeTrail>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("RenderMode", ezParticleTypeRenderMode, m_RenderMode),
    EZ_ENUM_MEMBER_PROPERTY("LightingMode", ezParticleLightingMode, m_LightingMode),
    EZ_MEMBER_PROPERTY("NormalCurvature", m_fNormalCurvature)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0, 1)),
    EZ_MEMBER_PROPERTY("LightDirectionality", m_fLightDirectionality)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0, 1)),
    EZ_MEMBER_PROPERTY("UseCustomMaterial", m_bUseCustomMaterial),
    EZ_MEMBER_PROPERTY("CustomMaterial", m_sCustomMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material", "TrailParticle")),
    EZ_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_2D"), new ezDefaultValueAttribute(ezStringView("{ e00262e8-58f5-42f5-880d-569257047201 }"))),// wrap in ezStringView to prevent a memory leak report
    EZ_MEMBER_PROPERTY("Segments", m_uiMaxPoints)->AddAttributes(new ezDefaultValueAttribute(6), new ezClampValueAttribute(3, 64)),
    EZ_ENUM_MEMBER_PROPERTY("TextureAtlas", ezParticleTextureAtlasType, m_TextureAtlasType),
    EZ_ENUM_MEMBER_PROPERTY("TextureOrientation", ezParticleTextureAtlasOrientation, m_TextureAtlasOrientation),
    EZ_MEMBER_PROPERTY("NumSpritesX", m_uiNumSpritesX)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("NumSpritesY", m_uiNumSpritesY)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

enum class TypeTrailVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added render mode
  Version_3, // added texture atlas support
  Version_4, // added tint color
  Version_5, // added distortion mode
  Version_6, // added particle lighting
  Version_7, // added custom material support
  Version_8, // added texture atlas orientation

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeTrail::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)TypeTrailVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_sTexture;
  inout_stream << m_uiMaxPoints;
  inout_stream << m_UpdateDiff;
  inout_stream << m_RenderMode;

  // version 3
  inout_stream << m_TextureAtlasType;
  inout_stream << m_uiNumSpritesX;
  inout_stream << m_uiNumSpritesY;

  // version 4
  inout_stream << m_sTintColorParameter;

  // version 5
  ezString sDistortionTexture;
  float fDistortionStrength = 0;
  inout_stream << sDistortionTexture;
  inout_stream << fDistortionStrength;

  // Version 6
  inout_stream << m_LightingMode;
  inout_stream << m_fNormalCurvature;
  inout_stream << m_fLightDirectionality;

  // Version 7
  inout_stream << m_bUseCustomMaterial;
  inout_stream << m_sCustomMaterial;

  // Version 8
  inout_stream << m_TextureAtlasOrientation;
}

void ezParticleTypeTrail::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeTrailVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_sTexture;
  inout_stream >> m_uiMaxPoints;
  inout_stream >> m_UpdateDiff;

  if (uiVersion >= 2)
  {
    inout_stream >> m_RenderMode;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_TextureAtlasType;
    inout_stream >> m_uiNumSpritesX;
    inout_stream >> m_uiNumSpritesY;

    if (m_TextureAtlasType == ezParticleTextureAtlasType::None)
    {
      m_uiNumSpritesX = 1;
      m_uiNumSpritesY = 1;
    }
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_sTintColorParameter;
  }

  if (uiVersion >= 5)
  {
    ezString sDistortionTexture;
    float fDistortionStrength;
    inout_stream >> sDistortionTexture;
    inout_stream >> fDistortionStrength;
  }

  if (uiVersion >= 6)
  {
    inout_stream >> m_LightingMode;
    inout_stream >> m_fNormalCurvature;
    inout_stream >> m_fLightDirectionality;
  }

  if (uiVersion >= 7)
  {
    inout_stream >> m_bUseCustomMaterial;
    inout_stream >> m_sCustomMaterial;
  }

  if (uiVersion >= 8)
  {
    inout_stream >> m_TextureAtlasOrientation;
  }

  if (m_bUseCustomMaterial)
  {
    if (!m_sCustomMaterial.IsEmpty())
    {
      m_hCustomMaterial = ezResourceManager::LoadResource<ezMaterialResource>(m_sCustomMaterial);
    }
  }
  else if (!m_sTexture.IsEmpty())
  {
    m_hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(m_sTexture);
  }
}

//////////////////////////////////////////////////////////////////////////

bool ezParticleTypeTrail::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleTypeTrail::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  ctx.CreateStream(ezParticleStreamNames::s_sLifeTime, ezProcessingStream::DataType::Half2, &data.m_pStreamLifeTime, false);
  ctx.CreateStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4, &data.m_pStreamPosition, false);
  ctx.CreateStream(ezParticleStreamNames::s_sSize, ezProcessingStream::DataType::Half, &data.m_pStreamSize, false);
  ctx.CreateStream(ezParticleStreamNames::s_sColor, ezProcessingStream::DataType::Half4, &data.m_pStreamColor, false);
  ctx.CreateStream(ezParticleStreamNames::s_sTrailData, ezProcessingStream::DataType::Short2, &data.m_pStreamTrailData, true);

  data.m_pStreamVariation = nullptr;

  if (m_TextureAtlasType == ezParticleTextureAtlasType::RandomVariations || m_TextureAtlasType == ezParticleTextureAtlasType::RandomYAnimatedX || m_TextureAtlasType == ezParticleTextureAtlasType::RandomXAnimatedY)
  {
    ctx.CreateStream(ezParticleStreamNames::s_sVariation, ezProcessingStream::DataType::Int, &data.m_pStreamVariation, false);
  }
}

void ezParticleTypeTrail::OnFinalize(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  if (!data.m_bInitialized)
  {
    data.m_bInitialized = true;
    data.m_LastSnapshot = ctx.GetEffect()->GetTotalEffectLifeTime();
  }

  // clamp the number of points to the maximum possible count
  data.m_uiMaxPoints = ezMath::Min<ezUInt16>(m_uiMaxPoints, ComputeTrailPointBucketSize(m_uiMaxPoints));
  data.m_uiCurFirstIndex = 1;
}

void ezParticleTypeTrail::ExtractTypeRenderData(const ezParticleModuleContext& ctx, ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  EZ_PROFILE_SCOPE("PFX: Trail");

  if (!m_hTexture.IsValid() && !m_hCustomMaterial.IsValid())
    return;

  const ezUInt32 numActiveParticles = (ezUInt32)ctx.m_pSystem->GetNumActiveParticles();

  if (numActiveParticles == 0)
    return;

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if (data.m_uiLastExtractedFrame != ezRenderWorld::GetFrameCounter())
  {
    data.m_uiLastExtractedFrame = ezRenderWorld::GetFrameCounter();

    const ezColor tintColor = ctx.GetEffect()->GetColorParameter(m_sTintColorParameter, ezColor::White);

    const ezFloat16* pSize = data.m_pStreamSize->GetData<ezFloat16>();
    const ezColorLinear16f* pColor = data.m_pStreamColor->GetData<ezColorLinear16f>();
    const TrailData* pTrailData = data.m_pStreamTrailData->GetData<TrailData>();
    const ezFloat16Vec2* pLifeTime = data.m_pStreamLifeTime->GetData<ezFloat16Vec2>();
    const ezUInt32* pVariation = data.m_pStreamVariation ? data.m_pStreamVariation->GetData<ezUInt32>() : nullptr;

    const ezUInt32 uiBucketSize = ComputeTrailPointBucketSize(data.m_uiMaxPoints);

    // this will automatically be deallocated at the end of the frame
    data.m_BaseParticleData =
      EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezBaseParticleShaderData, (ezUInt32)ctx.m_pSystem->GetNumActiveParticles());
    data.m_TrailPointsShared =
      EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezVec4, (ezUInt32)ctx.m_pSystem->GetNumActiveParticles() * uiBucketSize);
    data.m_TrailParticleData =
      EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezTrailParticleShaderData, (ezUInt32)ctx.m_pSystem->GetNumActiveParticles());

    for (ezUInt32 p = 0; p < numActiveParticles; ++p)
    {
      data.m_BaseParticleData[p].Size = pSize[p];
      data.m_BaseParticleData[p].Color = pColor[p].ToLinearFloat() * tintColor;
      data.m_BaseParticleData[p].Life = pLifeTime[p].x * pLifeTime[p].y;
      data.m_BaseParticleData[p].Variation = (pVariation != nullptr) ? pVariation[p] : 0;

      data.m_TrailParticleData[p].NumPoints = pTrailData[p].m_uiNumPoints;
    }

    for (ezUInt32 p = 0; p < numActiveParticles; ++p)
    {
      const ezVec4* pTrailPositions = data.GetTrailPointsPositions(pTrailData[p].m_uiIndexForTrailPoints);

      ezVec4* pRenderPositions = &data.m_TrailPointsShared[p * uiBucketSize];

      /// \todo This loop could be done without a condition
      for (ezUInt32 i = 0; i < data.m_uiMaxPoints; ++i)
      {
        if (i > data.m_uiCurFirstIndex)
        {
          pRenderPositions[i] = pTrailPositions[data.m_uiCurFirstIndex + data.m_uiMaxPoints - i];
        }
        else
        {
          pRenderPositions[i] = pTrailPositions[data.m_uiCurFirstIndex - i];
        }
      }
    }
  }

  auto pRenderData = ref_msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezParticleTrailRenderData>(nullptr);

  if (m_hCustomMaterial.IsValid())
  {
    pRenderData->m_uiSortingKey = ComputeSortingKey(m_RenderMode, m_hCustomMaterial.GetResourceIDHash(), 0);
  }
  else
  {
    pRenderData->m_uiSortingKey = ComputeSortingKey(m_RenderMode, m_hTexture.GetResourceIDHash(), 0);
  }

  pRenderData->m_vGlobalPosition = instanceTransform.m_vPosition;

  pRenderData->m_GlobalTransform = ctx.GetEffect()->NeedsToApplyTransform() ? instanceTransform : ezTransform::MakeIdentity();
  pRenderData->m_TotalEffectLifeTime = ctx.GetEffect()->GetTotalEffectLifeTime();
  pRenderData->m_RenderMode = m_RenderMode;
  pRenderData->m_uiMaxTrailPoints = data.m_uiMaxPoints;
  pRenderData->m_hTexture = m_hTexture;
  pRenderData->m_BaseParticleData = data.m_BaseParticleData;
  pRenderData->m_TrailParticleData = data.m_TrailParticleData;
  pRenderData->m_TrailPointsShared = data.m_TrailPointsShared;
  pRenderData->m_fSnapshotFraction = data.m_fSnapshotFraction;
  pRenderData->m_LightingMode = m_LightingMode;
  pRenderData->m_TextureAtlasOrientation = m_TextureAtlasOrientation;
  pRenderData->m_fNormalCurvature = m_fNormalCurvature;
  pRenderData->m_fLightDirectionality = m_fLightDirectionality;
  pRenderData->m_hCustomMaterial = m_hCustomMaterial;

  pRenderData->m_uiNumVariationsX = 1;
  pRenderData->m_uiNumVariationsY = 1;
  pRenderData->m_uiNumFlipbookAnimationsX = 1;
  pRenderData->m_uiNumFlipbookAnimationsY = 1;

  switch (m_TextureAtlasType)
  {
    case ezParticleTextureAtlasType::None:
      break;

    case ezParticleTextureAtlasType::RandomVariations:
      pRenderData->m_uiNumVariationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumVariationsY = m_uiNumSpritesY;
      break;

    case ezParticleTextureAtlasType::FlipbookAnimation:
      pRenderData->m_uiNumFlipbookAnimationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumFlipbookAnimationsY = m_uiNumSpritesY;
      break;

    case ezParticleTextureAtlasType::RandomYAnimatedX:
      pRenderData->m_uiNumFlipbookAnimationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumVariationsY = m_uiNumSpritesY;
      break;

    case ezParticleTextureAtlasType::RandomXAnimatedY:
      pRenderData->m_uiNumVariationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumFlipbookAnimationsY = m_uiNumSpritesY;
      break;
  }

  ref_msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitTransparent, ezRenderData::Caching::Never);
}

void ezParticleTypeTrail::InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  TrailData* pTrailData = data.m_pStreamTrailData->GetWritableData<TrailData>() + uiStartIndex;

  const ezVec4* pPosData = data.m_pStreamPosition->GetData<ezVec4>() + uiStartIndex;

  const ezUInt32 uiPrevIndex = (data.m_uiCurFirstIndex > 0) ? (data.m_uiCurFirstIndex - 1) : (data.m_uiMaxPoints - 1);
  const ezUInt32 uiPrevIndex2 = (uiPrevIndex > 0) ? (uiPrevIndex - 1) : (data.m_uiMaxPoints - 1);

  for (ezUInt64 i = 0; i < uiNumElements; ++i)
  {
    const ezVec4 vStartPos = pPosData[i];

    TrailData& td = pTrailData[i];
    td.m_uiNumPoints = 2;
    td.m_uiIndexForTrailPoints = data.GetIndexForTrailPoints();

    ezVec4* pPos = data.GetTrailPointsPositions(td.m_uiIndexForTrailPoints);
    pPos[data.m_uiCurFirstIndex] = vStartPos;
    pPos[uiPrevIndex] = vStartPos;
    pPos[uiPrevIndex2] = vStartPos;
  }
}


void ezParticleTypeTrail::Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  // fixed 25 FPS for the update rate
  const ezTime tUpdateDiff = ezTime::MakeFromSeconds(1.0 / 25.0);

  const ezTime tNow = ctx.GetEffect()->GetTotalEffectLifeTime();

  TrailData* pTrailData = data.m_pStreamTrailData->GetWritableData<TrailData>();
  const ezVec4* pPosData = data.m_pStreamPosition->GetData<ezVec4>();

  if (tNow - data.m_LastSnapshot >= tUpdateDiff)
  {
    data.m_LastSnapshot = tNow;

    data.m_uiCurFirstIndex = (data.m_uiCurFirstIndex + 1) == data.m_uiMaxPoints ? 0 : (data.m_uiCurFirstIndex + 1);

    for (ezUInt64 i = 0; i < uiNumElements; ++i)
    {
      pTrailData[i].m_uiNumPoints = ezMath::Min<ezUInt16>(pTrailData[i].m_uiNumPoints + 1, data.m_uiMaxPoints);
    }
  }

  data.m_fSnapshotFraction = 1.0f - (float)((tNow - data.m_LastSnapshot).GetSeconds() / tUpdateDiff.GetSeconds());

  for (ezUInt64 i = 0; i < uiNumElements; ++i)
  {
    ezVec4* pPositions = data.GetTrailPointsPositions(pTrailData[i].m_uiIndexForTrailPoints);
    pPositions[data.m_uiCurFirstIndex] = pPosData[i];
  }
}

ezUInt16 ezParticleTypeTrail::InstanceData::GetIndexForTrailPoints()
{
  ezUInt16 res = 0;

  if (!m_FreeTrailData.IsEmpty())
  {
    res = m_FreeTrailData.PeekBack();
    m_FreeTrailData.PopBack();
  }
  else
  {
    // expand the proper array

    // if (m_uiMaxPoints > 32)
    //{
    res = static_cast<ezUInt16>(m_TrailPoints64.GetCount());
    m_TrailPoints64.ExpandAndGetRef();
    //}
    // else if (m_uiMaxPoints > 16)
    //{
    //  res = m_TrailData32.GetCount() & 0xFFFF;
    //  m_TrailData64.ExpandAndGetRef();
    //}
    // else if (m_uiMaxPoints > 8)
    //{
    //  res = m_TrailData16.GetCount() & 0xFFFF;
    //  m_TrailData64.ExpandAndGetRef();
    //}
    // else
    //{
    //  res = m_TrailData8.GetCount() & 0xFFFF;
    //  m_TrailData8.ExpandAndGetRef();
    //}
  }

  return res;
}

ezVec4* ezParticleTypeTrail::InstanceData::GetTrailPointsPositions(ezUInt32 uiIndex)
{
  // if (m_uiMaxPoints > 32)
  {
    return &m_TrailPoints64[uiIndex].Positions[0];
  }
  // else if (m_uiMaxPoints > 16)
  //{
  //  return &m_TrailPoints32[uiIndex].Positions[0];
  //}
  // else if (m_uiMaxPoints > 8)
  //{
  //  return &m_TrailPoints16[uiIndex].Positions[0];
  //}
  // else
  //{
  //  return &m_TrailPoints8[uiIndex].Positions[0];
  //}
}

const ezVec4* ezParticleTypeTrail::InstanceData::GetTrailPointsPositions(ezUInt32 uiIndex) const
{
  // if (m_uiMaxPoints > 32)
  {
    return &m_TrailPoints64[uiIndex].Positions[0];
  }
  // else if (m_uiMaxPoints > 16)
  //{
  //  return &m_TrailPoints32[uiIndex].Positions[0];
  //}
  // else if (m_uiMaxPoints > 8)
  //{
  //  return &m_TrailPoints16[uiIndex].Positions[0];
  //}
  // else
  //{
  //  return &m_TrailPoints8[uiIndex].Positions[0];
  //}
}


ezUInt16 ezParticleTypeTrail::ComputeTrailPointBucketSize(ezUInt16 uiMaxTrailPoints)
{
  if (uiMaxTrailPoints > 32)
  {
    return 64;
  }
  else if (uiMaxTrailPoints > 16)
  {
    return 32;
  }
  else if (uiMaxTrailPoints > 8)
  {
    return 16;
  }
  else
  {
    return 8;
  }
}

void ezParticleTypeTrail::OnParticleDeath(const ezParticleModuleContext& ctx, ezUInt64 uiElementIndex) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  const TrailData* pTrailData = data.m_pStreamTrailData->GetData<TrailData>();

  // return the trail data to the list of free elements
  data.m_FreeTrailData.PushBack(pTrailData[uiElementIndex].m_uiIndexForTrailPoints);
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Trail_ParticleTypeTrail);
