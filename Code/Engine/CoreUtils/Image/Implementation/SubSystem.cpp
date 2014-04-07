#include <CoreUtils/PCH.h>
#include <Foundation/Configuration/SubSystem.h>

#include <CoreUtils/Image/ImageConversion.h>
#include <CoreUtils/Image/ImageFormat.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(Core, Image)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    nullptr
  END_SUBSYSTEM_DEPENDENCIES

  ON_BASE_STARTUP
  {
    ezImageFormat::Startup();
  }

EZ_END_SUBSYSTEM_DECLARATION



EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Image_Implementation_SubSystem);

