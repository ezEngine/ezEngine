#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Velocity.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Velocity, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Velocity>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RiseSpeed", m_fRiseSpeed),
    EZ_MEMBER_PROPERTY("Friction", m_fFriction)->AddAttributes(new ezClampValueAttribute(0.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("WindInfluence", m_fWindInfluence)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Velocity, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Velocity>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleBehaviorFactory_Velocity::ezParticleBehaviorFactory_Velocity() = default;
ezParticleBehaviorFactory_Velocity::~ezParticleBehaviorFactory_Velocity() = default;

const ezRTTI* ezParticleBehaviorFactory_Velocity::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Velocity>();
}

void ezParticleBehaviorFactory_Velocity::CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const
{
  ezParticleBehavior_Velocity* pBehavior = static_cast<ezParticleBehavior_Velocity*>(pObject);

  pBehavior->m_fRiseSpeed = m_fRiseSpeed;
  pBehavior->m_fFriction = m_fFriction;
  pBehavior->m_fWindInfluence = m_fWindInfluence;


  pBehavior->m_pPhysicsModule = (ezPhysicsWorldModuleInterface*)pBehavior->GetOwnerSystem()->GetOwnerWorldModule()->GetCachedWorldModule(ezGetStaticRTTI<ezPhysicsWorldModuleInterface>());
}

enum class BehaviorVelocityVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added rise speed and acceleration
  Version_3, // added wind influence

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleBehaviorFactory_Velocity::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorVelocityVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_fRiseSpeed;
  inout_stream << m_fFriction;

  // Version 3
  inout_stream << m_fWindInfluence;
}

void ezParticleBehaviorFactory_Velocity::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorVelocityVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fRiseSpeed;
  inout_stream >> m_fFriction;

  if (uiVersion >= 3)
  {
    inout_stream >> m_fWindInfluence;
  }
}

void ezParticleBehaviorFactory_Velocity::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
}

void ezParticleBehavior_Velocity::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity, false);

  if (m_fWindInfluence > 0)
  {
    GetOwnerEffect()->RequestWindSamples();
  }
}

void ezParticleBehavior_Velocity::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Velocity");

  auto pOwner = GetOwnerEffect();

  const float tDiff = (float)m_TimeDiff.GetSeconds();
  const ezVec3 vDown = m_pPhysicsModule != nullptr ? m_pPhysicsModule->GetGravity().GetNormalized() : ezVec3(0.0f, 0.0f, -1.0f);
  const ezSimdVec4f vRise = ezSimdConversion::ToVec3(vDown * tDiff * -m_fRiseSpeed);

  const float fFriction = ezMath::Clamp(m_fFriction, 0.0f, 100.0f);
  const float fFrictionFactor = ezMath::Pow(0.5f, tDiff * fFriction);

  ezProcessingStreamIterator<ezSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);
  ezProcessingStreamIterator<ezVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  if (m_fWindInfluence > 0)
  {
    const ezSimdFloat fWindFactor = m_fWindInfluence * tDiff;

    while (!itPosition.HasReachedEnd())
    {
      ezSimdVec4f addPos = vRise + pOwner->GetWindAt(itPosition.Current()) * fWindFactor;

      itPosition.Current() += addPos;
      itVelocity.Current() *= fFrictionFactor;

      itPosition.Advance();
      itVelocity.Advance();
    }
  }
  else
  {
    while (!itPosition.HasReachedEnd())
    {
      itPosition.Current() += vRise;
      itVelocity.Current() *= fFrictionFactor;

      itPosition.Advance();
      itVelocity.Advance();
    }
  }
}

void ezParticleBehavior_Velocity::RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<ezPhysicsWorldModuleInterface>();
  pParticleModule->CacheWorldModule<ezWindWorldModuleInterface>();
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Velocity);
