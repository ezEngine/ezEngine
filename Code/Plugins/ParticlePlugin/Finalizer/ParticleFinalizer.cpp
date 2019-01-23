#include <PCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFinalizerFactory, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFinalizer, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleFinalizer* ezParticleFinalizerFactory::CreateFinalizer(ezParticleSystemInstance* pOwner) const
{
  const ezRTTI* pRtti = GetFinalizerType();

  ezParticleFinalizer* pFinalizer = pRtti->GetAllocator()->Allocate<ezParticleFinalizer>();
  pFinalizer->Reset(pOwner);

  CopyFinalizerProperties(pFinalizer, true);
  pFinalizer->CreateRequiredStreams();

  return pFinalizer;
}

ezParticleFinalizer::ezParticleFinalizer()
{
  // run after the behaviors, before the types
  m_fPriority = +500.0f;
}
