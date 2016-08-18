#include <ParticlePlugin/PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/Manager/ParticleEffectManager.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(Particle, ParticlePlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    EZ_DEFAULT_NEW(ezParticleEffectManager);

    ezResourceManager::RegisterResourceForAssetType("Particle Effect", ezGetStaticRTTI<ezParticleEffectResource>());

    ezParticleEffectResourceDescriptor desc;
    ezParticleEffectResourceHandle hEffect = ezResourceManager::CreateResource<ezParticleEffectResource>("ParticleEffectMissing", desc, "Fallback for missing Particle Effects");
    ezParticleEffectResource::SetTypeMissingResource(hEffect);
  }

  ON_CORE_SHUTDOWN
  {
    ezParticleEffectResource::CleanupDynamicPluginReferences();

    auto ptr = ezParticleEffectManager::GetSingleton();
    EZ_DEFAULT_DELETE(ptr);
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
    ezParticleEffectManager::GetSingleton()->Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION
