#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Wind.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Wind, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Wind>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("WindInfluence", m_fWindInfluence)->AddAttributes(new ezClampValueAttribute(0.0f, 10.0f), new ezDefaultValueAttribute(1.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Wind, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Wind>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleBehaviorFactory_Wind::ezParticleBehaviorFactory_Wind() = default;
ezParticleBehaviorFactory_Wind::~ezParticleBehaviorFactory_Wind() = default;

const ezRTTI* ezParticleBehaviorFactory_Wind::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Wind>();
}

void ezParticleBehaviorFactory_Wind::CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const
{
  ezParticleBehavior_Wind* pBehavior = static_cast<ezParticleBehavior_Wind*>(pObject);

  pBehavior->m_fWindInfluence = m_fWindInfluence;
}

enum class BehaviorWindVersion
{
  Version_0 = 0,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleBehaviorFactory_Wind::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorWindVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_fWindInfluence;
}

void ezParticleBehaviorFactory_Wind::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorWindVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fWindInfluence;
}

void ezParticleBehavior_Wind::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);

  if (m_fWindInfluence > 0)
  {
    GetOwnerEffect()->RequestWindSamples();
  }
}

void ezParticleBehavior_Wind::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Wind");

  if (m_fWindInfluence <= 0)
    return;

  auto pOwner = GetOwnerEffect();
  const float tDiff = (float)m_TimeDiff.GetSeconds();
  const ezSimdFloat fWindFactor = m_fWindInfluence * tDiff;

  ezProcessingStreamIterator<ezSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);

  while (!itPosition.HasReachedEnd())
  {
    ezSimdVec4f windOffset = pOwner->GetWindAt(itPosition.Current()) * fWindFactor;
    itPosition.Current() += windOffset;

    itPosition.Advance();
  }
}

void ezParticleBehavior_Wind::RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<ezWindWorldModuleInterface>();
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Wind);
