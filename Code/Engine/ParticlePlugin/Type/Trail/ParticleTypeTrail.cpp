#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Type/Trail/ParticleTypeTrail.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Textures/TextureResource.h>
#include <Core/World/GameObject.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/World/World.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeTrailFactory, 1, ezRTTIDefaultAllocator<ezParticleTypeTrailFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_MEMBER_PROPERTY("Segments", m_uiMaxSegments)->AddAttributes(new ezDefaultValueAttribute(6), new ezClampValueAttribute(3, 64)),
    EZ_MEMBER_PROPERTY("Update Time", m_UpdateDiff)->AddAttributes(new ezDefaultValueAttribute(ezTime::Milliseconds(50)), new ezClampValueAttribute(ezTime::Milliseconds(20), ezTime::Milliseconds(300))),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeTrail, 1, ezRTTIDefaultAllocator<ezParticleTypeTrail>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleTypeTrailFactory::GetTypeType() const
{
  return ezGetStaticRTTI<ezParticleTypeTrail>();
}


void ezParticleTypeTrailFactory::CopyTypeProperties(ezParticleType* pObject) const
{
  ezParticleTypeTrail* pType = static_cast<ezParticleTypeTrail*>(pObject);

  pType->m_uiMaxSegments = m_uiMaxSegments;
  pType->m_hTexture.Invalidate();
  pType->m_UpdateDiff = m_UpdateDiff;

  if (!m_sTexture.IsEmpty())
    pType->m_hTexture = ezResourceManager::LoadResource<ezTextureResource>(m_sTexture);
}

enum class TypeTrailVersion
{
  Version_0 = 0,
  Version_1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeTrailFactory::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)TypeTrailVersion::Version_Current;
  stream << uiVersion;

  stream << m_sTexture;
  stream << m_uiMaxSegments;
  stream << m_UpdateDiff;
}

void ezParticleTypeTrailFactory::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeTrailVersion::Version_Current, "Invalid version %u", uiVersion);

  stream >> m_sTexture;
  stream >> m_uiMaxSegments;
  stream >> m_UpdateDiff;
}


ezParticleTypeTrail::ezParticleTypeTrail()
{
  m_uiCurFirstIndex = 0;
  m_uiCurLastIndex = 0;
}

ezParticleTypeTrail::~ezParticleTypeTrail()
{
  GetOwnerSystem()->RemoveParticleDeathEventHandler(ezMakeDelegate(&ezParticleTypeTrail::OnParticleDeath, this));
}

void ezParticleTypeTrail::AfterPropertiesConfigured(bool bFirstTime)
{
  if (bFirstTime)
  {
    GetOwnerSystem()->AddParticleDeathEventHandler(ezMakeDelegate(&ezParticleTypeTrail::OnParticleDeath, this));

    m_LastSnapshot = GetOwnerSystem()->GetWorld()->GetClock().GetAccumulatedTime();

    m_uiCurLastIndex = 0;
  }

  //if (m_uiMaxSegments > 32)
  m_uiMaxSegmentsMask = 63;
  //else if (m_uiMaxSegments > 16)
  //  m_uiMaxSegmentsMask = 31;
  //else if (m_uiMaxSegments > 8)
  //  m_uiMaxSegmentsMask = 15;
  //else
  //  m_uiMaxSegmentsMask = 7;

  m_uiCurFirstIndex = (m_uiCurLastIndex + m_uiMaxSegments - 1) & m_uiMaxSegmentsMask;
}

void ezParticleTypeTrail::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float3, &m_pStreamPosition, false);
  CreateStream("Size", ezProcessingStream::DataType::Float, &m_pStreamSize, false);
  CreateStream("Color", ezProcessingStream::DataType::Float4, &m_pStreamColor, false);
  CreateStream("TrailData", ezProcessingStream::DataType::Int, &m_pStreamTrailData, true);
}

void ezParticleTypeTrail::ExtractTypeRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const
{
  if (!m_hTexture.IsValid())
    return;

  const ezUInt32 numActiveParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numActiveParticles == 0)
    return;

  const ezTime tCur = GetOwnerSystem()->GetWorld()->GetClock().GetAccumulatedTime();

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if (m_uiLastExtractedFrame != uiExtractedFrame)
  {
    m_uiLastExtractedFrame = uiExtractedFrame;

    const ezUInt32 uiBucketSize = GetMaxSegmentBucketSize();

    const ezVec3* pPosition = m_pStreamPosition->GetData<ezVec3>();
    const float* pSize = m_pStreamSize->GetData<float>();
    const ezColor* pColor = m_pStreamColor->GetData<ezColor>();
    const TrailData* pTrailData = m_pStreamTrailData->GetData<TrailData>();

    if (m_GpuData == nullptr)
    {
      m_GpuData = EZ_DEFAULT_NEW(ezTrailParticleDataContainer);
      m_GpuData->m_Content.SetCountUninitialized((ezUInt32)GetOwnerSystem()->GetMaxParticles());
    }

    if (m_SegmentGpuData == nullptr)
    {
      m_SegmentGpuData = EZ_DEFAULT_NEW(ezTrailParticleSegmentDataContainer);
      m_SegmentGpuData->m_Content.SetCountUninitialized((ezUInt32)(uiBucketSize * sizeof(ezVec3) * GetOwnerSystem()->GetMaxParticles()));
    }

    ezTrailParticleData* TempData = m_GpuData->m_Content.GetData();

    for (ezUInt32 p = 0; p < numActiveParticles; ++p)
    {
      TempData[p].Size = pSize[p];
      TempData[p].Color = pColor[p];
      TempData[p].NumSegments = pTrailData[p].m_uiNumSegments;
    }

    for (ezUInt32 p = 0; p < numActiveParticles; ++p)
    {
      const ezVec3* pPositions = GetTrailDataPositions(pTrailData[p].m_uiTrailDataIndex);

      const ezUInt32 uiSegmentOffset = uiBucketSize * sizeof(ezVec3) * p;
      ezVec3* pSegmentPositions = reinterpret_cast<ezVec3*>(&m_SegmentGpuData->m_Content[uiSegmentOffset]);

      ezUInt32 seg = m_uiCurLastIndex;
      for (ezUInt32 i = m_uiMaxSegments; i > 0; --i)
      {
        pSegmentPositions[i - 1] = pPositions[seg];

        seg = (seg + 1) & m_uiMaxSegmentsMask;
      }
    }
  }

  /// \todo Is this batch ID correct?
  const ezUInt32 uiBatchId = m_hTexture.GetResourceIDHash();
  auto pRenderData = ezCreateRenderDataForThisFrame<ezParticleTrailRenderData>(nullptr, uiBatchId);

  pRenderData->m_GlobalTransform = instanceTransform;
  pRenderData->m_uiNumParticles = numActiveParticles;
  pRenderData->m_uiMaxSegmentBucketSize = GetMaxSegmentBucketSize();
  pRenderData->m_hTexture = m_hTexture;
  pRenderData->m_GpuData = m_GpuData;
  pRenderData->m_SegmentGpuData = m_SegmentGpuData;

  /// \todo Generate a proper sorting key?
  const ezUInt32 uiSortingKey = 0;
  pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::SimpleTransparent, uiSortingKey);
}

void ezParticleTypeTrail::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  TrailData* pTrailData = m_pStreamTrailData->GetWritableData<TrailData>();

  const ezVec3* pPosData = m_pStreamPosition->GetData<ezVec3>();

  const ezUInt32 uiPrevIndex = (m_uiCurFirstIndex > 0) ? (m_uiCurFirstIndex - 1) : m_uiMaxSegmentsMask;
  const ezUInt32 uiPrevIndex2 = (uiPrevIndex > 0) ? (uiPrevIndex - 1) : m_uiMaxSegmentsMask;

  for (ezUInt64 i = 0; i < uiNumElements; ++i)
  {
    const ezVec3 vStartPos = pPosData[uiStartIndex + i];

    TrailData& td = pTrailData[uiStartIndex + i];
    td.m_uiNumSegments = 2;
    td.m_uiTrailDataIndex = GetNewTrailDataIndex();

    ezVec3* pPos = GetTrailDataPositions(td.m_uiTrailDataIndex);
    pPos[uiPrevIndex2] = vStartPos;
    pPos[uiPrevIndex] = vStartPos;
    pPos[m_uiCurFirstIndex] = vStartPos;
  }
}


void ezParticleTypeTrail::Process(ezUInt64 uiNumElements)
{
  const ezTime tNow = GetOwnerSystem()->GetWorld()->GetClock().GetAccumulatedTime();

  TrailData* pTrailData = m_pStreamTrailData->GetWritableData<TrailData>();
  const ezVec3* pPosData = m_pStreamPosition->GetData<ezVec3>();

  if (tNow - m_LastSnapshot >= m_UpdateDiff)
  {
    m_LastSnapshot = tNow;

    m_uiCurFirstIndex = (m_uiCurFirstIndex + 1) & m_uiMaxSegmentsMask;
    m_uiCurLastIndex = (m_uiCurLastIndex + 1) & m_uiMaxSegmentsMask;

    for (ezUInt64 i = 0; i < uiNumElements; ++i)
    {
      pTrailData[i].m_uiNumSegments = ezMath::Min<ezUInt16>(pTrailData[i].m_uiNumSegments + 1, m_uiMaxSegments);
    }
  }

  for (ezUInt64 i = 0; i < uiNumElements; ++i)
  {
    ezVec3* pPositions = GetTrailDataPositions(pTrailData[i].m_uiTrailDataIndex);
    pPositions[m_uiCurFirstIndex] = pPosData[i];
  }
}

ezUInt16 ezParticleTypeTrail::GetNewTrailDataIndex()
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

    //if (m_uiMaxSegments > 32)
    //{
    res = m_TrailData64.GetCount();
    m_TrailData64.ExpandAndGetRef();
    //}
    //else if (m_uiMaxSegments > 16)
    //{
    //  res = m_TrailData32.GetCount() & 0xFFFF;
    //  m_TrailData64.ExpandAndGetRef();
    //}
    //else if (m_uiMaxSegments > 8)
    //{
    //  res = m_TrailData16.GetCount() & 0xFFFF;
    //  m_TrailData64.ExpandAndGetRef();
    //}
    //else
    //{
    //  res = m_TrailData8.GetCount() & 0xFFFF;
    //  m_TrailData8.ExpandAndGetRef();
    //}
  }

  return res;
}

ezVec3* ezParticleTypeTrail::GetTrailDataPositions(ezUInt32 index)
{
  //if (m_uiMaxSegments > 32)
  //{
  return &m_TrailData64[index].m_Positions[0];
  //}
  //else if (m_uiMaxSegments > 16)
  //{
  //  return &m_TrailData32[index].m_Positions[0];
  //}
  //else if (m_uiMaxSegments > 8)
  //{
  //  return &m_TrailData16[index].m_Positions[0];
  //}
  //else
  //{
  //  return &m_TrailData8[index].m_Positions[0];
  //}
}

const ezVec3* ezParticleTypeTrail::GetTrailDataPositions(ezUInt32 index) const
{
  //if (m_uiMaxSegments > 32)
  //{
  return &m_TrailData64[index].m_Positions[0];
  //}
  //else if (m_uiMaxSegments > 16)
  //{
  //  return &m_TrailData32[index].m_Positions[0];
  //}
  //else if (m_uiMaxSegments > 8)
  //{
  //  return &m_TrailData16[index].m_Positions[0];
  //}
  //else
  //{
  //  return &m_TrailData8[index].m_Positions[0];
  //}
}

void ezParticleTypeTrail::OnParticleDeath(const ezStreamGroupElementRemovedEvent& e)
{
  const TrailData* pTrailData = m_pStreamTrailData->GetData<TrailData>();

  // return the trail data to the list of free elements
  m_FreeTrailData.PushBack(pTrailData[e.m_uiElementIndex].m_uiTrailDataIndex);
}

ezUInt32 ezParticleTypeTrail::GetMaxSegmentBucketSize() const
{
  //if (m_uiMaxSegments > 32)
  //{
    return 64;
  //}
  //else if (m_uiMaxSegments > 16)
  //{
  //  return 32;
  //}
  //else if (m_uiMaxSegments > 8)
  //{
  //  return 16;
  //}
  //else
  //{
  //  return 8;
  //}
}
