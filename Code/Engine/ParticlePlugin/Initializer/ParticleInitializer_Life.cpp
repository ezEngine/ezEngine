#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_Life.h>
#include <CoreUtils/DataProcessing/Stream/StreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_Life, 1, ezRTTIDefaultAllocator<ezParticleInitializerFactory_Life>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Min Life Time", m_MinLifeTime)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(1))),
    EZ_MEMBER_PROPERTY("Life Time Range", m_LifeTimeRange)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(1))),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_Life, 1, ezRTTIDefaultAllocator<ezParticleInitializer_Life>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleInitializerFactory_Life::ezParticleInitializerFactory_Life()
{
  m_MinLifeTime = ezTime::Seconds(1.0f);
  m_LifeTimeRange = ezTime::Seconds(1.0f);
}

const ezRTTI* ezParticleInitializerFactory_Life::GetInitializerType() const
{
  return ezGetStaticRTTI<ezParticleInitializer_Life>();
}

void ezParticleInitializerFactory_Life::CopyInitializerProperties(ezParticleInitializer* pInitializer0) const
{
  ezParticleInitializer_Life* pInitializer = static_cast<ezParticleInitializer_Life*>(pInitializer0);

  pInitializer->m_MinLifeTime = ezMath::Max(m_MinLifeTime, ezTime::Milliseconds(100)); // make sure min life time cannot be 0
  pInitializer->m_LifeTimeRange = m_LifeTimeRange;
}

void ezParticleInitializerFactory_Life::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_MinLifeTime;
  stream << m_LifeTimeRange;
}

void ezParticleInitializerFactory_Life::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_MinLifeTime;
  stream >> m_LifeTimeRange;

}


void ezParticleInitializer_Life::CreateRequiredStreams()
{
  CreateStream("LifeTime", ezProcessingStream::DataType::Float2, &m_pStreamLifeTime);
}

void ezParticleInitializer_Life::SpawnElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  ezVec2* pLifeTime = m_pStreamLifeTime->GetWritableData<ezVec2>();

  if (m_LifeTimeRange == ezTime()) // 0 range
  {
    const float tLifeTime = (float)m_MinLifeTime.GetSeconds();

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pLifeTime[i].Set(tLifeTime, tLifeTime);
    }
  }
  else // random range
  {
    ezRandom& rng = GetRNG();

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const float tLifeTime = (float)rng.DoubleInRange(m_MinLifeTime.GetSeconds(), m_LifeTimeRange.GetSeconds());

      pLifeTime[i].Set(tLifeTime, tLifeTime);
    }
  }
}
