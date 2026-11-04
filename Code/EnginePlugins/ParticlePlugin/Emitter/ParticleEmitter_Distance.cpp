#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Distance.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitter_Distance, 1, ezRTTIDefaultAllocator<ezParticleEmitter_Distance>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DistanceThreshold", m_fDistanceThreshold)->AddAttributes(new ezDefaultValueAttribute(0.1f), new ezClampValueAttribute(0.01f, 100.0f)),
    EZ_MEMBER_PROPERTY("MinSpawnCount", m_uiSpawnCountMin)->AddAttributes(new ezDefaultValueAttribute(1)),
    EZ_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange),
    EZ_MEMBER_PROPERTY("SpawnCountScaleParam", m_sSpawnCountScaleParameter),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleEmitter_Distance::ezParticleEmitter_Distance() = default;

void ezParticleEmitter_Distance::QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const
{
  out_uiMaxParticlesAbs = 0;
  out_uiMaxParticlesPerSecond = (m_uiSpawnCountMin + m_uiSpawnCountRange) * 10; // assume that this won't fire more than 10 times per second

  // TODO: consider to scale by m_sSpawnCountScaleParameter
}

enum class EmitterDistanceVersion
{
  Version_1 = 1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void ezParticleEmitter_Distance::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)EmitterDistanceVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_fDistanceThreshold;
  inout_stream << m_uiSpawnCountMin;
  inout_stream << m_uiSpawnCountRange;
  inout_stream << m_sSpawnCountScaleParameter;
}

void ezParticleEmitter_Distance::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)EmitterDistanceVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fDistanceThreshold;
  inout_stream >> m_uiSpawnCountMin;
  inout_stream >> m_uiSpawnCountRange;
  inout_stream >> m_sSpawnCountScaleParameter;
}

bool ezParticleEmitter_Distance::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleEmitter_Distance::OnFinalize(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  // do not use the System transform, because then this would not work with local space simulation
  data.m_vLastSpawnPosition = ctx.GetEffect()->GetTransform().m_vPosition;
  data.m_bFirstUpdate = true;

  if (ctx.GetEffect()->IsSharedEffect())
  {
    ezLog::Warning("Particle emitters of type 'Distance' do not work for shared particle effect instances.");
  }
}

ezParticleEmitterState ezParticleEmitter_Distance::IsFinished(const ezParticleModuleContext& ctx) const
{
  return ezParticleEmitterState::Active;
}

ezUInt32 ezParticleEmitter_Distance::ComputeSpawnCount(const ezParticleModuleContext& ctx, const ezTime& diff) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  const ezVec3 vCurPos = ctx.GetEffect()->GetTransform().m_vPosition;

  if ((data.m_vLastSpawnPosition - vCurPos).GetLengthSquared() < ezMath::Square(m_fDistanceThreshold))
    return 0;

  data.m_vLastSpawnPosition = vCurPos;

  if (data.m_bFirstUpdate)
  {
    data.m_bFirstUpdate = false;
    return 0;
  }

  float fSpawnFactor = 1.0f;

  const float spawnCountScale = ezMath::Max(ctx.GetEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  fSpawnFactor *= spawnCountScale;

  const float fMultiplier = ctx.m_pSystem->GetSpawnCountMultiplier();
  const ezUInt32 uiSpawnCountRange = (ezUInt32)(m_uiSpawnCountRange * fMultiplier);

  ezUInt32 uiSpawn = (ezUInt32)(m_uiSpawnCountMin * fMultiplier);

  if (uiSpawnCountRange > 0)
    uiSpawn += ctx.GetRNG().UIntInRange(uiSpawnCountRange);

  uiSpawn = static_cast<ezUInt32>((float)uiSpawn * fSpawnFactor);

  return uiSpawn;
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_Distance);
