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
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitter_Continuous, 1, ezRTTIDefaultAllocator<ezParticleEmitter_Continuous>)
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
// clang-format on

ezParticleEmitter_Continuous::ezParticleEmitter_Continuous()
{
  m_uiSpawnCountPerSec = 10;
  m_uiSpawnCountPerSecRange = 0;

  m_CurveDuration = ezTime::MakeFromSeconds(10.0);
}


void ezParticleEmitter_Continuous::QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const
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


void ezParticleEmitter_Continuous::Save(ezStreamWriter& inout_stream) const
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

void ezParticleEmitter_Continuous::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
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

bool ezParticleEmitter_Continuous::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

float ezParticleEmitter_Continuous::ComputeRandomSpawnPerSec(const ezParticleModuleContext& ctx) const
{
  const float fMultiplier = ctx.m_pSystem->GetSpawnCountMultiplier();
  const ezUInt32 uiSpawnCountPerSec = (ezUInt32)(m_uiSpawnCountPerSec * fMultiplier);
  const ezUInt32 uiSpawnCountPerSecRange = (ezUInt32)(m_uiSpawnCountPerSecRange * fMultiplier);

  return static_cast<float>(ctx.GetRNG().DoubleMinMax(uiSpawnCountPerSec, uiSpawnCountPerSec + uiSpawnCountPerSecRange));
}

void ezParticleEmitter_Continuous::OnFinalize(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  data.m_StartDelay = m_StartDelay;
  data.m_CountCurveTime = ezTime::MakeZero();
  data.m_fCurSpawnPerSec = ComputeRandomSpawnPerSec(ctx);
  data.m_TimeSinceRandom = ezTime::MakeZero();
  data.m_fCurSpawnCounter = 1; // make sure to always spawn at least one particle right away in the first frame
}

ezParticleEmitterState ezParticleEmitter_Continuous::IsFinished(const ezParticleModuleContext& ctx) const
{
  return ezParticleEmitterState::Active;
}

ezUInt32 ezParticleEmitter_Continuous::ComputeSpawnCount(const ezParticleModuleContext& ctx, const ezTime& tDiff) const
{
  EZ_PROFILE_SCOPE("PFX: Continuous - Spawn Count ");

  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  // delay before the emitter becomes active
  if (data.m_StartDelay.IsPositive())
  {
    data.m_StartDelay -= tDiff;
    return 0;
  }

  data.m_TimeSinceRandom += tDiff;
  data.m_CountCurveTime += tDiff;

  if (data.m_TimeSinceRandom >= ezTime::MakeFromMilliseconds(200))
  {
    data.m_TimeSinceRandom = ezTime::MakeZero();
    data.m_fCurSpawnPerSec = ComputeRandomSpawnPerSec(ctx);
  }


  float fSpawnFactor = 1.0f;

  if (m_hCountCurve.IsValid())
  {
    ezResourceLock<ezCurve1DResource> pCurve(m_hCountCurve, ezResourceAcquireMode::BlockTillLoaded);

    if (!pCurve->GetDescriptor().m_Curves.IsEmpty())
    {
      const ezTime curveDuration = ezMath::Max(m_CurveDuration, ezTime::MakeFromSeconds(1.0));

      while (data.m_CountCurveTime > curveDuration)
        data.m_CountCurveTime -= curveDuration;

      const auto& curve = pCurve->GetDescriptor().m_Curves[0];

      const double normPos = (float)(data.m_CountCurveTime.GetSeconds() / curveDuration.GetSeconds());
      const double evalPos = curve.ConvertNormalizedPos(normPos);

      fSpawnFactor = (float)ezMath::Max(0.0, curve.Evaluate(evalPos));
    }
  }

  const float spawnCountScale = ezMath::Max(ctx.GetEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  fSpawnFactor *= spawnCountScale;

  data.m_fCurSpawnCounter += fSpawnFactor * data.m_fCurSpawnPerSec * (float)tDiff.GetSeconds();

  const ezUInt32 uiSpawn = (ezUInt32)data.m_fCurSpawnCounter;
  data.m_fCurSpawnCounter -= uiSpawn;

  return uiSpawn;
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_Continuous);
