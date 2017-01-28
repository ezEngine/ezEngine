#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleInitializer* ezParticleInitializerFactory::CreateInitializer(ezParticleSystemInstance* pOwner) const
{
  const ezRTTI* pRtti = GetInitializerType();

  ezParticleInitializer* pInitializer = (ezParticleInitializer*)pRtti->GetAllocator()->Allocate();
  pInitializer->Reset(pOwner);

  CopyInitializerProperties(pInitializer);
  pInitializer->AfterPropertiesConfigured(true);
  pInitializer->CreateRequiredStreams();

  return pInitializer;
}
