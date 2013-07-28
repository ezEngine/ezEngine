#pragma once

#include <Core/Input/InputDevice.h>

/// \brief This is the base class for all input devices that handle mouse and keyboard input.
///
/// This class is derived from ezInputDevice but adds interface functions to handle mouse and keyboard input.
class EZ_CORE_DLL ezInputDeviceMouseKeyboard : public ezInputDevice
{
public:
  
  /// \brief Returns 'MouseKeyboard'.
  virtual const char* GetDeviceType() const { return "MouseKeyboard"; }

  /// \brief Shows or hides the mouse cursor inside the application window.
  virtual void SetShowMouseCursor(bool bShow) = 0;

  /// \brief Returns whether the mouse cursor is shown.
  virtual bool GetShowMouseCursor() const = 0;

};