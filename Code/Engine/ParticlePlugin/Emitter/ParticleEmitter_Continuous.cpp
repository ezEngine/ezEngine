#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Continuous.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Foundation/Time/Clock.h>
#include <Core/World/World.h>
#include <GameEngine/Curves/Curve1DResource.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitterFactory_Continuous, 1, ezRTTIDefaultAllocator<ezParticleEmitterFactory_Continuous>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Duration", m_Duration),
    EZ_MEMBER_PROPERTY("StartDelay", m_StartDelay),

    EZ_MEMBER_PROPERTY("MinSpawnCount", m_uiSpawnCountMin)->AddAttributes(new ezDefaultValueAttribute(1)),
    EZ_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange),

    EZ_MEMBER_PROPERTY("Interval", m_SpawnInterval),

    EZ_ACCESSOR_PROPERTY("CountCurve", GetCountCurveFile, SetCountCurveFile)->AddAttributes(new ezAssetBrowserAttribute("Curve1D")),
    EZ_MEMBER_PROPERTY("CurveDuration", m_CurveDuration)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(10.0))),
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

  m_CurveDuration = ezTime::Seconds(10.0);
}


const ezRTTI* ezParticleEmitterFactory_Continuous::GetEmitterType() const
{
  return ezGetStaticRTTI<ezParticleEmitter_Continuous>();
}

void ezParticleEmitterFactory_Continuous::CopyEmitterProperties(ezParticleEmitter* pEmitter0) const
{
  ezParticleEmitter_Continuous* pEmitter = static_cast<ezParticleEmitter_Continuous*>(pEmitter0);

  pEmitter->m_Duration = m_Duration;
  pEmitter->m_StartDelay = m_StartDelay;

  pEmitter->m_uiSpawnCountMin = m_uiSpawnCountMin;
  pEmitter->m_uiSpawnCountRange = m_uiSpawnCountRange;

  pEmitter->m_SpawnInterval = m_SpawnInterval;

  pEmitter->m_hCountCurve = m_hCountCurve;
  pEmitter->m_CurveDuration = ezMath::Max(m_CurveDuration, ezTime::Seconds(1.0));
}

enum class EmitterContinuousVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  Version_3,
  Version_4, // added emitter start delay

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void ezParticleEmitterFactory_Continuous::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)EmitterContinuousVersion::Version_Current;
  stream << uiVersion;

  // Version 3
  stream << m_Duration;
  // Version 4
  stream << m_StartDelay;

  // Version 1
  stream << m_uiSpawnCountMin;
  stream << m_uiSpawnCountRange;
  stream << m_SpawnInterval.m_Value;
  stream << m_SpawnInterval.m_fVariance;

  // Version 2
  stream << m_hCountCurve;
  stream << m_CurveDuration;
}

void ezParticleEmitterFactory_Continuous::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)EmitterContinuousVersion::Version_Current, "Invalid version {0}", uiVersion);

  if (uiVersion >= 3)
  {
    stream >> m_Duration;
  }

  if (uiVersion >= 4)
  {
    stream >> m_StartDelay;
  }

  stream >> m_uiSpawnCountMin;
  stream >> m_uiSpawnCountRange;
  stream >> m_SpawnInterval.m_Value;
  stream >> m_SpawnInterval.m_fVariance;

  if (uiVersion >= 2)
  {
    stream >> m_hCountCurve;
    stream >> m_CurveDuration;
  }
}


void ezParticleEmitterFactory_Continuous::SetCountCurveFile(const char* szFile)
{
  ezCurve1DResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezCurve1DResource>(szFile);
  }

  m_hCountCurve = hResource;
}

const char* ezParticleEmitterFactory_Continuous::GetCountCurveFile() const
{
  if (!m_hCountCurve.IsValid())
    return "";

  return m_hCountCurve.GetResourceID();
}


void ezParticleEmitter_Continuous::AfterPropertiesConfigured(bool bFirstTime)
{
}


ezParticleEmitterState ezParticleEmitter_Continuous::IsFinished()
{
  if (m_Duration == ezTime())
    return ezParticleEmitterState::Active;

  return (m_RunningTime >= m_StartDelay + m_Duration) ? ezParticleEmitterState::Finished : ezParticleEmitterState::Active;
}

ezUInt32 ezParticleEmitter_Continuous::ComputeSpawnCount(const ezTime& tDiff)
{
  m_RunningTime += tDiff;

  // delay before the emitter becomes active
  if (m_StartDelay > ezTime::Seconds(0))
  {
    if (m_RunningTime < m_StartDelay)
      return 0;
  }

  m_NextSpawn -= tDiff;
  m_CountCurveTime += tDiff;

  if (m_NextSpawn > ezTime::Seconds(0))
    return 0;

  float fSpawnFactor = 1.0f;

  if (m_hCountCurve.IsValid())
  {
    ezResourceLock<ezCurve1DResource> pCurve(m_hCountCurve, ezResourceAcquireMode::NoFallback);

    if (!pCurve->GetDescriptor().m_Curves.IsEmpty())
    {
      while (m_CountCurveTime > m_CurveDuration)
        m_CountCurveTime -= m_CurveDuration;

      const auto& curve = pCurve->GetDescriptor().m_Curves[0];

      const float normPos = (float)(m_CountCurveTime.GetSeconds() / m_CurveDuration.GetSeconds());
      const float evalPos = curve.ConvertNormalizedPos(normPos);

      fSpawnFactor = ezMath::Max(0.0f, curve.Evaluate(evalPos));
    }
  }

  ezRandom& rng = GetRNG();

  const ezUInt32 uiSpawn = (ezUInt32)(rng.IntInRange(m_uiSpawnCountMin, 1 + m_uiSpawnCountRange) * fSpawnFactor);

  const ezTime interval = ezTime::Seconds(rng.DoubleVariance(m_SpawnInterval.m_Value.GetSeconds(), m_SpawnInterval.m_fVariance));

  // we ignore the fact that with lower update frequencies (bad framerate), the actual interval will become larger and the effect
  // might visibly change
  m_NextSpawn = interval;

  return uiSpawn;
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_Continuous);

