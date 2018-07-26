#include <PCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Trail/ParticleTypeTrail.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeTrailFactory, 1, ezRTTIDefaultAllocator<ezParticleTypeTrailFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("RenderMode", ezParticleTypeRenderMode, m_RenderMode),
    EZ_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D"), new ezDefaultValueAttribute(ezStringView("{ e00262e8-58f5-42f5-880d-569257047201 }"))),// wrap in ezStringView to prevent a memory leak report
    EZ_MEMBER_PROPERTY("Segments", m_uiMaxPoints)->AddAttributes(new ezDefaultValueAttribute(6), new ezClampValueAttribute(3, 64)),
    EZ_ENUM_MEMBER_PROPERTY("TextureAtlas", ezParticleTextureAtlasType, m_TextureAtlasType),
    EZ_MEMBER_PROPERTY("NumSpritesX", m_uiNumSpritesX)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("NumSpritesY", m_uiNumSpritesY)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    // EZ_MEMBER_PROPERTY("UpdateTime", m_UpdateDiff)->AddAttributes(new ezDefaultValueAttribute(ezTime::Milliseconds(50)), new
    // ezClampValueAttribute(ezTime::Milliseconds(20), ezTime::Milliseconds(300))),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeTrail, 1, ezRTTIDefaultAllocator<ezParticleTypeTrail>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const ezRTTI* ezParticleTypeTrailFactory::GetTypeType() const
{
  return ezGetStaticRTTI<ezParticleTypeTrail>();
}

void ezParticleTypeTrailFactory::CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const
{
  ezParticleTypeTrail* pType = static_cast<ezParticleTypeTrail*>(pObject);

  pType->m_RenderMode = m_RenderMode;
  pType->m_uiMaxPoints = m_uiMaxPoints;
  pType->m_hTexture.Invalidate();
  pType->m_TextureAtlasType = m_TextureAtlasType;
  pType->m_uiNumSpritesX = m_uiNumSpritesX;
  pType->m_uiNumSpritesY = m_uiNumSpritesY;

  // fixed 25 FPS for the update rate
  pType->m_UpdateDiff = ezTime::Seconds(1.0 / 25.0); // m_UpdateDiff;

  if (!m_sTexture.IsEmpty())
    pType->m_hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(m_sTexture);


  if (bFirstTime)
  {
    pType->GetOwnerSystem()->AddParticleDeathEventHandler(ezMakeDelegate(&ezParticleTypeTrail::OnParticleDeath, pType));

    pType->m_LastSnapshot = pType->GetOwnerSystem()->GetWorld()->GetClock().GetAccumulatedTime();
  }

  // m_uiMaxPoints = ezMath::Min<ezUInt16>(8, m_uiMaxPoints);

  // clamp the number of points to the maximum possible count
  pType->m_uiMaxPoints = ezMath::Min<ezUInt16>(pType->m_uiMaxPoints, pType->ComputeTrailPointBucketSize(pType->m_uiMaxPoints));

  pType->m_uiCurFirstIndex = pType->m_uiMaxPoints - 1;
  pType->m_uiCurFirstIndex = 1;
}

enum class TypeTrailVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added render mode
  Version_3, // added texture atlas support

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeTrailFactory::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)TypeTrailVersion::Version_Current;
  stream << uiVersion;

  stream << m_sTexture;
  stream << m_uiMaxPoints;
  stream << m_UpdateDiff;
  stream << m_RenderMode;

  // version 3
  stream << m_TextureAtlasType;
  stream << m_uiNumSpritesX;
  stream << m_uiNumSpritesY;
}

void ezParticleTypeTrailFactory::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeTrailVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_sTexture;
  stream >> m_uiMaxPoints;
  stream >> m_UpdateDiff;

  if (uiVersion >= 2)
  {
    stream >> m_RenderMode;
  }

  if (uiVersion >= 3)
  {
    stream >> m_TextureAtlasType;
    stream >> m_uiNumSpritesX;
    stream >> m_uiNumSpritesY;

    if (m_TextureAtlasType == ezParticleTextureAtlasType::None)
    {
      m_uiNumSpritesX = 1;
      m_uiNumSpritesY = 1;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

ezParticleTypeTrail::ezParticleTypeTrail()
{
  m_uiCurFirstIndex = 0;
}

ezParticleTypeTrail::~ezParticleTypeTrail()
{
  GetOwnerSystem()->RemoveParticleDeathEventHandler(ezMakeDelegate(&ezParticleTypeTrail::OnParticleDeath, this));
}

void ezParticleTypeTrail::CreateRequiredStreams()
{
  CreateStream("LifeTime", ezProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", ezProcessingStream::DataType::Half, &m_pStreamSize, false);
  CreateStream("Color", ezProcessingStream::DataType::Half4, &m_pStreamColor, false);
  CreateStream("TrailData", ezProcessingStream::DataType::Short2, &m_pStreamTrailData, true);

  m_pStreamVariation = nullptr;

  if (m_TextureAtlasType == ezParticleTextureAtlasType::RandomVariations ||
      m_TextureAtlasType == ezParticleTextureAtlasType::RandomYAnimatedX)
  {
    CreateStream("Variation", ezProcessingStream::DataType::Int, &m_pStreamVariation, false);
  }
}

void ezParticleTypeTrail::ExtractTypeRenderData(const ezView& view, ezExtractedRenderData& extractedRenderData,
                                                const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const
{
  EZ_PROFILE("PFX: Trail");

  if (!m_hTexture.IsValid())
    return;

  const ezUInt32 numActiveParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numActiveParticles == 0)
    return;

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if (m_uiLastExtractedFrame != uiExtractedFrame)
  {
    m_uiLastExtractedFrame = uiExtractedFrame;

    const ezFloat16* pSize = m_pStreamSize->GetData<ezFloat16>();
    const ezColorLinear16f* pColor = m_pStreamColor->GetData<ezColorLinear16f>();
    const TrailData* pTrailData = m_pStreamTrailData->GetData<TrailData>();
    const ezFloat16Vec2* pLifeTime = m_pStreamLifeTime->GetData<ezFloat16Vec2>();
    const ezUInt32* pVariation = m_pStreamVariation ? m_pStreamVariation->GetData<ezUInt32>() : nullptr;

    const ezUInt32 uiBucketSize = ComputeTrailPointBucketSize(m_uiMaxPoints);

    // this will automatically be deallocated at the end of the frame
    m_BaseParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezBaseParticleShaderData,
                                      (ezUInt32)GetOwnerSystem()->GetNumActiveParticles());
    m_TrailPointsShared =
        EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezVec4, (ezUInt32)GetOwnerSystem()->GetNumActiveParticles() * uiBucketSize);
    m_TrailParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezTrailParticleShaderData,
                                       (ezUInt32)GetOwnerSystem()->GetNumActiveParticles());

    for (ezUInt32 p = 0; p < numActiveParticles; ++p)
    {
      m_BaseParticleData[p].Size = pSize[p];
      m_BaseParticleData[p].Color = pColor[p].ToLinearFloat();
      m_BaseParticleData[p].Life = pLifeTime[p].x * pLifeTime[p].y;
      m_BaseParticleData[p].Variation = (pVariation != nullptr) ? pVariation[p] : 0;

      m_TrailParticleData[p].NumPoints = pTrailData[p].m_uiNumPoints;
    }

    for (ezUInt32 p = 0; p < numActiveParticles; ++p)
    {
      const ezVec4* pTrailPositions = GetTrailPointsPositions(pTrailData[p].m_uiIndexForTrailPoints);

      ezVec4* pRenderPositions = &m_TrailPointsShared[p * uiBucketSize];

      /// \todo This loop could be done without a condition
      for (ezUInt32 i = 0; i < m_uiMaxPoints; ++i)
      {
        if (i > m_uiCurFirstIndex)
        {
          pRenderPositions[i] = pTrailPositions[m_uiCurFirstIndex - i + m_uiMaxPoints];
        }
        else
        {
          pRenderPositions[i] = pTrailPositions[m_uiCurFirstIndex - i];
        }
      }
    }
  }

  /// \todo Is this batch ID correct?
  const ezUInt32 uiBatchId = m_hTexture.GetResourceIDHash() + m_uiMaxPoints;
  auto pRenderData = ezCreateRenderDataForThisFrame<ezParticleTrailRenderData>(nullptr, uiBatchId);

  pRenderData->m_bApplyObjectTransform = GetOwnerEffect()->NeedsToApplyTransform();
  pRenderData->m_RenderMode = m_RenderMode;
  pRenderData->m_GlobalTransform = instanceTransform;
  pRenderData->m_uiMaxTrailPoints = m_uiMaxPoints;
  pRenderData->m_hTexture = m_hTexture;
  pRenderData->m_BaseParticleData = m_BaseParticleData;
  pRenderData->m_TrailParticleData = m_TrailParticleData;
  pRenderData->m_TrailPointsShared = m_TrailPointsShared;
  pRenderData->m_fSnapshotFraction = m_fSnapshotFraction;

  pRenderData->m_uiNumVariationsX = 1;
  pRenderData->m_uiNumVariationsY = 1;
  pRenderData->m_uiNumFlipbookAnimationsX = 1;
  pRenderData->m_uiNumFlipbookAnimationsY = 1;

  switch (m_TextureAtlasType)
  {
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
  }

  const ezUInt32 uiSortingKey = ComputeSortingKey(m_RenderMode);
  extractedRenderData.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitTransparent, uiSortingKey);
}

void ezParticleTypeTrail::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  TrailData* pTrailData = m_pStreamTrailData->GetWritableData<TrailData>();

  const ezVec4* pPosData = m_pStreamPosition->GetData<ezVec4>();

  const ezUInt32 uiPrevIndex = (m_uiCurFirstIndex > 0) ? (m_uiCurFirstIndex - 1) : (m_uiMaxPoints - 1);
  const ezUInt32 uiPrevIndex2 = (uiPrevIndex > 0) ? (uiPrevIndex - 1) : (m_uiMaxPoints - 1);

  for (ezUInt64 i = 0; i < uiNumElements; ++i)
  {
    const ezVec4 vStartPos = pPosData[uiStartIndex + i];

    TrailData& td = pTrailData[uiStartIndex + i];
    td.m_uiNumPoints = 2;
    td.m_uiIndexForTrailPoints = GetIndexForTrailPoints();

    ezVec4* pPos = GetTrailPointsPositions(td.m_uiIndexForTrailPoints);
    pPos[m_uiCurFirstIndex] = vStartPos;
    pPos[uiPrevIndex] = vStartPos;
    pPos[uiPrevIndex2] = vStartPos;
  }
}


void ezParticleTypeTrail::Process(ezUInt64 uiNumElements)
{
  const ezTime tNow = GetOwnerSystem()->GetWorld()->GetClock().GetAccumulatedTime();

  TrailData* pTrailData = m_pStreamTrailData->GetWritableData<TrailData>();
  const ezVec4* pPosData = m_pStreamPosition->GetData<ezVec4>();

  if (tNow - m_LastSnapshot >= m_UpdateDiff)
  {
    m_LastSnapshot = tNow;

    /// \todo Get around the modulo
    m_uiCurFirstIndex = (m_uiCurFirstIndex + 1) % m_uiMaxPoints;

    for (ezUInt64 i = 0; i < uiNumElements; ++i)
    {
      pTrailData[i].m_uiNumPoints = ezMath::Min<ezUInt16>(pTrailData[i].m_uiNumPoints + 1, m_uiMaxPoints);
    }
  }

  m_fSnapshotFraction = 1.0f - (float)((tNow - m_LastSnapshot).GetSeconds() / m_UpdateDiff.GetSeconds());

  for (ezUInt64 i = 0; i < uiNumElements; ++i)
  {
    ezVec4* pPositions = GetTrailPointsPositions(pTrailData[i].m_uiIndexForTrailPoints);
    pPositions[m_uiCurFirstIndex] = pPosData[i];
  }
}

ezUInt16 ezParticleTypeTrail::GetIndexForTrailPoints()
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
    res = m_TrailPoints64.GetCount();
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

ezVec4* ezParticleTypeTrail::GetTrailPointsPositions(ezUInt32 index)
{
  // if (m_uiMaxPoints > 32)
  {
    return &m_TrailPoints64[index].Positions[0];
  }
  // else if (m_uiMaxPoints > 16)
  //{
  //  return &m_TrailPoints32[index].Positions[0];
  //}
  // else if (m_uiMaxPoints > 8)
  //{
  //  return &m_TrailPoints16[index].Positions[0];
  //}
  // else
  //{
  //  return &m_TrailPoints8[index].Positions[0];
  //}
}

const ezVec4* ezParticleTypeTrail::GetTrailPointsPositions(ezUInt32 index) const
{
  // if (m_uiMaxPoints > 32)
  {
    return &m_TrailPoints64[index].Positions[0];
  }
  // else if (m_uiMaxPoints > 16)
  //{
  //  return &m_TrailPoints32[index].Positions[0];
  //}
  // else if (m_uiMaxPoints > 8)
  //{
  //  return &m_TrailPoints16[index].Positions[0];
  //}
  // else
  //{
  //  return &m_TrailPoints8[index].Positions[0];
  //}
}


ezUInt32 ezParticleTypeTrail::ComputeTrailPointBucketSize(ezUInt32 uiMaxTrailPoints)
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

void ezParticleTypeTrail::OnParticleDeath(const ezStreamGroupElementRemovedEvent& e)
{
  const TrailData* pTrailData = m_pStreamTrailData->GetData<TrailData>();

  // return the trail data to the list of free elements
  m_FreeTrailData.PushBack(pTrailData[e.m_uiElementIndex].m_uiIndexForTrailPoints);
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Trail_ParticleTypeTrail);
