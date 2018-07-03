#include <PCH.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;


ezParticleBehavior* ezParticleBehaviorFactory::CreateBehavior(ezParticleSystemInstance* pOwner) const
{
  const ezRTTI* pRtti = GetBehaviorType();

  ezParticleBehavior* pBehavior = pRtti->GetAllocator()->Allocate<ezParticleBehavior>();
  pBehavior->Reset(pOwner);

  CopyBehaviorProperties(pBehavior);
  pBehavior->AfterPropertiesConfigured(true);
  pBehavior->CreateRequiredStreams();

  return pBehavior;
}

ezParticleBehavior::ezParticleBehavior()
{
  // run after the initializers, before the types
  m_fPriority = 0.0f;
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior);

