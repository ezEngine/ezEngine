#pragma once

#include <System/Basics.h>
#include <SFML/Window.hpp>
#include <Core/Input/DeviceTypes/MouseKeyboard.h>

/// \brief Implements an input device abstraction on top of the SFML library.
///
/// Unfortunately SFML does neither support relative mouse motion, nor clipping a mouse cursor to the window area.
/// Thus relative and absolute mouse position are 'emulated' by this device. As a consequence, relative mouse motion
/// only works without limits, when the mouse cursor is clipped (in that case it is reset to the window center after each
/// movement).
///
/// SFML has several other limitations, as well. For example keyboard input is not language setting independent.
/// To see the effect, just try out what different things happen when you use an English, German and Russian language setting :-(
/// Therefore, it is highly advised to make all key-mappings configurable, such that users with other language settings can
/// 'fix' this on their end.
///
/// Also many special keys are not properly supported by SFML, most notably Numpad Enter is handled like Return.
class EZ_SYSTEM_DLL ezStandardInputDevice : public ezInputDeviceMouseKeyboard
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStandardInputDevice);

public:
  ezStandardInputDevice(sf::Window* pWindow, ezUInt32 uiWindowNumber);
  ~ezStandardInputDevice();

  /// \brief Returns an ezStandardInputDevice device for the given window.
  static ezStandardInputDevice* GetDevice(ezUInt32 uiWindowNumber = 0);

  /// \brief This function needs to be called whenever an SFML event arrives, such that the input device can handle it.
  void WindowMessage(const sf::Event& TheEvent);

  /// \brief Destroys all devices of this type. Automatically called at engine shutdown.
  static void DestroyAllDevices();

  /// \brief Will trap the mouse inside the application window. Should usually be enabled, to prevent accidental task switches.
  ///
  /// Especially on multi-monitor systems, the mouse can easily leave the application window (even in full-screen mode).
  /// Do NOT use this function when you have multiple windows and require absolute mouse positions.
  ///
  /// \note Unfortunately the SFML implementation has some restrictions regarding relative and absolute mouse positions.
  /// Therefore when the mouse cursor is clipped to the window area, it is actually always centered in the window.
  /// This enables relative mouse motions without limits (ie. the cursor cannot reach the screen border, thus it can be moved
  /// indefinitely). On the other hand, it does not make much sense to show the mouse cursor and have it clipped, as the
  /// actual OS cursor position will be quite useless. However, ez then 'emulates' the absolute cursor position, thus
  /// retrieving the absolute mouse position from the input device still works as expected.
  ///
  /// If, however, you need to allow the user to move the mouse cursor outside the window, be aware that the relative mouse
  /// motions will stop working, once the mouse cursor reaches the screen borders.
  void SetClipMouseCursor(bool bEnable);

  /// \brief Returns whether the mouse is confined to the application window or not.
  bool GetClipMouseCursor() const { return m_bClipCursor; }

  /// \brief Shows or hides the mouse cursor inside the application window.
  ///
  /// \note When SetClipMouseCursor is enabled, the mouse will always be centered in the window.
  /// In that case it should also always be hidden.
  virtual void SetShowMouseCursor(bool bShow);

  /// \brief Returns whether the mouse cursor is shown.
  virtual bool GetShowMouseCursor() const { return m_bShowCursor; }

private:
  virtual void InitializeDevice() override { }
  virtual void UpdateInputSlotValues() override;
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;

  static bool s_bMainWindowUsed;

  sf::Window* m_pWindow;

  ezUInt32 m_uiWindowNumber;
  ezVec2I32 m_vEmulatedMousePos;
  ezVec2I32 m_vLastMousePos;

  bool m_bShowCursor;
  bool m_bClipCursor;

  void UpdateMouseCursor();
};

