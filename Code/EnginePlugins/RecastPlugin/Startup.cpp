#include <RecastPlugin/RecastPluginPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RecastPlugin/RecastInterface.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>

EZ_BEGIN_PLUGIN(ezRecastPlugin)
EZ_END_PLUGIN;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Recast, RecastPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on
