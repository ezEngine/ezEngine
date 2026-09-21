#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>

class ezOpenDdlWriter;
class ezOpenDdlReader;
class ezOpenDdlReaderElement;
class ezWindowPlatformShared;

// Currently the following scenarios are possible
// - Windows native implementation, using HWND
// - GLFW on windows, using GLFWWindow* internally and HWND to pass windows around
// - GLFW / XCB on linux. Runtime uses GLFWWindow*. Editor uses xcb-window. Tagged union is passed around as window handle.

#include <WindowDecl_Platform.h>

/// Base class of all window classes that have a client area and a native window handle.
class EZ_CORE_DLL ezWindowBase
{
public:
  virtual ~ezWindowBase() = default;

  virtual ezSizeU32 GetClientAreaSize() const = 0;

  /// Returns the platform specific window handle.
  virtual ezWindowHandle GetNativeWindowHandle() const = 0;

  /// Whether the window is a fullscreen window
  /// or should be one - some platforms may enforce this via the GALSwapchain)
  ///
  /// If bOnlyProperFullscreenMode, the caller accepts borderless windows that cover the entire screen as "fullscreen".
  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const = 0;

  /// Whether the window can potentially be seen by the user.
  /// Windows that are minimized or hidden are not visible.
  virtual bool IsVisible() const = 0;

  /// Runs the platform specific message pump.
  ///
  /// You should call ProcessWindowMessages every frame to keep the window responsive.
  virtual void ProcessWindowMessages() = 0;

  virtual void AddReference() = 0;
  virtual void RemoveReference() = 0;

  /// Returns by which factor UI elements have to be scaled up to appear at a consistent physical size.
  ///
  /// 1.0 is the reference density (96 DPI on Windows). Everything with a fixed pixel size (text, icons, gizmo
  /// handles) has to be multiplied with this. GetClientAreaSize() is always in physical pixels and unaffected,
  /// so 3D rendering can ignore this.
  ///
  /// Changes while the window lives, for instance when it is dragged to another monitor, which is broadcast as
  /// ezWindowEvent::ContentScaleChanged.
  ///
  /// Stays at 1.0 where the value isn't available, and on Windows also as long as the process didn't call
  /// ezScreen::MakeProcessDpiAware().
  virtual float GetContentScaleFactor() const { return 1.0f; }
};

/// Determines how the position and resolution for a window are picked
struct EZ_CORE_DLL ezWindowMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    WindowFixedResolution,                ///< The resolution and size are what the user picked and will not be changed. The window will not be resizable.
    WindowResizable,                      ///< The resolution and size are what the user picked and will not be changed. Allows window resizing by the user.
    FullscreenBorderlessNativeResolution, ///< A borderless window, the position and resolution are taken from the monitor on which the
                                          ///< window shall appear.
    FullscreenFixedResolution,            ///< A fullscreen window using the user provided resolution. Tries to change the monitor resolution
                                          ///< accordingly.

    Default = WindowFixedResolution
  };

  /// Returns whether the window covers an entire monitor. This includes borderless windows and proper fullscreen modes.
  static constexpr bool IsFullscreen(Enum e) { return e == FullscreenBorderlessNativeResolution || e == FullscreenFixedResolution; }
};

/// Parameters for creating a window, such as position and resolution
struct EZ_CORE_DLL ezWindowCreationDesc
{
  /// Adjusts the position and size members, depending on the current value of m_WindowMode and m_iMonitor.
  ///
  /// For windowed mode, this does nothing.
  /// For fullscreen modes, the window position is taken from the given monitor.
  /// For borderless fullscreen mode, the window resolution is also taken from the given monitor.
  ///
  /// This function can only fail if ezScreen::EnumerateScreens fails to enumerate the available screens.
  ezResult AdjustWindowSizeAndPosition();

  /// Serializes the configuration to DDL.
  void SaveToDDL(ezOpenDdlWriter& ref_writer);

  /// Serializes the configuration to DDL.
  ezResult SaveToDDL(ezStringView sFile);

  /// Deserializes the configuration from DDL.
  void LoadFromDDL(const ezOpenDdlReaderElement* pParentElement);

  /// Deserializes the configuration from DDL.
  ezResult LoadFromDDL(ezStringView sFile);


  /// The window title to be displayed.
  ezString m_Title = "ezEngine";

  /// Defines how the window size is determined.
  ezEnum<ezWindowMode> m_WindowMode;

  /// The monitor index is as given by ezScreen::EnumerateScreens.
  /// -1 as the index means to pick the primary monitor.
  ezInt8 m_iMonitor = -1;

  /// The virtual position of the window. Determines on which monitor the window ends up.
  ezVec2I32 m_Position = ezVec2I32(0x80000000, 0x80000000); // Magic number on windows that positions the window at a 'good default position'

  /// The pixel resolution of the window.
  ezSizeU32 m_Resolution = ezSizeU32(1280, 720);

  /// Whether the mouse cursor should be trapped inside the window or not.
  /// \see ezInputDeviceMouseKeyboard::SetClipMouseCursor
  bool m_bClipMouseCursor = true;

  /// Whether the mouse cursor should be visible or not.
  /// \see ezInputDeviceMouseKeyboard::SetShowMouseCursor
  bool m_bShowMouseCursor = false;

  /// Whether the window is activated and focussed on Initialize()
  bool m_bSetForegroundOnInit = true;

  /// Whether the window is centered on the display.
  bool m_bCenterWindowOnDisplay = true;
};

/// Broadcast when various things happen to a window.
///
/// Subscribe through the WindowEvents() function to be notified.
struct ezWindowEvent
{
  enum Type : ezUInt32
  {
    WindowDestruction, ///< Sent shortly before the window gets destroyed
    VisibilityChanged, ///< visibility state is in m_iPayload1 (0 or 1)
    FocusChanged,      ///< focus state is in m_iPayload1 (0 or 1)
    SizeChanged,       ///< new size width/height in m_iPayload1/m_iPayload2
    PositionChanged,   ///< new position x/y in m_iPayload1/m_iPayload2
    CloseButtonClicked,
    ContentScaleChanged, ///< the DPI scaling of the display changed, read the new value with ezWindowBase::GetContentScaleFactor()

    UserEvent = 0xFF,
  };

  Type m_Type;
  ezWindowPlatformShared* m_pWindow = nullptr;

  ezInt32 m_iPayload1 = 0;
  ezInt32 m_iPayload2 = 0;
};

/// A simple abstraction for platform specific window creation.
///
/// Will handle basic message looping. Notable events can be listened to by overriding the corresponding callbacks.
/// You should call ProcessWindowMessages every frame to keep the window responsive.
/// Input messages will not be forwarded automatically. You can do so by overriding the OnWindowMessage function.
class EZ_CORE_DLL ezWindowPlatformShared : public ezWindowBase
{
public:
  /// Creates empty window instance with standard settings
  ///
  /// You need to call InitializeWindow() to actually create a window.
  ezWindowPlatformShared();

  /// Destroys the window if not already done.
  ///
  /// Also broadcasts ezWindowEvent::Type::WindowDestruction.
  ~ezWindowPlatformShared();

  /// Returns the window creation description. The description may get updated by window moves, resizes and such.
  inline const ezWindowCreationDesc& GetCreationDescription() const { return m_CreationDescription; }

  /// Returns the size of the client area / ie. the window resolution.
  virtual ezSizeU32 GetClientAreaSize() const override { return m_CreationDescription.m_Resolution; }

  virtual float GetContentScaleFactor() const override { return m_fContentScaleFactor; }

  /// Returns whether the window covers an entire monitor.
  ///
  /// If bOnlyProperFullscreenMode == false, this includes borderless windows.
  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const override
  {
    if (bOnlyProperFullscreenMode)
      return m_CreationDescription.m_WindowMode == ezWindowMode::FullscreenFixedResolution;

    return ezWindowMode::IsFullscreen(m_CreationDescription.m_WindowMode);
  }

  /// Whether the window is currently theoretically visible.
  ///
  /// How accurate this is, depends on the platform specific implementation.
  /// The implementation may return "visible" even though the window can't be seen.
  /// However, it should never err the other way round.
  bool IsVisible() const override { return m_bVisible; }

  /// Creates a new platform specific window with the current settings
  ///
  /// Will automatically call DestroyWindow() if window is already initialized.
  ///
  /// \see ezWindow::Destroy, ezWindow::Initialize
  virtual ezResult InitializeWindow() = 0;

  /// Creates a new platform specific window with the given settings.
  ///
  /// Will automatically call DestroyWindow() if window is already initialized.
  ///
  /// \param creationDescription
  ///   Struct with various settings for window creation. Will be saved internally for later lookup.
  ///
  /// \see DestroyWindow(), InitializeWindow()
  ezResult Initialize(const ezWindowCreationDesc& creationDescription)
  {
    m_CreationDescription = creationDescription;
    return InitializeWindow();
  }

  /// Gets if the window is up and running.
  inline bool IsInitialized() const { return m_bInitialized; }

  /// Destroys the window.
  virtual void DestroyWindow() = 0;

  /// Tries to resize the window.
  ///
  /// The given size is the desired size of the client area, ie. the resolution that is rendered at.
  /// Override OnResize to get the actual new window size.
  virtual ezResult Resize(const ezSizeU32& newWindowSize) = 0;

  /// Changes the window mode, the monitor and the resolution of an already created window.
  ///
  /// In contrast to Initialize() the window is not destroyed and created again, so its native handle stays valid
  /// and swapchains that reference it keep working. Everything that doesn't describe the window's placement
  /// (the mouse cursor options, for instance) is ignored.
  ///
  /// Returns EZ_FAILURE if the configuration can't be applied to a live window, in which case the window has to
  /// be created again. That is the case for ezWindowMode::FullscreenFixedResolution, which changes the display
  /// resolution, and on platforms where this isn't implemented. Those cases leave the window untouched, only a
  /// failing platform call can leave it partially reconfigured, which is logged.
  virtual ezResult Reconfigure(const ezWindowCreationDesc& desc);

  /// Called when a window got resized.
  ///
  /// The new window size is also saved to the creation description.
  /// The function also broadcasts ezWindowEvent::Type::SizeChanged.
  virtual void OnResize(const ezSizeU32& newWindowSize);

  /// Called when the window position is changed. Not possible on all OSes.
  ///
  /// The function also broadcasts ezWindowEvent::Type::PositionChanged.
  virtual void OnWindowMove(const ezInt32 iNewPosX, const ezInt32 iNewPosY);

  /// Called when the window gets or loses focus.
  ///
  /// The function also broadcasts ezWindowEvent::Type::FocusChanged.
  virtual void OnFocus(bool bHasFocus);

  /// Called when the window gets focus or loses focus.
  ///
  /// The function also broadcasts ezWindowEvent::Type::VisibilityChanged.
  virtual void OnVisibleChange(bool bVisible);

  /// Called when the close button of the window is clicked. Does nothing by default.
  ///
  /// The function also broadcasts ezWindowEvent::Type::CloseButtonClicked.
  virtual void OnClickClose();

  /// Called when the DPI scaling of the display that the window is on changed.
  ///
  /// The new factor is also stored for GetContentScaleFactor().
  /// The function also broadcasts ezWindowEvent::Type::ContentScaleChanged.
  virtual void OnContentScaleChanged(float fNewContentScale);

  /// Returns the input device that is attached to this window and typically provides mouse / keyboard input.
  ezInputDevice* GetInputDevice() const { return m_pInputDevice.Borrow(); }

  /// Allows to subscribe to window events.
  ///
  /// Note that AddEventHandler() is a const function, so can be called on the returned const ezEvent reference.
  const ezEvent<ezWindowEvent>& WindowEvents() const { return m_WindowEvents; }

  virtual void AddReference() override { m_iReferenceCount.Increment(); }
  virtual void RemoveReference() override { m_iReferenceCount.Decrement(); }

protected:
  /// Description at creation time. ezWindow will not update this in any method other than Initialize.
  /// \remarks That means that messages like Resize will also have no effect on this variable.
  ezWindowCreationDesc m_CreationDescription;

  ezEvent<ezWindowEvent> m_WindowEvents;

  bool m_bInitialized = false;
  bool m_bVisible = true;
  bool m_bHasFocus = true;

  ezUniquePtr<ezInputDevice> m_pInputDevice;

  mutable ezWindowInternalHandle m_hWindowHandle = ezWindowInternalHandle();

  ezAtomicInteger32 m_iReferenceCount = 0;

  float m_fContentScaleFactor = 1.0f;
};

// include the platform specific implementation
#include <Window_Platform.h>
