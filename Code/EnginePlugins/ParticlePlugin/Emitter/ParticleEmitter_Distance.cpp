#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
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
    EZ_MEMBER_PROPERTY("MaxParticles", m_uiMaxParticles),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleEmitter_Distance::ezParticleEmitter_Distance()
{
  // InitializeElements() moves the new particles, after the initializers placed them and before the finalizers read the position
  m_fPriority = -499.5f;
}

void ezParticleEmitter_Distance::QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const
{
  if (m_uiMaxParticles > 0)
  {
    out_uiMaxParticlesAbs = m_uiMaxParticles;
    out_uiMaxParticlesPerSecond = 0;
    return;
  }

  // one update can spawn this many
  out_uiMaxParticlesAbs = MaxSpawnPerUpdate;

  // the real rate depends on how fast the effect moves, which is not known here; assume one batch per frame at 60 fps
  out_uiMaxParticlesPerSecond = (m_uiSpawnCountMin + m_uiSpawnCountRange) * 60;

  // TODO: consider to scale by m_sSpawnCountScaleParameter
}

enum class EmitterDistanceVersion
{
  Version_1 = 1,
  Version_2, // added m_uiMaxParticles

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

  // Version 2
  inout_stream << m_uiMaxParticles;
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

  if (uiVersion >= 2)
  {
    inout_stream >> m_uiMaxParticles;
  }
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
  data.m_uiNumSpawned = 0;

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
  data.m_uiNumSpawned = 0;

  const ezVec3 vCurPos = ctx.GetEffect()->GetTransform().m_vPosition;

  if (data.m_bFirstUpdate)
  {
    // the effect may have been created before it was moved to its actual position
    data.m_bFirstUpdate = false;
    data.m_vLastSpawnPosition = vCurPos;
    return 0;
  }

  const ezVec3 vToCurPos = vCurPos - data.m_vLastSpawnPosition;
  const float fDistance = vToCurPos.GetLength();

  if (fDistance < m_fDistanceThreshold)
    return 0;

  // one batch for every full threshold distance, the rest of the distance carries over to the next update
  const ezUInt32 uiNumBatches = static_cast<ezUInt32>(fDistance / m_fDistanceThreshold);
  const ezVec3 vBatchStep = vToCurPos * (m_fDistanceThreshold / fDistance);

  data.m_vEffectPosition = vCurPos;
  data.m_vFirstBatchPosition = data.m_vLastSpawnPosition + vBatchStep;
  data.m_vBatchStep = vBatchStep;
  data.m_vLastSpawnPosition += vBatchStep * static_cast<float>(uiNumBatches);

  const float fSpawnFactor = ezMath::Max(ctx.GetEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  const float fMultiplier = ctx.m_pSystem->GetSpawnCountMultiplier();
  const ezUInt32 uiSpawnCountMin = static_cast<ezUInt32>(m_uiSpawnCountMin * fMultiplier);
  const ezUInt32 uiSpawnCountRange = static_cast<ezUInt32>(m_uiSpawnCountRange * fMultiplier);

  ezUInt32 uiNumSpawned = 0;

  for (ezUInt32 uiBatch = 0; uiBatch < uiNumBatches && uiBatch < MaxSpawnPerUpdate && uiNumSpawned < MaxSpawnPerUpdate; ++uiBatch)
  {
    ezUInt32 uiSpawn = uiSpawnCountMin;

    if (uiSpawnCountRange > 0)
      uiSpawn += ctx.GetRNG().UIntInRange(uiSpawnCountRange);

    uiSpawn = static_cast<ezUInt32>(static_cast<float>(uiSpawn) * fSpawnFactor);
    uiSpawn = ezMath::Min(uiSpawn, MaxSpawnPerUpdate - uiNumSpawned);

    for (ezUInt32 i = 0; i < uiSpawn; ++i)
    {
      data.m_uiBatchOfParticle[uiNumSpawned + i] = static_cast<ezUInt8>(uiBatch);
    }

    uiNumSpawned += uiSpawn;
  }

  data.m_uiNumSpawned = uiNumSpawned;
  return uiNumSpawned;
}

void ezParticleEmitter_Distance::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  // the initializers set the position, this only moves it
  ctx.CreateStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4, &data.m_pStreamPosition, false);
}

void ezParticleEmitter_Distance::InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  const ezUInt32 uiNumSpawned = data.m_uiNumSpawned;
  data.m_uiNumSpawned = 0;

  // in local space the particles move with the effect, so they stay where the initializers put them
  if (uiNumSpawned == 0 || ctx.GetEffect()->IsSimulatedInLocalSpace())
    return;

  EZ_PROFILE_SCOPE("PFX: Distance Emitter Placement");

  ezVec4* pPosition = data.m_pStreamPosition->GetWritableData<ezVec4>();

  // the stream group may have spawned fewer, if the system is full
  const ezUInt64 uiNumToMove = ezMath::Min<ezUInt64>(uiNumElements, uiNumSpawned);

  for (ezUInt64 i = 0; i < uiNumToMove; ++i)
  {
    const ezVec3 vBatchPosition = data.m_vFirstBatchPosition + data.m_vBatchStep * static_cast<float>(data.m_uiBatchOfParticle[i]);
    pPosition[uiStartIndex + i] += (vBatchPosition - data.m_vEffectPosition).GetAsVec4(0.0f);
  }
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_Distance);
