#pragma once

#include <Core/CoreDLL.h>

class ezInputDeviceController;

class EZ_CORE_DLL ezControllerInput
{
public:
  // \brief Returns if a global controller input device exists.
  static bool HasDevice();

  // \brief Returns the global controller input device. May be nullptr.
  static ezInputDeviceController* GetDevice();

  // \brief Set the global controller input device.
  static void SetDevice(ezInputDeviceController* pDevice);
};
