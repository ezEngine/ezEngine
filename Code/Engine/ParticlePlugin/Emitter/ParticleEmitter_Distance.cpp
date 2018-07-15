#include <PCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Distance.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitterFactory_Distance, 1, ezRTTIDefaultAllocator<ezParticleEmitterFactory_Distance>)
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitter_Distance, 1, ezRTTIDefaultAllocator<ezParticleEmitter_Distance>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleEmitterFactory_Distance::ezParticleEmitterFactory_Distance() = default;

const ezRTTI* ezParticleEmitterFactory_Distance::GetEmitterType() const
{
  return ezGetStaticRTTI<ezParticleEmitter_Distance>();
}

void ezParticleEmitterFactory_Distance::CopyEmitterProperties(ezParticleEmitter* pEmitter0) const
{
  ezParticleEmitter_Distance* pEmitter = static_cast<ezParticleEmitter_Distance*>(pEmitter0);

  pEmitter->m_fDistanceThresholdSQR = ezMath::Square(m_fDistanceThreshold);
  pEmitter->m_uiSpawnCountMin = m_uiSpawnCountMin;
  pEmitter->m_uiSpawnCountRange = m_uiSpawnCountRange;
  pEmitter->m_sSpawnCountScaleParameter = ezTempHashedString(m_sSpawnCountScaleParameter.GetData());
}

enum class EmitterDistanceVersion
{
  Version_1 = 1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void ezParticleEmitterFactory_Distance::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)EmitterDistanceVersion::Version_Current;
  stream << uiVersion;

  // Version 1
  stream << m_fDistanceThreshold;
  stream << m_uiSpawnCountMin;
  stream << m_uiSpawnCountRange;
  stream << m_sSpawnCountScaleParameter;
}

void ezParticleEmitterFactory_Distance::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)EmitterDistanceVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_fDistanceThreshold;
  stream >> m_uiSpawnCountMin;
  stream >> m_uiSpawnCountRange;
  stream >> m_sSpawnCountScaleParameter;
}

void ezParticleEmitter_Distance::CreateRequiredStreams() {}
void ezParticleEmitter_Distance::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) {}

bool ezParticleEmitter_Distance::IsContinuous() const
{
  return true;
}

void ezParticleEmitter_Distance::OnFinalize()
{
  // do not use the System transform, because then this would not work with local space simulation
  m_vLastSpawnPosition = GetOwnerEffect()->GetTransform().m_vPosition;
  m_bFirstUpdate = true;

  if (GetOwnerEffect()->IsSharedEffect())
  {
    ezLog::Warning("Particle emitters of type 'Distance' do not work for shared particle effect instances.");
  }
}


ezParticleEmitterState ezParticleEmitter_Distance::IsFinished()
{
  return ezParticleEmitterState::Active;
}

ezUInt32 ezParticleEmitter_Distance::ComputeSpawnCount(const ezTime& tDiff)
{
  const ezVec3 vCurPos = GetOwnerEffect()->GetTransform().m_vPosition;

  if ((m_vLastSpawnPosition - vCurPos).GetLengthSquared() < m_fDistanceThresholdSQR)
    return 0;

  m_vLastSpawnPosition = vCurPos;

  if (m_bFirstUpdate)
  {
    m_bFirstUpdate = false;
    return 0;
  }

  float fSpawnFactor = 1.0f;

  const float spawnCountScale = ezMath::Clamp(GetOwnerEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f, 10.0f);
  fSpawnFactor *= spawnCountScale;

  ezUInt32 uiSpawn = m_uiSpawnCountMin;

  if (m_uiSpawnCountRange > 0)
    uiSpawn += GetRNG().UIntInRange(m_uiSpawnCountRange);

  uiSpawn = static_cast<ezUInt32>((float)uiSpawn * fSpawnFactor);

  return uiSpawn;
}
