#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFinalizer, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleFinalizer::ezParticleFinalizer()
{
  // run after the behaviors, before the types
  m_fPriority = +500.0f;
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer);
