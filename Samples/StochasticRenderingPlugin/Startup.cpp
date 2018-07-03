#include <PCH.h>

#include <Foundation/Configuration/Startup.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(StochasticRendering, StochasticRendering)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "Graphics"
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

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on
