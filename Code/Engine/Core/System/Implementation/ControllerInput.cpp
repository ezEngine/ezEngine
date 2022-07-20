#include <Core/CorePCH.h>

#include <Core/System/ControllerInput.h>
#include <Foundation/System/PlatformFeatures.h>

namespace
{
  ezInputDeviceController* g_pInputDeviceController = nullptr;
}

bool ezControllerInput::HasDevice()
{
  return g_pInputDeviceController != nullptr;
}

ezInputDeviceController* ezControllerInput::GetDevice()
{
  return g_pInputDeviceController;
}

void ezControllerInput::SetDevice(ezInputDeviceController* pDevice)
{
  g_pInputDeviceController = pDevice;
}

#if EZ_ENABLED(EZ_SUPPORTS_GLFW)
#  include <Core/System/Implementation/glfw/ControllerInput_glfw.inl>
#endif
