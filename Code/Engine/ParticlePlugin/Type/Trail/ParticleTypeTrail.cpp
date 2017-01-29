#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Type/Trail/ParticleTypeTrail.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <Core/World/GameObject.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/World/World.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeTrailFactory, 1, ezRTTIDefaultAllocator<ezParticleTypeTrailFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_MEMBER_PROPERTY("Segments", m_uiMaxPoints)->AddAttributes(new ezDefaultValueAttribute(6), new ezClampValueAttribute(3, 64)),
    EZ_MEMBER_PROPERTY("UpdateTime", m_UpdateDiff)->AddAttributes(new ezDefaultValueAttribute(ezTime::Milliseconds(50)), new ezClampValueAttribute(ezTime::Milliseconds(20), ezTime::Milliseconds(300))),
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

  pType->m_uiMaxPoints = m_uiMaxPoints;
  pType->m_hTexture.Invalidate();
  pType->m_UpdateDiff = m_UpdateDiff;

  if (!m_sTexture.IsEmpty())
    pType->m_hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(m_sTexture);
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
  stream << m_uiMaxPoints;
  stream << m_UpdateDiff;
}

void ezParticleTypeTrailFactory::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeTrailVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_sTexture;
  stream >> m_uiMaxPoints;
  stream >> m_UpdateDiff;
}


ezParticleTypeTrail::ezParticleTypeTrail()
{
  m_uiCurFirstIndex = 0;
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
  }

  //if (m_uiMaxPoints > 32)
  m_uiMaxPoints = TRAIL_POINTS;
  m_uiMaxPointsMask = TRAIL_POINTS - 1;
  //else if (m_uiMaxPoints > 16)
  //  m_uiMaxPointsMask = 31;
  //else if (m_uiMaxPoints > 8)
  //  m_uiMaxPointsMask = 15;
  //else
  //  m_uiMaxPointsMask = 7;

  //m_uiCurFirstIndex = (m_uiCurLastIndex + m_uiMaxPoints - 1) & m_uiMaxPointsMask;
  m_uiCurFirstIndex = TRAIL_POINTS - 1;
  m_uiCurFirstIndex = 1;
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

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if (m_uiLastExtractedFrame != uiExtractedFrame)
  {
    m_uiLastExtractedFrame = uiExtractedFrame;

    const ezUInt32 uiBucketSize = TRAIL_POINTS;

    const float* pSize = m_pStreamSize->GetData<float>();
    const ezColor* pColor = m_pStreamColor->GetData<ezColor>();
    const TrailData* pTrailData = m_pStreamTrailData->GetData<TrailData>();

    // this will automatically be deallocated at the end of the frame
    m_ParticleDataShared = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezTrailParticleData, (ezUInt32)GetOwnerSystem()->GetNumActiveParticles());
    m_TrailPointsShared = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezTrailParticlePointsData, (ezUInt32)GetOwnerSystem()->GetNumActiveParticles());

    for (ezUInt32 p = 0; p < numActiveParticles; ++p)
    {
      m_ParticleDataShared[p].Size = pSize[p];
      m_ParticleDataShared[p].Color = pColor[p];
      m_ParticleDataShared[p].NumPoints = pTrailData[p].m_uiNumPoints;
      m_ParticleDataShared[p].SnapshotFraction = m_fSnapshotFraction;
    }

    for (ezUInt32 p = 0; p < numActiveParticles; ++p)
    {
      const ezVec4* pTrailPositions = GetTrailPointsPositions(pTrailData[p].m_uiIndexForTrailPoints);

      ezTrailParticlePointsData& renderPositions = m_TrailPointsShared[p];

      //ezUInt32 seg = m_uiCurLastIndex;
      //for (ezUInt32 i = m_uiMaxPoints; i > 0; --i)
      //{
      //  renderPositions.Positions[i - 1] = pTrailPositions[seg];

      //  seg = (seg + 1) & m_uiMaxPointsMask;
      //}

      /// \todo This loop could be done without a condition
      for (ezUInt32 i = 0; i < m_uiMaxPoints; ++i)
      {
        if (i > m_uiCurFirstIndex)
        {
          renderPositions.Positions[i] = pTrailPositions[m_uiCurFirstIndex - i + m_uiMaxPoints];
        }
        else
        {
          renderPositions.Positions[i] = pTrailPositions[m_uiCurFirstIndex - i];
        }
      }
    }
  }

  /// \todo Is this batch ID correct?
  const ezUInt32 uiBatchId = m_hTexture.GetResourceIDHash();
  auto pRenderData = ezCreateRenderDataForThisFrame<ezParticleTrailRenderData>(nullptr, uiBatchId);

  pRenderData->m_GlobalTransform = instanceTransform;
  pRenderData->m_uiMaxTrailPoints = TRAIL_POINTS;
  pRenderData->m_hTexture = m_hTexture;
  pRenderData->m_ParticleDataShared = m_ParticleDataShared;
  pRenderData->m_TrailPointsShared = m_TrailPointsShared;

  /// \todo Generate a proper sorting key?
  const ezUInt32 uiSortingKey = 0;
  pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::SimpleTransparent, uiSortingKey);
}

void ezParticleTypeTrail::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  TrailData* pTrailData = m_pStreamTrailData->GetWritableData<TrailData>();

  const ezVec3* pPosData = m_pStreamPosition->GetData<ezVec3>();

  const ezUInt32 uiPrevIndex = (m_uiCurFirstIndex > 0) ? (m_uiCurFirstIndex - 1) : m_uiMaxPointsMask;
  const ezUInt32 uiPrevIndex2 = (uiPrevIndex > 0) ? (uiPrevIndex - 1) : m_uiMaxPointsMask;

  for (ezUInt64 i = 0; i < uiNumElements; ++i)
  {
    const ezVec4 vStartPos = pPosData[uiStartIndex + i].GetAsPositionVec4();

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
  const ezVec3* pPosData = m_pStreamPosition->GetData<ezVec3>();

  if (tNow - m_LastSnapshot >= m_UpdateDiff)
  {
    m_LastSnapshot = tNow;

    m_uiCurFirstIndex = (m_uiCurFirstIndex + 1) & m_uiMaxPointsMask;

    for (ezUInt64 i = 0; i < uiNumElements; ++i)
    {
      pTrailData[i].m_uiNumPoints = ezMath::Min<ezUInt16>(pTrailData[i].m_uiNumPoints + 1, m_uiMaxPoints);
    }
  }

  m_fSnapshotFraction = 1.0f - (float)((tNow - m_LastSnapshot).GetSeconds() / m_UpdateDiff.GetSeconds());

  for (ezUInt64 i = 0; i < uiNumElements; ++i)
  {
    ezVec4* pPositions = GetTrailPointsPositions(pTrailData[i].m_uiIndexForTrailPoints);
    pPositions[m_uiCurFirstIndex] = pPosData[i].GetAsPositionVec4();
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

    //if (m_uiMaxPoints > 32)
    //{
    res = m_TrailPoints.GetCount();
    m_TrailPoints.ExpandAndGetRef();
    //}
    //else if (m_uiMaxPoints > 16)
    //{
    //  res = m_TrailData32.GetCount() & 0xFFFF;
    //  m_TrailData64.ExpandAndGetRef();
    //}
    //else if (m_uiMaxPoints > 8)
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

ezVec4* ezParticleTypeTrail::GetTrailPointsPositions(ezUInt32 index)
{
  //if (m_uiMaxPoints > 32)
  //{
  return &m_TrailPoints[index].Positions[0];
  //}
  //else if (m_uiMaxPoints > 16)
  //{
  //  return &m_TrailData32[index].m_Positions[0];
  //}
  //else if (m_uiMaxPoints > 8)
  //{
  //  return &m_TrailData16[index].m_Positions[0];
  //}
  //else
  //{
  //  return &m_TrailData8[index].m_Positions[0];
  //}
}

const ezVec4* ezParticleTypeTrail::GetTrailPointsPositions(ezUInt32 index) const
{
  //if (m_uiMaxPoints > 32)
  //{
  return &m_TrailPoints[index].Positions[0];
  //}
  //else if (m_uiMaxPoints > 16)
  //{
  //  return &m_TrailData32[index].m_Positions[0];
  //}
  //else if (m_uiMaxPoints > 8)
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
  m_FreeTrailData.PushBack(pTrailData[e.m_uiElementIndex].m_uiIndexForTrailPoints);
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Trail_ParticleTypeTrail);

