#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamGroup.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE


ezParticleBehavior* ezParticleBehaviorFactory::CreateBehavior(ezParticleSystemInstance* pOwner) const
{
  const ezRTTI* pRtti = GetBehaviorType();

  ezParticleBehavior* pBehavior = (ezParticleBehavior*)pRtti->GetAllocator()->Allocate();
  pBehavior->Reset(pOwner);

  CopyBehaviorProperties(pBehavior);
  pBehavior->AfterPropertiesConfigured(true);
  pBehavior->CreateRequiredStreams();

  return pBehavior;
}
