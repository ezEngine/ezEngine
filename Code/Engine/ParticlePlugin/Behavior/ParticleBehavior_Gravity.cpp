#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Gravity.h>
#include <Core/World/WorldModule.h>
#include <GameUtils/Interfaces/PhysicsWorldModule.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <CoreUtils/DataProcessing/Stream/StreamElementIterator.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Gravity, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Gravity>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Gravity Factor", m_fGravityFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Gravity, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleBehaviorFactory_Gravity::ezParticleBehaviorFactory_Gravity()
{
  m_fGravityFactor = 1.0f;
}


ezParticleBehavior* ezParticleBehaviorFactory_Gravity::CreateBehavior(ezParticleSystemInstance* pOwner) const
{
  ezParticleBehavior_Gravity* pBehavior = EZ_DEFAULT_NEW(ezParticleBehavior_Gravity, pOwner);

  // Copy Properties
  {
    pBehavior->m_fGravityFactor = m_fGravityFactor;
  }

  return pBehavior;
}

void ezParticleBehaviorFactory_Gravity::Save(ezStreamWriter& stream) const
{
  ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_fGravityFactor;
}

void ezParticleBehaviorFactory_Gravity::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_fGravityFactor;
}

ezParticleBehavior_Gravity::ezParticleBehavior_Gravity(ezParticleSystemInstance* pOwner)
  : ezParticleBehavior(pOwner)
{
  m_fGravityFactor = 1.0f;
  m_pPhysicsModule = static_cast<ezPhysicsWorldModuleInterface*>(ezWorldModule::FindModule(m_pParticleSystem->GetWorld(), ezPhysicsWorldModuleInterface::GetStaticRTTI()));
}

void ezParticleBehavior_Gravity::Process(ezUInt64 uiNumElements)
{
  const ezVec3 vGravity = m_pPhysicsModule->GetGravity();

  const float tDiff = (float)m_pParticleSystem->GetWorld()->GetClock().GetTimeDiff().GetSeconds();
  const ezVec3 addGravity = vGravity * m_fGravityFactor * tDiff;

  ezStreamElementIterator<ezVec3> itVelocity(m_pStreamVelocity, uiNumElements);

  while (!itVelocity.HasReachedEnd())
  {
    itVelocity.Current() += addGravity;

    itVelocity.Advance();
  }
}

void ezParticleBehavior_Gravity::StepParticleSystem()
{
}
