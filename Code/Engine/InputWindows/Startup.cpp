#include <Core/PCH.h>
#include <InputWindows/InputDeviceWindows.h>
#include <Foundation/Configuration/Startup.h>

static const ezUInt32 g_uiMaxSupportedWindows = 16;

static ezInputDeviceWindows* g_InputDeviceWindows[g_uiMaxSupportedWindows] = { NULL };

ezInputDeviceWindows* ezInputDeviceWindows::GetDevice(ezUInt32 uiWindowNumber)
{
  EZ_ASSERT_API(uiWindowNumber < g_uiMaxSupportedWindows, "Maximum Number of supported Windows is %i.", g_uiMaxSupportedWindows);

  if (g_InputDeviceWindows[uiWindowNumber] == NULL)
    g_InputDeviceWindows[uiWindowNumber] = EZ_DEFAULT_NEW(ezInputDeviceWindows)(uiWindowNumber);

  return g_InputDeviceWindows[uiWindowNumber];
}

void ezInputDeviceWindows::DestroyAllDevices()
{
  for (ezUInt32 w = 0; w < g_uiMaxSupportedWindows; ++w)
    EZ_DEFAULT_DELETE(g_InputDeviceWindows[w]);
}

EZ_BEGIN_SUBSYSTEM_DECLARATION(InputDevices, InputDeviceWindows)
 
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
    ezInputDeviceWindows::GetDevice(0);
  }
 
  ON_ENGINE_SHUTDOWN
  {
    ezInputDeviceWindows::DestroyAllDevices();
  }
 
EZ_END_SUBSYSTEM_DECLARATION

