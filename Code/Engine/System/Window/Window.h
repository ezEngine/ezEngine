#pragma once

#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Strings/String.h>
#include <System/Basics.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

typedef HWND ezWindowHandle;
#define INVALID_WINDOW_HANDLE_VALUE (ezWindowHandle)(0)

#elif EZ_ENABLED(EZ_PLATFORM_OSX)

typedef void* ezWindowHandle; // TODO
#define INVALID_WINDOW_HANDLE_VALUE (ezWindowHandle)(0)

#elif EZ_ENABLED(EZ_PLATFORM_LINUX)

typedef void* ezWindowHandle; // TODO
#define INVALID_WINDOW_HANDLE_VALUE (ezWindowHandle)(0)

#else
#error "Missing Platform Code!"
#endif

struct EZ_SYSTEM_DLL ezWindowCreationDesc
{

  ezWindowCreationDesc()
    : m_ClientAreaSize(1280, 720),
    m_Title("ezWindow"),
    m_bFullscreenWindow(false),
    m_bResizable(false),
    m_bWindowsUseDevmodeFullscreen(false)
  {
  }

  ezVec2U32 m_WindowPosition;
  ezSizeU32 m_ClientAreaSize;

  ezHybridString<64> m_Title;

  bool m_bFullscreenWindow;

  /// Enables window resizing by the user.
  /// Ignored for fullscreen windows.
  bool m_bResizable;

  /// Windows only - set to true if you want create a fullscreen window using the Windows device mode
  /// Does not work together with DirectX device settings
  /// \remarks will be ignored if \code{.cpp} m_bFullscreenWindow == false\endcode
  bool m_bWindowsUseDevmodeFullscreen;
};

/// \brief A simple abstraction for platform specific window creation.
///
/// Will handle basic message looping. Notable events can be listened to by overriding the corresponding callbacks.
/// You should call ProcessWindowMessages every frame to keep the window responsive.
/// Input messages will not be forwared automatically. You can do so by overriding the OnWindowMessage function.
class EZ_SYSTEM_DLL ezWindow
{
public:

  /// \brief Creates empty window instance with standard settings
  ///
  /// You need to call Initialize to actually create a window.
  /// \see ezWindow::Initialize
  inline ezWindow()
    : m_bInitialized(false), m_WindowHandle()
  {
  }

  /// \brief Destroys the window if not already done.
  virtual ~ezWindow();

  /// \brief Returns the currently active description struct.
  inline const ezWindowCreationDesc& GetCreationDescription() const
  {
    return m_CreationDescription;
  }

  /// \brief Returns the platform specific window handle.
  inline ezWindowHandle GetNativeWindowHandle() const
  {
    return m_WindowHandle;
  }

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
  inline bool IsInitialized() const
  {
    return m_bInitialized;
  }

  /// \brief Destroys the window.
  ezResult Destroy();

  /// \brief Possible results for window message processing
  enum WindowMessageResult
  {
    Continue,
    Quit
  };

  /// \brief Runs the platform specific message pump.
  ///
  /// You should call ProcessWindowMessages every frame to keep the window responsive.
  WindowMessageResult ProcessWindowMessages();

  /// \brief Called on window resize messages.
  ///
  /// \param newWindowSize
  ///   New window size in pixel.
  /// \see OnWindowMessage
  virtual void OnResizeMessage(const ezSizeU32& newWindowSize) {}


#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  /// \brief Called on any window message.
  ///
  /// You can use this function for example to dispatch the message to another system:
  /// \code {.cpp}
  ///   ezInputDeviceWindows::GetDevice()->WindowMessage(hWnd, msg, wParam, lParam);
  /// \endcode
  ///
  /// \remarks
  ///   Will be called <i>after</i> the On[...]Message callbacks!
  ///
  /// \see OnResizeMessage
  virtual void OnWindowMessage(HWND hWnd, UINT Msg, WPARAM WParam, LPARAM LParam) {}
#else
#error "Missing code for ezWindow on this platform!"
#endif

protected:

  /// Description at creation time. ezWindow will not update this in any method other than Initialize.
  /// \remarks That means that messages like Resize will also have no effect on this variable.
  ezWindowCreationDesc m_CreationDescription;

private:

  bool m_bInitialized;

  mutable ezWindowHandle m_WindowHandle;
};