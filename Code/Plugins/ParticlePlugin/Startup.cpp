#include <ParticlePluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Particle, ParticlePlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezResourceManager::RegisterResourceForAssetType("Particle Effect", ezGetStaticRTTI<ezParticleEffectResource>());

    ezParticleEffectResourceDescriptor desc;
    ezParticleEffectResourceHandle hEffect = ezResourceManager::CreateResource<ezParticleEffectResource>("ParticleEffectMissing", std::move(desc), "Fallback for missing Particle Effects");
    ezResourceManager::SetResourceTypeMissingFallback<ezParticleEffectResource>(hEffect);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezParticleEffectResource::CleanupDynamicPluginReferences();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Startup);
