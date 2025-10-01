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

// Currently the following scenarios are possible
// - Windows native implementation, using HWND
// - GLFW on windows, using GLFWWindow* internally and HWND to pass windows around
// - GLFW / XCB on linux. Runtime uses GLFWWindow*. Editor uses xcb-window. Tagged union is passed around as window handle.

#include <WindowDecl_Platform.h>

/// \brief Base class of all window classes that have a client area and a native window handle.
class EZ_CORE_DLL ezWindowBase
{
public:
  virtual ~ezWindowBase() = default;

  virtual ezSizeU32 GetClientAreaSize() const = 0;

  /// \brief Returns the platform specific window handle.
  virtual ezWindowHandle GetNativeWindowHandle() const = 0;

  /// \brief Whether the window is a fullscreen window
  /// or should be one - some platforms may enforce this via the GALSwapchain)
  ///
  /// If bOnlyProperFullscreenMode, the caller accepts borderless windows that cover the entire screen as "fullscreen".
  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const = 0;

  /// \brief Whether the window can potentially be seen by the user.
  /// Windows that are minimized or hidden are not visible.
  virtual bool IsVisible() const = 0;

  /// \brief Runs the platform specific message pump.
  ///
  /// You should call ProcessWindowMessages every frame to keep the window responsive.
  virtual void ProcessWindowMessages() = 0;

  virtual void AddReference() = 0;
  virtual void RemoveReference() = 0;
};

/// \brief Determines how the position and resolution for a window are picked
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

  /// \brief Returns whether the window covers an entire monitor. This includes borderless windows and proper fullscreen modes.
  static constexpr bool IsFullscreen(Enum e) { return e == FullscreenBorderlessNativeResolution || e == FullscreenFixedResolution; }
};

/// \brief Parameters for creating a window, such as position and resolution
struct EZ_CORE_DLL ezWindowCreationDesc
{
  /// \brief Adjusts the position and size members, depending on the current value of m_WindowMode and m_iMonitor.
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

/// \brief A simple abstraction for platform specific window creation.
///
/// Will handle basic message looping. Notable events can be listened to by overriding the corresponding callbacks.
/// You should call ProcessWindowMessages every frame to keep the window responsive.
/// Input messages will not be forwarded automatically. You can do so by overriding the OnWindowMessage function.
class EZ_CORE_DLL ezWindowPlatformShared : public ezWindowBase
{
public:
  /// \brief Creates empty window instance with standard settings
  ///
  /// You need to call Initialize to actually create a window.
  /// \see ezWindow::Initialize
  ezWindowPlatformShared();

  /// \brief Destroys the window if not already done.
  ~ezWindowPlatformShared();

  /// \brief Returns the currently active description struct.
  inline const ezWindowCreationDesc& GetCreationDescription() const { return m_CreationDescription; }

  /// \brief Returns the size of the client area / ie. the window resolution.
  virtual ezSizeU32 GetClientAreaSize() const override { return m_CreationDescription.m_Resolution; }

  /// \brief Returns whether the window covers an entire monitor.
  ///
  /// If bOnlyProperFullscreenMode == false, this includes borderless windows.
  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const override
  {
    if (bOnlyProperFullscreenMode)
      return m_CreationDescription.m_WindowMode == ezWindowMode::FullscreenFixedResolution;

    return ezWindowMode::IsFullscreen(m_CreationDescription.m_WindowMode);
  }

  virtual bool IsVisible() const override { return m_bVisible; }

  virtual void AddReference() override { m_iReferenceCount.Increment(); }
  virtual void RemoveReference() override { m_iReferenceCount.Decrement(); }

  /// \brief Creates a new platform specific window with the current settings
  ///
  /// Will automatically call ezWindow::Destroy if window is already initialized.
  ///
  /// \see ezWindow::Destroy, ezWindow::Initialize
  virtual ezResult InitializeWindow() = 0;

  /// \brief Creates a new platform specific window with the given settings.
  ///
  /// Will automatically call ezWindow::Destroy if window is already initialized.
  ///
  /// \param creationDescription
  ///   Struct with various settings for window creation. Will be saved internally for later lookup.
  ///
  /// \see ezWindow::Destroy, ezWindow::Initialize
  ezResult Initialize(const ezWindowCreationDesc& creationDescription)
  {
    m_CreationDescription = creationDescription;
    return InitializeWindow();
  }

  /// \brief Gets if the window is up and running.
  inline bool IsInitialized() const { return m_bInitialized; }

  /// \brief Destroys the window.
  virtual void DestroyWindow() = 0;

  /// \brief Tries to resize the window.
  /// Override OnResize to get the actual new window size.
  virtual ezResult Resize(const ezSizeU32& newWindowSize) = 0;

  /// \brief Called on window resize messages.
  ///
  /// \param newWindowSize
  ///   New window size in pixel.
  /// \see OnWindowMessage
  virtual void OnResize(const ezSizeU32& newWindowSize) = 0;

  /// \brief Called when the window position is changed. Not possible on all OSes.
  virtual void OnWindowMove(const ezInt32 iNewPosX, const ezInt32 iNewPosY)
  {
    EZ_IGNORE_UNUSED(iNewPosX);
    EZ_IGNORE_UNUSED(iNewPosY);
  }

  /// \brief Called when the window gets focus or loses focus.
  virtual void OnFocus(bool bHasFocus) { EZ_IGNORE_UNUSED(bHasFocus); }

  /// \brief Called when the window gets focus or loses focus.
  virtual void OnVisibleChange(bool bVisible) { m_bVisible = bVisible; }

  /// \brief Called when the close button of the window is clicked. Does nothing by default.
  virtual void OnClickClose() {}

  /// \brief Returns the input device that is attached to this window and typically provides mouse / keyboard input.
  ezInputDevice* GetInputDevice() const { return m_pInputDevice.Borrow(); }

  /// \brief Returns a number that can be used as a window number in ezWindowCreationDesc
  ///
  /// This number just increments every time an ezWindow is created. It starts at zero.
  static ezUInt8 GetNextUnusedWindowNumber();

protected:
  /// Description at creation time. ezWindow will not update this in any method other than Initialize.
  /// \remarks That means that messages like Resize will also have no effect on this variable.
  ezWindowCreationDesc m_CreationDescription;

  bool m_bInitialized = false;
  bool m_bVisible = true;

  ezUniquePtr<ezInputDevice> m_pInputDevice;

  mutable ezWindowInternalHandle m_hWindowHandle = ezWindowInternalHandle();

  /// increased every time an ezWindow is created, to be able to get a free window index easily
  static ezUInt8 s_uiNextUnusedWindowNumber;
  ezAtomicInteger32 m_iReferenceCount = 0;
};

// include the platform specific implementation
#include <Window_Platform.h>
