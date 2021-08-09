#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Math/Color16f.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Point/ParticleTypePoint.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypePointFactory, 1, ezRTTIDefaultAllocator<ezParticleTypePointFactory>)
{
  //EZ_BEGIN_ATTRIBUTES
  //{
  //  new ezHiddenAttribute()
  //}
  //EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypePoint, 1, ezRTTIDefaultAllocator<ezParticleTypePoint>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const ezRTTI* ezParticleTypePointFactory::GetTypeType() const
{
  return ezGetStaticRTTI<ezParticleTypePoint>();
}

void ezParticleTypePointFactory::CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const
{
  // ezParticleTypePoint* pType = static_cast<ezParticleTypePoint*>(pObject);
}

enum class TypePointVersion
{
  Version_0 = 0,


  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypePointFactory::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)TypePointVersion::Version_Current;
  stream << uiVersion;
}

void ezParticleTypePointFactory::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypePointVersion::Version_Current, "Invalid version {0}", uiVersion);
}

void ezParticleTypePoint::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Color", ezProcessingStream::DataType::Half4, &m_pStreamColor, false);
}

void ezParticleTypePoint::ExtractTypeRenderData(ezMsgExtractRenderData& msg, const ezTransform& instanceTransform) const
{
  EZ_PROFILE_SCOPE("PFX: Point");

  const ezUInt32 numParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if (m_uiLastExtractedFrame != ezRenderWorld::GetFrameCounter())
  {
    m_uiLastExtractedFrame = ezRenderWorld::GetFrameCounter();

    const ezVec4* pPosition = m_pStreamPosition->GetData<ezVec4>();
    const ezColorLinear16f* pColor = m_pStreamColor->GetData<ezColorLinear16f>();

    // this will automatically be deallocated at the end of the frame
    m_BaseParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezBaseParticleShaderData, numParticles);
    m_BillboardParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezBillboardQuadParticleShaderData, numParticles);

    for (ezUInt32 p = 0; p < numParticles; ++p)
    {
      m_BaseParticleData[p].Color = pColor[p].ToLinearFloat();
      m_BillboardParticleData[p].Position = pPosition[p].GetAsVec3();
    }
  }

  auto pRenderData = ezCreateRenderDataForThisFrame<ezParticlePointRenderData>(nullptr);

  pRenderData->m_bApplyObjectTransform = GetOwnerEffect()->NeedsToApplyTransform();
  pRenderData->m_GlobalTransform = instanceTransform;
  pRenderData->m_TotalEffectLifeTime = GetOwnerEffect()->GetTotalEffectLifeTime();
  pRenderData->m_BaseParticleData = m_BaseParticleData;
  pRenderData->m_BillboardParticleData = m_BillboardParticleData;

  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitTransparent, ezRenderData::Caching::Never);
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Point_ParticleTypePoint);
