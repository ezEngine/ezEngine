#include <OpenVRPluginPCH.h>

#include <OpenVRPlugin/OpenVRIncludes.h>
#include <OpenVRPlugin/OpenVRSingleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <GameEngine/GameApplication/GameApplication.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(OpenVR, OpenVRPlugin)

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

EZ_STATICLINK_FILE(OpenVRPlugin, OpenVRPlugin_OpenVRStartup);
