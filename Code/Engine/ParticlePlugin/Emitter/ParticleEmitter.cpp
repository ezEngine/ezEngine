#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <CoreUtils/DataProcessing/Stream/StreamGroup.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitterFactory, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitter, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleEmitterFactory::ezParticleEmitterFactory()
{

}

ezParticleEmitter* ezParticleEmitterFactory::CreateEmitter(ezParticleSystemInstance* pOwner) const
{
  const ezRTTI* pRtti = GetEmitterType();

  ezParticleEmitter* pEmitter = (ezParticleEmitter*)pRtti->GetAllocator()->Allocate();
  pEmitter->m_pOwnerSystem = pOwner;

  CopyEmitterProperties(pEmitter);
  pEmitter->AfterPropertiesConfigured(true);

  return pEmitter;
}

ezResult ezParticleEmitter::UpdateStreamBindings()
{
  m_pStreamPosition = m_pStreamGroup->GetStreamByName("Position");
  m_pStreamVelocity = m_pStreamGroup->GetStreamByName("Velocity");
  m_pStreamColor = m_pStreamGroup->GetStreamByName("Color");
  m_pStreamLifeTime = m_pStreamGroup->GetStreamByName("LifeTime");

  return EZ_SUCCESS;
}
