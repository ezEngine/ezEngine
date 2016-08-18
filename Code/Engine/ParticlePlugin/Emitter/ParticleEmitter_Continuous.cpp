#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Continuous.h>
#include <CoreUtils/DataProcessing/Stream/StreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Foundation/Time/Clock.h>
#include <Core/World/World.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitterFactory_Continuous, 1, ezRTTIDefaultAllocator<ezParticleEmitterFactory_Continuous>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Min Spawn Count", m_uiSpawnCountMin)->AddAttributes(new ezDefaultValueAttribute(1)),
    EZ_MEMBER_PROPERTY("Spawn Count Range", m_uiSpawnCountRange),

    EZ_MEMBER_PROPERTY("Min Interval", m_SpawnIntervalMin)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(1.0))),
    EZ_MEMBER_PROPERTY("Interval Range", m_SpawnIntervalRange),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitter_Continuous, 1, ezRTTIDefaultAllocator<ezParticleEmitter_Continuous>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleEmitterFactory_Continuous::ezParticleEmitterFactory_Continuous()
{
  m_uiSpawnCountMin = 1;
  m_uiSpawnCountRange = 0;

  m_SpawnIntervalMin = ezTime::Seconds(0.5);
  m_SpawnIntervalRange = ezTime();
}


const ezRTTI* ezParticleEmitterFactory_Continuous::GetEmitterType() const
{
  return ezGetStaticRTTI<ezParticleEmitter_Continuous>();
}

void ezParticleEmitterFactory_Continuous::CopyEmitterProperties(ezParticleEmitter* pEmitter0) const
{
  ezParticleEmitter_Continuous* pEmitter = static_cast<ezParticleEmitter_Continuous*>(pEmitter0);

  pEmitter->m_uiSpawnCountMin = m_uiSpawnCountMin;
  pEmitter->m_uiSpawnCountRange = m_uiSpawnCountRange;

  pEmitter->m_SpawnIntervalMin = m_SpawnIntervalMin;
  pEmitter->m_SpawnIntervalRange = m_SpawnIntervalRange;
}

void ezParticleEmitterFactory_Continuous::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_uiSpawnCountMin;
  stream << m_uiSpawnCountRange;
  stream << m_SpawnIntervalMin;
  stream << m_SpawnIntervalRange;
}

void ezParticleEmitterFactory_Continuous::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= 1, "Invalid version %u", uiVersion);

  stream >> m_uiSpawnCountMin;
  stream >> m_uiSpawnCountRange;
  stream >> m_SpawnIntervalMin;
  stream >> m_SpawnIntervalRange;
}

void ezParticleEmitter_Continuous::SpawnElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  const ezUInt64 uiElementSize = m_pStreamPosition->GetElementSize();

  ezVec3* pPosition = m_pStreamPosition->GetWritableData<ezVec3>();
  ezVec3* pVelocity = m_pStreamVelocity->GetWritableData<ezVec3>();
  ezColor* pColor = m_pStreamColor->GetWritableData<ezColor>();

  ezRandom& rng = m_pOwnerSystem->GetRNG();
  const ezTransform transform = m_pOwnerSystem->GetTransform();

  for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    pPosition[i] = transform.m_vPosition;
    pVelocity[i].SetZero();// = transform.m_Rotation * ezVec3(1, 3, 10);
    pColor[i] = ezColor::White;
  }
}

ezUInt32 ezParticleEmitter_Continuous::ComputeSpawnCount()
{
  const ezTime tCurrent = m_pOwnerSystem->GetWorld()->GetClock().GetAccumulatedTime();

  if (tCurrent < m_NextSpawn)
    return 0;

  ezRandom& rng = m_pOwnerSystem->GetRNG();

  const ezUInt32 uiSpawn = rng.IntInRange(m_uiSpawnCountMin, 1 + m_uiSpawnCountRange);

  const ezTime interval = ezTime::Seconds(rng.DoubleInRange(m_SpawnIntervalMin.GetSeconds(), m_SpawnIntervalRange.GetSeconds()));

  // we ignore the fact that with lower update frequencies (bad framerate), the actual interval will become larger and the effect
  // might visibly change
  m_NextSpawn = tCurrent + interval;

  return uiSpawn;
}


