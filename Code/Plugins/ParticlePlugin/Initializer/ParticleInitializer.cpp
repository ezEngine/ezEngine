#include <ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleInitializer* ezParticleInitializerFactory::CreateInitializer(ezParticleSystemInstance* pOwner) const
{
  const ezRTTI* pRtti = GetInitializerType();

  ezParticleInitializer* pInitializer = pRtti->GetAllocator()->Allocate<ezParticleInitializer>();
  pInitializer->Reset(pOwner);

  CopyInitializerProperties(pInitializer, true);
  pInitializer->CreateRequiredStreams();

  return pInitializer;
}

float ezParticleInitializerFactory::GetSpawnCountMultiplier(const ezParticleEffectInstance* pEffect) const
{
  return 1.0f;
}

ezParticleInitializer::ezParticleInitializer()
{
  // run these early, but after the stream default initializers
  m_fPriority = -500.0f;
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer);
