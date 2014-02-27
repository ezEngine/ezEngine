#include <Core/PCH.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Configuration/Startup.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(Core, InputManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
  }
 
  ON_CORE_SHUTDOWN
  {
    ezInputManager::DeallocateInternals();
  }

  ON_ENGINE_STARTUP
  {
  }
 
  ON_ENGINE_SHUTDOWN
  {
  }
 
EZ_END_SUBSYSTEM_DECLARATION

EZ_STATICLINK_FILE(Core, Core_Input_Implementation_Startup);

