#include <InputXBox360/PCH.h>
#include <InputXBox360/InputDeviceXBox.h>
#include <Foundation/Configuration/Startup.h>

static ezInputDeviceXBox360* g_InputDeviceXBox360 = nullptr;

ezInputDeviceXBox360* ezInputDeviceXBox360::GetDevice()
{
  if (g_InputDeviceXBox360 == nullptr)
    g_InputDeviceXBox360 = EZ_DEFAULT_NEW(ezInputDeviceXBox360);

  return g_InputDeviceXBox360;
}

void ezInputDeviceXBox360::DestroyAllDevices()
{
  EZ_DEFAULT_DELETE(g_InputDeviceXBox360);
}

EZ_BEGIN_SUBSYSTEM_DECLARATION(InputDevices, InputDeviceXBox360)
 
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation", 
    "InputManager"
    
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
  }
 
  ON_CORE_SHUTDOWN
  {
    ezInputDeviceXBox360::DestroyAllDevices();
  }

  ON_ENGINE_STARTUP
  {
    ezInputDeviceXBox360::GetDevice();
  }
 
  ON_ENGINE_SHUTDOWN
  {
    ezInputDeviceXBox360::DestroyAllDevices();
  }
 
EZ_END_SUBSYSTEM_DECLARATION



EZ_STATICLINK_FILE(InputXBox360, InputXBox360_Startup);

