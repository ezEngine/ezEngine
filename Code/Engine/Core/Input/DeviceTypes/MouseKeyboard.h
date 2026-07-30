#pragma once

#include <Core/Input/InputDevice.h>
#include <Foundation/Math/Vec2.h>

/// Specifies how to restrict movement of the Operating System mouse
struct ezMouseCursorClipMode
{
  enum Enum
  {
    NoClip,                ///< The mouse can move unrestricted and leave the application window
    ClipToWindow,          ///< The mouse cannot leave the window area anymore after the user started interacting with it (ie. clicks into the
                           ///< window)
    ClipToWindowImmediate, ///< The mouse gets restricted to the window area as soon as possible
    ClipToPosition,        ///< The mouse may not leave its current position. Can be used to keep the mouse in place while it is hidden. Note that
                           ///< you will still get mouse move deltas, just the OS cursor will stay in place.

    Default = NoClip,
  };
};

/// This is the base class for all input devices that handle mouse and keyboard input.
///
/// This class is derived from ezInputDevice but adds interface functions to handle mouse and keyboard input.
/// It is typically instantiated on a per-window basis.
class EZ_CORE_DLL ezInputDeviceMouseKeyboard : public ezInputDevice
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInputDeviceMouseKeyboard, ezInputDevice);

public:
  ezInputDeviceMouseKeyboard() { m_vMouseScale.Set(1.0f); }

  /// Shows or hides the mouse cursor inside the application window.
  ///
  /// This only stores what the application wants. Overlay UI (such as the in-game console) may
  /// temporarily override this through ezInputManager::PushMouseCursorOverride(), and a custom
  /// ('software') cursor (see ezInputManager::SetMouseCursor()) hides the OS cursor as well.
  /// Once those no longer apply, the state set here is restored automatically, so callers never
  /// need to save and restore it themselves.
  void SetShowMouseCursor(bool bShow);

  /// Returns whether the application wants the mouse cursor to be shown.
  ///
  /// This ignores any overrides, ie. it returns exactly what was passed to SetShowMouseCursor().
  bool GetShowMouseCursor() const { return m_bShowMouseCursorDesired; }

  /// Returns the edge length of the operating system's mouse cursor, in pixels.
  ///
  /// This takes both the display's DPI scaling and the user's mouse pointer size setting into account,
  /// so it is the value to use to render a custom cursor at the size that the user expects,
  /// independent of resolution, window size and monitor.
  ///
  /// Returns 0 if the platform can't report it.
  ///
  /// Querying this involves system calls. Prefer ezInputManager::GetHardwareCursorSize(),
  /// which caches the value and provides a fallback.
  virtual ezUInt32 GetHardwareCursorSize() const { return 0; }

  /// Will trap the mouse inside the application window. Should usually be enabled, to prevent accidental task switches.
  ///
  /// Especially on multi-monitor systems, the mouse can easily leave the application window (even in fullscreen mode).
  /// Do NOT use this function when you have multiple windows and require absolute mouse positions.
  ///
  /// Like SetShowMouseCursor(), this only stores what the application wants. Overlay UI may
  /// temporarily force ezMouseCursorClipMode::NoClip.
  ///
  /// \sa ezMouseCursorClipMode
  void SetClipMouseCursor(ezMouseCursorClipMode::Enum mode);

  /// Returns how the application wants the mouse to be confined to the window.
  ///
  /// This ignores any overrides, ie. it returns exactly what was passed to SetClipMouseCursor().
  ezMouseCursorClipMode::Enum GetClipMouseCursor() const { return m_ClipModeDesired; }

  /// If enabled, the OS will not handle system hotkeys (e.g. the Windows key) while this device is active.
  ///
  /// Can be called before or after device initialization. On platforms that support it, calling this after
  /// initialization will re-register the input device with the updated setting.
  virtual void SetDisableOSHotkeys(bool bDisable) { m_bDisableOSHotkeys = bDisable; }

  /// Returns whether OS hotkey suppression is currently requested.
  bool GetDisableOSHotkeys() const { return m_bDisableOSHotkeys; }

  /// Sets the scaling factor that is applied on all (relative) mouse input.
  virtual void SetMouseSpeed(const ezVec2& vScale) { m_vMouseScale = vScale; }

  /// Returns the scaling factor that is applied on all (relative) mouse input.
  ezVec2 GetMouseSpeed() const { return m_vMouseScale; }

  /// Returns the number of the ezWindow over which the mouse moved last.
  bool IsMouseOver() const { return s_pMouseOver == this; }

  /// Returns if the associated ezWindow has focus
  bool IsFocused() { return m_bIsFocused; }

  /// Returns the current (normalized) mouse position within the window.
  ///
  /// Coordinates are in [0; 1] range ([0,0] = top left, [1,1] = bottom right of the window).
  ezVec2 GetLocalMouseCoordinates() const { return m_vLocalMouseCoordinates; }

protected:
  friend class ezInputManager;

  virtual void UpdateInputSlotValues() override;

  /// Platform implementation of showing / hiding the OS cursor.
  ///
  /// Called by the base class only, and only when the effective state actually changed,
  /// so implementations don't need to filter out redundant calls themselves.
  ///
  /// \param bShow The state to apply.
  /// \param bCustomCursorActive Whether the cursor is hidden because a custom ('software') cursor is
  ///        drawn instead. In that case the mouse must keep reporting absolute in-window positions,
  ///        ie. the cursor may be hidden, but must not be captured.
  virtual void ApplyShowMouseCursor(bool bShow, bool bCustomCursorActive) = 0;

  /// Platform implementation of confining the OS cursor. \sa ApplyShowMouseCursor()
  virtual void ApplyClipMouseCursor(ezMouseCursorClipMode::Enum mode) = 0;

  /// Recomputes the effective cursor state from the desired state and the overrides that are
  /// registered at the ezInputManager, and forwards it to the platform, if it changed.
  void UpdateEffectiveMouseCursorState();

  static ezInputDevice* s_pMouseOver;

  ezTime m_DoubleClickTime = ezTime::MakeFromMilliseconds(500);
  ezVec2 m_vLocalMouseCoordinates = ezVec2(0.0f);
  bool m_bDisableOSHotkeys = false;

  // What the application requested, see SetShowMouseCursor() / SetClipMouseCursor().
  bool m_bShowMouseCursorDesired = true;
  ezMouseCursorClipMode::Enum m_ClipModeDesired = ezMouseCursorClipMode::NoClip;

  // What was last forwarded to Apply*(), used to filter out redundant calls.
  // These must match the state that the platforms start out with (cursor visible, not confined),
  // otherwise the first real state change would be swallowed.
  // Note that ezWindowCreationDesc::m_bShowMouseCursor defaults to false, ie. window creation
  // usually performs that first state change.
  bool m_bShowMouseCursorEffective = true;
  ezMouseCursorClipMode::Enum m_ClipModeEffective = ezMouseCursorClipMode::NoClip;

private:
  ezVec2 m_vMouseScale = ezVec2(1.0f);
  bool m_bIsFocused = true;

  ezTime m_LastMouseClick[3];
  bool m_bMouseDown[3] = {false, false, false};
};
