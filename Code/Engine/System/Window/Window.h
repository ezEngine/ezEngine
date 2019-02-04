#pragma once

#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>
#include <System/Basics.h>

class ezOpenDdlWriter;
class ezOpenDdlReader;
class ezOpenDdlReaderElement;

// Include the proper Input implementation to use
#if EZ_ENABLED(EZ_SUPPORTS_SFML)
#include <System/Window/Implementation/SFML/InputDevice_SFML.h>
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#include <System/Window/Implementation/Win32/InputDevice_win32.h>
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#include <System/Window/Implementation/uwp/InputDevice_uwp.h>
#else
#error "Missing code for ezWindow Input!"
#endif

#if EZ_ENABLED(EZ_SUPPORTS_SFML)

typedef sf::Window* ezWindowHandle;
#define INVALID_WINDOW_HANDLE_VALUE (sf::Window*)(0)

#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

typedef HWND ezWindowHandle;
#define INVALID_WINDOW_HANDLE_VALUE (ezWindowHandle)(0)

#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

typedef IUnknown* ezWindowHandle;
#define INVALID_WINDOW_HANDLE_VALUE nullptr

#else
#error "Missing Platform Code!"
#endif

/// \brief Base class of all window classes that have a client area and a native window handle.
class EZ_SYSTEM_DLL ezWindowBase
{
public:
  virtual ~ezWindowBase() {}

  virtual ezSizeU32 GetClientAreaSize() const = 0;
  virtual ezWindowHandle GetNativeWindowHandle() const = 0;

  /// \brief Whether the window is a fullscreen window
  /// or should be one - some platforms may enforce this via the GALSwapchain)
  ///
  /// If bOnlyProperFullscreenMode, the caller accepts borderless windows that cover the entire screen as "fullscreen".
  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const = 0;

  virtual void ProcessWindowMessages() = 0;
};

/// \brief Determines how the position and resolution for a window are picked
struct EZ_SYSTEM_DLL ezWindowMode
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    WindowFixedResolution, ///< The resolution and size are what the user picked and will not be changed. The window will not be resizable.
    WindowResizable, ///< The resolution and size are what the user picked and will not be changed. Allows window resizing by the user.
    FullscreenBorderlessNativeResolution, ///< A borderless window, the position and resolution are taken from the monitor on which the
                                          ///< window shall appear.
    FullscreenFixedResolution, ///< A fullscreen window using the user provided resolution. Tries to change the monitor resolution
                               ///< accordingly.

    Default = WindowFixedResolution
  };

  /// \brief Returns whether the window covers an entire monitor. This includes borderless windows and proper fullscreen modes.
  static constexpr bool IsFullscreen(Enum e) { return e == FullscreenBorderlessNativeResolution || e == FullscreenFixedResolution; }
};

/// \brief Parameters for creating a window, such as position and resolution
struct EZ_SYSTEM_DLL ezWindowCreationDesc
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
  void SaveToDDL(ezOpenDdlWriter& writer);

  /// Serializes the configuration to DDL.
  ezResult SaveToDDL(const char* szFile);

  /// Deserializes the configuration from DDL.
  void LoadFromDDL(const ezOpenDdlReaderElement* pParentElement);

  /// Deserializes the configuration from DDL.
  ezResult LoadFromDDL(const char* szFile);


  /// The window title to be displayed.
  ezString m_Title = "ezEngine";

  /// Defines how the window size is determined.
  ezEnum<ezWindowMode> m_WindowMode;

  /// The monitor index is as given by ezScreen::EnumerateScreens.
  /// -1 as the index means to pick the primary monitor.
  ezInt8 m_iMonitor = -1;

  /// The virtual position of the window. Determines on which monitor the window ends up.
  ezVec2I32 m_Position =
      ezVec2I32(0x80000000, 0x80000000); // Magic number on windows that positions the window at a 'good default position'

  /// The pixel resolution of the window.
  ezSizeU32 m_Resolution = ezSizeU32(1280, 720);

  /// The number of the window. This is mostly used for setting up the input system, which then reports
  /// different mouse positions for each window.
  ezUInt8 m_uiWindowNumber = 0;

  /// Whether the mouse cursor should be trapped inside the window or not.
  /// \see ezStandardInputDevice::SetClipMouseCursor
  bool m_bClipMouseCursor = true;

  /// Whether the mouse cursor should be visible or not.
  /// \see ezStandardInputDevice::SetShowMouseCursor
  bool m_bShowMouseCursor = false;
};

/// \brief A simple abstraction for platform specific window creation.
///
/// Will handle basic message looping. Notable events can be listened to by overriding the corresponding callbacks.
/// You should call ProcessWindowMessages every frame to keep the window responsive.
/// Input messages will not be forwarded automatically. You can do so by overriding the OnWindowMessage function.
class EZ_SYSTEM_DLL ezWindow : public ezWindowBase
{
public:
  /// \brief Creates empty window instance with standard settings
  ///
  /// You need to call Initialize to actually create a window.
  /// \see ezWindow::Initialize
  ezWindow();

  /// \brief Destroys the window if not already done.
  virtual ~ezWindow();

  /// \brief Returns the currently active description struct.
  inline const ezWindowCreationDesc& GetCreationDescription() const { return m_CreationDescription; }

  /// \brief Returns the size of the client area / ie. the window resolution.
  virtual ezSizeU32 GetClientAreaSize() const override { return m_CreationDescription.m_Resolution; }

  /// \brief Returns the platform specific window handle.
  virtual ezWindowHandle GetNativeWindowHandle() const override { return m_WindowHandle; }

  /// \brief Returns whether the window covers an entire monitor.
  ///
  /// If bOnlyProperFullscreenMode == false, this includes borderless windows.
  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const override
  {
    if (bOnlyProperFullscreenMode)
      return m_CreationDescription.m_WindowMode == ezWindowMode::FullscreenFixedResolution;

    return ezWindowMode::IsFullscreen(m_CreationDescription.m_WindowMode);
  }


  /// \brief Runs the platform specific message pump.
  ///
  /// You should call ProcessWindowMessages every frame to keep the window responsive.
  virtual void ProcessWindowMessages() override;

  /// \brief Creates a new platform specific window with the current settings
  ///
  /// Will automatically call ezWindow::Destroy if window is already initialized.
  ///
  /// \see ezWindow::Destroy, ezWindow::Initialize
  ezResult Initialize();

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
    return Initialize();
  }

  /// \brief Gets if the window is up and running.
  inline bool IsInitialized() const { return m_bInitialized; }

  /// \brief Destroys the window.
  ezResult Destroy();

  /// \brief Called on window resize messages.
  ///
  /// \param newWindowSize
  ///   New window size in pixel.
  /// \see OnWindowMessage
  virtual void OnResize(const ezSizeU32& newWindowSize);

  /// \brief Called when the window position is changed. Not possible on all OSes.
  virtual void OnWindowMove(const ezInt32 newPosX, const ezInt32 newPosY) {}

  /// \brief Called when the window gets focus or loses focus.
  virtual void OnFocus(bool bHasFocus) {}

  /// \brief Called when the close button of the window is clicked. Does nothing by default.
  virtual void OnClickClose() {}


#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  /// \brief Called on any window message.
  ///
  /// You can use this function for example to dispatch the message to another system.
  ///
  /// \remarks
  ///   Will be called <i>after</i> the On[...] callbacks!
  ///
  /// \see OnResizeMessage
  virtual void OnWindowMessage(HWND hWnd, UINT Msg, WPARAM WParam, LPARAM LParam) {}

#elif EZ_ENABLED(EZ_PLATFORM_OSX)

#elif EZ_ENABLED(EZ_PLATFORM_LINUX)

#else
#error "Missing code for ezWindow on this platform!"
#endif

  /// \brief Returns the input device that is attached to this window and typically provides mouse / keyboard input.
  ezStandardInputDevice* GetInputDevice() const { return m_pInputDevice.Borrow(); }

protected:
  /// Description at creation time. ezWindow will not update this in any method other than Initialize.
  /// \remarks That means that messages like Resize will also have no effect on this variable.
  ezWindowCreationDesc m_CreationDescription;

private:
  bool m_bInitialized = false;

  ezUniquePtr<ezStandardInputDevice> m_pInputDevice;

  mutable ezWindowHandle m_WindowHandle = ezWindowHandle();
};

