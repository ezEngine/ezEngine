#pragma once

#include <Core/Input/InputDevice.h>
#include <Foundation/Math/Vec2.h>

/// \brief This is the base class for all input devices that handle mouse and keyboard input.
///
/// This class is derived from ezInputDevice but adds interface functions to handle mouse and keyboard input.
class EZ_CORE_DLL ezInputDeviceMouseKeyboard : public ezInputDevice
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInputDeviceMouseKeyboard);

public:
  ezInputDeviceMouseKeyboard()
  {
    m_vMouseScale.Set(1.0f);
  }
  
  /// \brief Shows or hides the mouse cursor inside the application window.
  virtual void SetShowMouseCursor(bool bShow) = 0;

  /// \brief Returns whether the mouse cursor is shown.
  virtual bool GetShowMouseCursor() const = 0;

  /// \brief Sets the scaling factor that is applied on all (relative) mouse input.
  virtual void SetMouseSpeed(const ezVec2& vScale) { m_vMouseScale = vScale; }

  /// \brief Returns the scaling factor that is applied on all (relative) mouse input.
  ezVec2 GetMouseSpeed() const { return m_vMouseScale; }

private:
  ezVec2 m_vMouseScale;
};

