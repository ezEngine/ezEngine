#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Burst.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitter_Burst, 1, ezRTTIDefaultAllocator<ezParticleEmitter_Burst>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Duration", m_Duration),
    EZ_MEMBER_PROPERTY("StartDelay", m_StartDelay),

    EZ_MEMBER_PROPERTY("MinSpawnCount", m_uiSpawnCountMin)->AddAttributes(new ezDefaultValueAttribute(10)),
    EZ_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange),
    EZ_MEMBER_PROPERTY("SpawnCountScaleParam", m_sSpawnCountScaleParameter),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleEmitter_Burst::ezParticleEmitter_Burst()
{
  m_uiSpawnCountMin = 10;
  m_uiSpawnCountRange = 0;
}

void ezParticleEmitter_Burst::QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const
{
  out_uiMaxParticlesAbs = m_uiSpawnCountMin + m_uiSpawnCountRange;
  out_uiMaxParticlesPerSecond = 0;

  // TODO: consider to scale by m_sSpawnCountScaleParameter
}

enum class EmitterBurstVersion
{
  Version_1 = 1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void ezParticleEmitter_Burst::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)EmitterBurstVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_Duration;
  inout_stream << m_StartDelay;
  inout_stream << m_uiSpawnCountMin;
  inout_stream << m_uiSpawnCountRange;
  inout_stream << m_sSpawnCountScaleParameter;
}

void ezParticleEmitter_Burst::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)EmitterBurstVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_Duration;
  inout_stream >> m_StartDelay;
  inout_stream >> m_uiSpawnCountMin;
  inout_stream >> m_uiSpawnCountRange;
  inout_stream >> m_sSpawnCountScaleParameter;
}

bool ezParticleEmitter_Burst::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleEmitter_Burst::OnFinalize(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  data.m_StartDelay = m_StartDelay;

  float fSpawnFactor = 1.0f;

  const float spawnCountScale = ezMath::Max(ctx.GetEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  fSpawnFactor *= spawnCountScale;

  const float fMultiplier = ctx.m_pSystem->GetSpawnCountMultiplier();
  const ezUInt32 uiSpawnCountMin = (ezUInt32)(m_uiSpawnCountMin * fMultiplier);
  const ezUInt32 uiSpawnCountRange = (ezUInt32)(m_uiSpawnCountRange * fMultiplier);

  data.m_uiSpawnCountLeft = (ezUInt32)(ctx.GetRNG().IntMinMax(uiSpawnCountMin, uiSpawnCountMin + uiSpawnCountRange) * fSpawnFactor);

  data.m_fSpawnAccu = 0;
  data.m_fSpawnPerSecond = 0;

  if (!m_Duration.IsZero())
  {
    data.m_fSpawnPerSecond = data.m_uiSpawnCountLeft / (float)m_Duration.GetSeconds();
  }
}

ezParticleEmitterState ezParticleEmitter_Burst::IsFinished(const ezParticleModuleContext& ctx) const
{
  return (ctx.GetInstanceData<InstanceData>().m_uiSpawnCountLeft == 0) ? ezParticleEmitterState::Finished : ezParticleEmitterState::Active;
}

ezUInt32 ezParticleEmitter_Burst::ComputeSpawnCount(const ezParticleModuleContext& ctx, const ezTime& diff) const
{
  EZ_PROFILE_SCOPE("PFX: Burst - Spawn Count ");

  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  // delay before the emitter becomes active
  if (data.m_StartDelay.IsPositive())
  {
    data.m_StartDelay -= diff;
    return 0;
  }

  ezUInt32 uiSpawn = 0;

  if (m_Duration.IsZero())
  {
    uiSpawn = data.m_uiSpawnCountLeft;
    data.m_uiSpawnCountLeft = 0;
  }
  else
  {
    data.m_fSpawnAccu += (float)diff.GetSeconds() * data.m_fSpawnPerSecond;
    uiSpawn = (ezUInt32)data.m_fSpawnAccu;
    uiSpawn = ezMath::Min(uiSpawn, data.m_uiSpawnCountLeft);

    data.m_fSpawnAccu -= uiSpawn;
    data.m_uiSpawnCountLeft -= uiSpawn;
  }

  return uiSpawn;
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_Burst);
