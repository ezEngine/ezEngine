#include <CoreUtils/PCH.h>
#include <Foundation/Configuration/SubSystem.h>

#include <CoreUtils/Image/Conversions/ImageConversion.h>
#include <CoreUtils/Image/ImageFormat.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(Core, Image)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    NULL
  END_SUBSYSTEM_DEPENDENCIES

  ON_BASE_STARTUP
  {
    ezImageFormat::Startup();
  }

EZ_END_SUBSYSTEM_DECLARATION