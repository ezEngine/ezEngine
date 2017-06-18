#include <PCH.h>
#include <InputXBox360/InputDeviceXBox.h>

// Among other things, this plugin defines input devices, which is why it acts as a subsystem to InputDevices and depends on the InputManager.

EZ_BEGIN_SUBSYSTEM_DECLARATION(InputDevices, WindowsMixedReality)
 
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation", 
    "InputManager"
    
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



EZ_STATICLINK_FILE(WindowsMixedReality, WindowsMixedReality_Startup);

