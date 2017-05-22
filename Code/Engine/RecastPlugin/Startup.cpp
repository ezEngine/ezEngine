#include <PCH.h>
#include <RecastPlugin/RecastInterface.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <Foundation/Configuration/Startup.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(Recast, RecastPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
  }

  ON_CORE_SHUTDOWN
  {
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

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_RECASTPLUGIN_DLL, ezRecastPlugin);

