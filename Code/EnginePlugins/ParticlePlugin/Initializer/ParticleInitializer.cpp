#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Initializer/ParticleInitializer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleInitializer::ezParticleInitializer()
{
  // run these early, but after the stream default initializers
  m_fPriority = -500.0f;
}

float ezParticleInitializer::GetSpawnCountMultiplier(const ezParticleEffectInstance* pEffect) const
{
  return 1.0f;
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer);
