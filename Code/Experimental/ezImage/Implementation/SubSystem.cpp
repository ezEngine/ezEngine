#include <Foundation/Basics.h>
#include <Foundation/Configuration/SubSystem.h>

#include <ImageConversion.h>
#include <ImageFormat.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(Core, Image)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  NULL
  END_SUBSYSTEM_DEPENDENCIES

  ON_BASE_SHUTDOWN
{
}

ON_CORE_STARTUP
{
  ezImageFormat::Startup();
  ezImageConversion::Startup();
}

ON_CORE_SHUTDOWN
{
  ezImageConversion::Shutdown();
}

ON_ENGINE_STARTUP
{
}

ON_ENGINE_SHUTDOWN
{
}

EZ_END_SUBSYSTEM_DECLARATION