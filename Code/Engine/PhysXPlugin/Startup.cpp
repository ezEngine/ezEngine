#include <PCH.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/Resources/PxMeshResource.h>
#include <Foundation/Configuration/Startup.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(PhysX, PhysXPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezPhysX::GetSingleton()->Startup();

    ezResourceManager::RegisterResourceForAssetType("Collision Mesh", ezGetStaticRTTI<ezPxMeshResource>());
    ezResourceManager::RegisterResourceForAssetType("Collision Mesh (Convex)", ezGetStaticRTTI<ezPxMeshResource>());
  }

  ON_CORE_SHUTDOWN
  {
    ezPhysX::GetSingleton()->Shutdown();
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

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_PHYSXPLUGIN_DLL, ezPhysXPlugin);



EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Startup);

