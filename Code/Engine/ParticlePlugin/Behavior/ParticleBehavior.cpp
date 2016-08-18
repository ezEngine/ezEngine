#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <CoreUtils/DataProcessing/Stream/StreamGroup.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleBehaviorFactory::ezParticleBehaviorFactory()
{

}

ezParticleBehavior::ezParticleBehavior(ezParticleSystemInstance* pOwner)
{
  m_pParticleSystem = pOwner;
}

ezResult ezParticleBehavior::UpdateStreamBindings()
{
  m_pStreamPosition = m_pStreamGroup->GetStreamByName("Position");
  m_pStreamVelocity = m_pStreamGroup->GetStreamByName("Velocity");
  m_pStreamColor = m_pStreamGroup->GetStreamByName("Color");
  m_pStreamLifeTime = m_pStreamGroup->GetStreamByName("LifeTime");

  return EZ_SUCCESS;
}
