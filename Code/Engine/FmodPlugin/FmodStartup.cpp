#include <FmodPlugin/PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <FmodPlugin/FmodSceneModule.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(Fmod, FmodPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezFmod::GetSingleton()->Startup();
  }

  ON_CORE_SHUTDOWN
  {
    ezFmod::GetSingleton()->Shutdown();
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION
