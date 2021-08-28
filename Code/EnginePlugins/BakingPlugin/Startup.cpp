#include <BakingPlugin/BakingPluginPCH.h>

#include <BakingPlugin/BakingScene.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>

static ezUniquePtr<ezBaking> s_Baking;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Baking, BakingPlugin)

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
    s_Baking = EZ_DEFAULT_NEW(ezBaking);
    s_Baking->Startup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    s_Baking->Shutdown();
    s_Baking = nullptr;
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezPlugin g_Plugin(false);
