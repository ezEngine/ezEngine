#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <CoreUtils/DataProcessing/Stream/StreamGroup.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitterFactory, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitter, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleEmitter* ezParticleEmitterFactory::CreateEmitter(ezParticleSystemInstance* pOwner) const
{
  const ezRTTI* pRtti = GetEmitterType();

  ezParticleEmitter* pEmitter = (ezParticleEmitter*)pRtti->GetAllocator()->Allocate();
  pEmitter->Reset(pOwner);

  CopyEmitterProperties(pEmitter);
  pEmitter->AfterPropertiesConfigured();
  pEmitter->CreateRequiredStreams();

  return pEmitter;
}
