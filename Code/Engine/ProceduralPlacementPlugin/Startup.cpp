#include <PCH.h>
#include <ProceduralPlacementPlugin/Basics.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Configuration/Plugin.h>
#include <Core/ResourceManager/ResourceManager.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(ProceduralPlacement, ProceduralPlacementPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    /*ezResourceManager::RegisterResourceForAssetType("Particle Effect", ezGetStaticRTTI<ezParticleEffectResource>());

    ezParticleEffectResourceDescriptor desc;
    ezParticleEffectResourceHandle hEffect = ezResourceManager::CreateResource<ezParticleEffectResource>("ParticleEffectMissing", desc, "Fallback for missing Particle Effects");
    ezParticleEffectResource::SetTypeMissingResource(hEffect);*/
  }

  ON_CORE_SHUTDOWN
  {
    //ezParticleEffectResource::CleanupDynamicPluginReferences();
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION

//////////////////////////////////////////////////////////////////////////

void OnLoadPlugin(bool bReloading) { }
void OnUnloadPlugin(bool bReloading) { }

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_PROCEDURALPLACEMENTPLUGIN_DLL, ezProceduralPlacementPlugin);


