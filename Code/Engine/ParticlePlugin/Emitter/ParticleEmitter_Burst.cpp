#include <PCH.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Burst.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Foundation/Time/Clock.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitterFactory_Burst, 1, ezRTTIDefaultAllocator<ezParticleEmitterFactory_Burst>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Duration", m_Duration),
    EZ_MEMBER_PROPERTY("StartDelay", m_StartDelay),

    EZ_MEMBER_PROPERTY("MinSpawnCount", m_uiSpawnCountMin)->AddAttributes(new ezDefaultValueAttribute(10)),
    EZ_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange),
    EZ_MEMBER_PROPERTY("SpawnCountScaleParam", m_sSpawnCountScaleParameter),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitter_Burst, 1, ezRTTIDefaultAllocator<ezParticleEmitter_Burst>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleEmitterFactory_Burst::ezParticleEmitterFactory_Burst()
{
  m_uiSpawnCountMin = 10;
  m_uiSpawnCountRange = 0;
}

const ezRTTI* ezParticleEmitterFactory_Burst::GetEmitterType() const
{
  return ezGetStaticRTTI<ezParticleEmitter_Burst>();
}

void ezParticleEmitterFactory_Burst::CopyEmitterProperties(ezParticleEmitter* pEmitter0) const
{
  ezParticleEmitter_Burst* pEmitter = static_cast<ezParticleEmitter_Burst*>(pEmitter0);

  pEmitter->m_Duration = m_Duration;
  pEmitter->m_StartDelay = m_StartDelay;

  pEmitter->m_uiSpawnCountMin = m_uiSpawnCountMin;
  pEmitter->m_uiSpawnCountRange = m_uiSpawnCountRange;
  pEmitter->m_sSpawnCountScaleParameter = ezTempHashedString(m_sSpawnCountScaleParameter.GetData());
}

enum class EmitterBurstVersion
{
  Version_1 = 1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void ezParticleEmitterFactory_Burst::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)EmitterBurstVersion::Version_Current;
  stream << uiVersion;

  // Version 1
  stream << m_Duration;
  stream << m_StartDelay;
  stream << m_uiSpawnCountMin;
  stream << m_uiSpawnCountRange;
  stream << m_sSpawnCountScaleParameter;
}

void ezParticleEmitterFactory_Burst::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)EmitterBurstVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_Duration;
  stream >> m_StartDelay;
  stream >> m_uiSpawnCountMin;
  stream >> m_uiSpawnCountRange;
  stream >> m_sSpawnCountScaleParameter;
}

void ezParticleEmitter_Burst::OnFinalize()
{
  float fSpawnFactor = 1.0f;

  const float spawnCountScale = ezMath::Clamp(GetOwnerEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f, 10.0f);
  fSpawnFactor *= spawnCountScale;

  ezRandom& rng = GetRNG();

  m_uiSpawnCountLeft = (ezUInt32)(rng.IntInRange(m_uiSpawnCountMin, 1 + m_uiSpawnCountRange) * fSpawnFactor);

  m_fSpawnAccu = 0;
  m_fSpawnPerSecond = 0;

  if (!m_Duration.IsZero())
  {
    m_fSpawnPerSecond = m_uiSpawnCountLeft / (float)m_Duration.GetSeconds();
  }
}

ezParticleEmitterState ezParticleEmitter_Burst::IsFinished()
{
  return (m_uiSpawnCountLeft == 0) ? ezParticleEmitterState::Finished : ezParticleEmitterState::Active;
}

ezUInt32 ezParticleEmitter_Burst::ComputeSpawnCount(const ezTime& tDiff)
{
  EZ_PROFILE("PFX: Burst - Spawn Count ");

  // delay before the emitter becomes active
  if (!m_StartDelay.IsZeroOrLess())
  {
    m_StartDelay -= tDiff;
    return 0;
  }

  ezUInt32 uiSpawn;

  if (m_Duration.IsZero())
  {
    uiSpawn = m_uiSpawnCountLeft;
    m_uiSpawnCountLeft = 0;
  }
  else
  {
    m_fSpawnAccu += (float)tDiff.GetSeconds() * m_fSpawnPerSecond;
    uiSpawn = (ezUInt32)m_fSpawnAccu;
    uiSpawn = ezMath::Min(uiSpawn, m_uiSpawnCountLeft);

    m_fSpawnAccu -= uiSpawn;
    m_uiSpawnCountLeft -= uiSpawn;
  }

  return uiSpawn;
}
