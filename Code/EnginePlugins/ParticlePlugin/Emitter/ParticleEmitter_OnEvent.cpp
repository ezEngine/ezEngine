#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_OnEvent.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitterFactory_OnEvent, 1, ezRTTIDefaultAllocator<ezParticleEmitterFactory_OnEvent>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("EventName", m_sEventName),
    EZ_MEMBER_PROPERTY("MinSpawnCount", m_uiSpawnCountMin)->AddAttributes(new ezDefaultValueAttribute(1)),
    EZ_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange),
    EZ_MEMBER_PROPERTY("SpawnCountScaleParam", m_sSpawnCountScaleParameter),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitter_OnEvent, 1, ezRTTIDefaultAllocator<ezParticleEmitter_OnEvent>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleEmitterFactory_OnEvent::ezParticleEmitterFactory_OnEvent() = default;
ezParticleEmitterFactory_OnEvent::~ezParticleEmitterFactory_OnEvent() = default;

const ezRTTI* ezParticleEmitterFactory_OnEvent::GetEmitterType() const
{
  return ezGetStaticRTTI<ezParticleEmitter_OnEvent>();
}

void ezParticleEmitterFactory_OnEvent::CopyEmitterProperties(ezParticleEmitter* pEmitter0, bool bFirstTime) const
{
  ezParticleEmitter_OnEvent* pEmitter = static_cast<ezParticleEmitter_OnEvent*>(pEmitter0);

  pEmitter->m_sEventName = ezTempHashedString(m_sEventName.GetData());

  pEmitter->m_uiSpawnCountMin = (ezUInt32)(m_uiSpawnCountMin * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());
  pEmitter->m_uiSpawnCountRange = (ezUInt32)(m_uiSpawnCountRange * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());

  pEmitter->m_sSpawnCountScaleParameter = ezTempHashedString(m_sSpawnCountScaleParameter.GetData());
}

void ezParticleEmitterFactory_OnEvent::QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const
{
  out_uiMaxParticlesAbs = 0;
  out_uiMaxParticlesPerSecond = (m_uiSpawnCountMin + m_uiSpawnCountRange) * 16; // some wild guess

  // TODO: consider to scale by m_sSpawnCountScaleParameter
}

enum class EmitterOnEventVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void ezParticleEmitterFactory_OnEvent::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)EmitterOnEventVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_sEventName;

  // Version 2
  inout_stream << m_uiSpawnCountMin;
  inout_stream << m_uiSpawnCountRange;
  inout_stream << m_sSpawnCountScaleParameter;
}

void ezParticleEmitterFactory_OnEvent::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)EmitterOnEventVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_sEventName;

  if (uiVersion >= 2)
  {
    inout_stream >> m_uiSpawnCountMin;
    inout_stream >> m_uiSpawnCountRange;
    inout_stream >> m_sSpawnCountScaleParameter;
  }
}

ezParticleEmitterState ezParticleEmitter_OnEvent::IsFinished()
{
  return ezParticleEmitterState::OnlyReacting;
}

ezUInt32 ezParticleEmitter_OnEvent::ComputeSpawnCount(const ezTime& tDiff)
{
  if (!m_bSpawn)
    return 0;

  m_bSpawn = false;

  float fSpawnFactor = 1.0f;

  const float spawnCountScale = ezMath::Max(GetOwnerEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  fSpawnFactor *= spawnCountScale;

  return static_cast<ezUInt32>((m_uiSpawnCountMin + GetRNG().UIntInRange(1 + m_uiSpawnCountRange)) * fSpawnFactor);
}

void ezParticleEmitter_OnEvent::ProcessEventQueue(ezParticleEventQueue queue)
{
  if (m_bSpawn)
    return;

  for (const ezParticleEvent& e : queue)
  {
    if (e.m_EventType == m_sEventName) // this is the event type we are waiting for!
    {
      m_bSpawn = true;
      return;
    }
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_OnEvent);
