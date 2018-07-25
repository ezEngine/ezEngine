#include <PCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <GameEngine/Interfaces/WindWorldModule.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Velocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>

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

void ezParticleBehaviorFactory_Velocity::CopyBehaviorProperties(ezParticleBehavior* pObject) const
{
  ezParticleBehavior_Velocity* pBehavior = static_cast<ezParticleBehavior_Velocity*>(pObject);

  pBehavior->m_fRiseSpeed = m_fRiseSpeed;
  pBehavior->m_fFriction = m_fFriction;
  pBehavior->m_fWindInfluence = m_fWindInfluence;
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

void ezParticleBehaviorFactory_Velocity::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorVelocityVersion::Version_Current;
  stream << uiVersion;

  stream << m_fRiseSpeed;
  stream << m_fFriction;

  // Version 3
  stream << m_fWindInfluence;
}

void ezParticleBehaviorFactory_Velocity::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorVelocityVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_fRiseSpeed;
  stream >> m_fFriction;

  if (uiVersion >= 3)
  {
    stream >> m_fWindInfluence;
  }
}

void ezParticleBehaviorFactory_Velocity::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_FinalizerDeps) const
{
  inout_FinalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
}

void ezParticleBehavior_Velocity::AfterPropertiesConfigured(bool bFirstTime)
{
  m_pPhysicsModule = GetOwnerSystem()->GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>();

  if (m_fWindInfluence > 0)
  {
    m_pWindModule = GetOwnerSystem()->GetWorld()->GetModule<ezWindWorldModuleInterface>();
  }
}

void ezParticleBehavior_Velocity::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
}

void ezParticleBehavior_Velocity::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE("PFX: Velocity");

  const float tDiff = (float)m_TimeDiff.GetSeconds();
  const ezVec3 vDown = m_pPhysicsModule != nullptr ? m_pPhysicsModule->GetGravity().GetNormalized() : ezVec3(0.0f, 0.0f, -1.0f);
  const ezVec3 vRise = vDown * tDiff * -m_fRiseSpeed;

  ezVec3 vWind(0);
  if (m_pWindModule != nullptr)
  {
    vWind = m_pWindModule->GetWindAt(GetOwnerSystem()->GetTransform().m_vPosition) * m_fWindInfluence * tDiff;
  }

  const ezVec3 vAddPos0 = vRise + vWind;

  ezSimdVec4f vAddPos;
  vAddPos.Load<3>(&vAddPos0.x);

  const float fFriction = ezMath::Clamp(m_fFriction, 0.0f, 100.0f);
  const float fFrictionFactor = ezMath::Pow(0.5f, tDiff * fFriction);

  ezProcessingStreamIterator<ezSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);
  ezProcessingStreamIterator<ezVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  while (!itPosition.HasReachedEnd())
  {
    itPosition.Current() += vAddPos;
    itVelocity.Current() *= fFrictionFactor;

    itPosition.Advance();
    itVelocity.Advance();
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Velocity);
