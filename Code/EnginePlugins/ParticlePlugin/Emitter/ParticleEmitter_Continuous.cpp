#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Continuous.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitterFactory_Continuous, 1, ezRTTIDefaultAllocator<ezParticleEmitterFactory_Continuous>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("StartDelay", m_StartDelay),

    EZ_MEMBER_PROPERTY("SpawnCountPerSec", m_uiSpawnCountPerSec)->AddAttributes(new ezDefaultValueAttribute(10)),
    EZ_MEMBER_PROPERTY("SpawnCountPerSecRange", m_uiSpawnCountPerSecRange),
    EZ_MEMBER_PROPERTY("SpawnCountScaleParam", m_sSpawnCountScaleParameter),

    EZ_RESOURCE_MEMBER_PROPERTY("CountCurve", m_hCountCurve)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
    EZ_MEMBER_PROPERTY("CurveDuration", m_CurveDuration)->AddAttributes(new ezDefaultValueAttribute(ezTime::MakeFromSeconds(10.0))),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitter_Continuous, 1, ezRTTIDefaultAllocator<ezParticleEmitter_Continuous>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleEmitterFactory_Continuous::ezParticleEmitterFactory_Continuous()
{
  m_uiSpawnCountPerSec = 10;
  m_uiSpawnCountPerSecRange = 0;

  m_CurveDuration = ezTime::MakeFromSeconds(10.0);
}


const ezRTTI* ezParticleEmitterFactory_Continuous::GetEmitterType() const
{
  return ezGetStaticRTTI<ezParticleEmitter_Continuous>();
}

void ezParticleEmitterFactory_Continuous::CopyEmitterProperties(ezParticleEmitter* pEmitter0, bool bFirstTime) const
{
  ezParticleEmitter_Continuous* pEmitter = static_cast<ezParticleEmitter_Continuous*>(pEmitter0);

  pEmitter->m_StartDelay = m_StartDelay;

  pEmitter->m_uiSpawnCountPerSec = (ezUInt32)(m_uiSpawnCountPerSec * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());
  pEmitter->m_uiSpawnCountPerSecRange = (ezUInt32)(m_uiSpawnCountPerSecRange * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());

  pEmitter->m_sSpawnCountScaleParameter = ezTempHashedString(m_sSpawnCountScaleParameter.GetData());

  pEmitter->m_hCountCurve = m_hCountCurve;
  pEmitter->m_CurveDuration = ezMath::Max(m_CurveDuration, ezTime::MakeFromSeconds(1.0));
}

void ezParticleEmitterFactory_Continuous::QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const
{
  out_uiMaxParticlesAbs = 0;
  out_uiMaxParticlesPerSecond = m_uiSpawnCountPerSec + (m_uiSpawnCountPerSecRange * 3 / 4); // don't be too pessimistic

  // TODO: consider to scale by m_sSpawnCountScaleParameter
}

enum class EmitterContinuousVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  Version_3,
  Version_4, // added emitter start delay
  Version_5, // added spawn count scale param
  Version_6, // removed duration, switched to particles per second

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void ezParticleEmitterFactory_Continuous::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)EmitterContinuousVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 4
  inout_stream << m_StartDelay;

  // Version 6
  inout_stream << m_uiSpawnCountPerSec;
  inout_stream << m_uiSpawnCountPerSecRange;

  // Version 2
  inout_stream << m_hCountCurve;
  inout_stream << m_CurveDuration;

  // Version 5
  inout_stream << m_sSpawnCountScaleParameter;
}

void ezParticleEmitterFactory_Continuous::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)EmitterContinuousVersion::Version_Current, "Invalid version {0}", uiVersion);

  if (uiVersion >= 3 && uiVersion < 6)
  {
    ezTime duraton;
    inout_stream >> duraton;
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_StartDelay;
  }

  inout_stream >> m_uiSpawnCountPerSec;
  inout_stream >> m_uiSpawnCountPerSecRange;

  if (uiVersion < 6)
  {
    ezVarianceTypeFloat interval;
    inout_stream >> interval.m_Value;
    inout_stream >> interval.m_fVariance;
  }

  if (uiVersion >= 2)
  {
    inout_stream >> m_hCountCurve;
    inout_stream >> m_CurveDuration;
  }

  if (uiVersion >= 5)
  {
    inout_stream >> m_sSpawnCountScaleParameter;
  }
}

void ezParticleEmitter_Continuous::OnFinalize()
{
  m_CountCurveTime = ezTime::MakeZero();
  m_fCurSpawnPerSec = (float)GetRNG().DoubleInRange(m_uiSpawnCountPerSec, m_uiSpawnCountPerSecRange);
  m_TimeSinceRandom = ezTime::MakeZero();
  m_fCurSpawnCounter = 0;
}

ezParticleEmitterState ezParticleEmitter_Continuous::IsFinished()
{
  return ezParticleEmitterState::Active;
}

ezUInt32 ezParticleEmitter_Continuous::ComputeSpawnCount(const ezTime& tDiff)
{
  EZ_PROFILE_SCOPE("PFX: Continuous - Spawn Count ");

  // delay before the emitter becomes active
  if (m_StartDelay.IsPositive())
  {
    m_StartDelay -= tDiff;
    return 0;
  }

  m_TimeSinceRandom += tDiff;
  m_CountCurveTime += tDiff;

  if (m_TimeSinceRandom >= ezTime::MakeFromMilliseconds(200))
  {
    m_TimeSinceRandom = ezTime::MakeZero();
    m_fCurSpawnPerSec = (float)GetRNG().DoubleInRange(m_uiSpawnCountPerSec, m_uiSpawnCountPerSecRange);
  }


  float fSpawnFactor = 1.0f;

  if (m_hCountCurve.IsValid())
  {
    ezResourceLock<ezCurve1DResource> pCurve(m_hCountCurve, ezResourceAcquireMode::BlockTillLoaded);

    if (!pCurve->GetDescriptor().m_Curves.IsEmpty())
    {
      while (m_CountCurveTime > m_CurveDuration)
        m_CountCurveTime -= m_CurveDuration;

      const auto& curve = pCurve->GetDescriptor().m_Curves[0];

      const double normPos = (float)(m_CountCurveTime.GetSeconds() / m_CurveDuration.GetSeconds());
      const double evalPos = curve.ConvertNormalizedPos(normPos);

      fSpawnFactor = (float)ezMath::Max(0.0, curve.Evaluate(evalPos));
    }
  }

  const float spawnCountScale = ezMath::Max(GetOwnerEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  fSpawnFactor *= spawnCountScale;


  m_fCurSpawnCounter += fSpawnFactor * m_fCurSpawnPerSec * (float)tDiff.GetSeconds();

  const ezUInt32 uiSpawn = (ezUInt32)m_fCurSpawnCounter;
  m_fCurSpawnCounter -= uiSpawn;

  return uiSpawn;
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_Continuous);
