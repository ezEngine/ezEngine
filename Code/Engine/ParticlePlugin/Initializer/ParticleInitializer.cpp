#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <CoreUtils/DataProcessing/Stream/StreamGroup.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleInitializerFactory::ezParticleInitializerFactory()
{

}

ezParticleInitializer* ezParticleInitializerFactory::CreateInitializer(ezParticleSystemInstance* pOwner) const
{
  const ezRTTI* pRtti = GetInitializerType();

  ezParticleInitializer* pInitializer = (ezParticleInitializer*)pRtti->GetAllocator()->Allocate();
  pInitializer->m_pOwnerSystem = pOwner;

  CopyInitializerProperties(pInitializer);
  pInitializer->AfterPropertiesConfigured();

  return pInitializer;
}

ezParticleInitializer::ezParticleInitializer()
{
}

ezResult ezParticleInitializer::UpdateStreamBindings()
{
  m_pStreamPosition = m_pStreamGroup->GetStreamByName("Position");
  m_pStreamVelocity = m_pStreamGroup->GetStreamByName("Velocity");
  m_pStreamColor = m_pStreamGroup->GetStreamByName("Color");
  m_pStreamLifeTime = m_pStreamGroup->GetStreamByName("LifeTime");

  return EZ_SUCCESS;
}
