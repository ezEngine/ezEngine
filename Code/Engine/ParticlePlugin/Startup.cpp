#include <ParticlePlugin/PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(Particle, ParticlePlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezResourceManager::RegisterResourceForAssetType("Particle Effect", ezGetStaticRTTI<ezParticleEffectResource>());

    ezParticleEffectResourceDescriptor desc;
    ezParticleEffectResourceHandle hEffect = ezResourceManager::CreateResource<ezParticleEffectResource>("ParticleEffectMissing", desc, "Fallback for missing Particle Effects");
    ezParticleEffectResource::SetTypeMissingResource(hEffect);
  }

  ON_CORE_SHUTDOWN
  {
    ezParticleEffectResource::CleanupDynamicPluginReferences();
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION
