#pragma once

#include <System/Basics.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Strings/String.h>


// Include the proper Input implementation to use
#if EZ_ENABLED(EZ_SUPPORTS_SFML)
  #include <System/Window/Implementation/SFML/InputDevice_SFML.h>
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <System/Window/Implementation/Win32/InputDevice_win32.h>
#else 
  #error "Missing code for ezWindow Input!"
#endif

#if EZ_ENABLED(EZ_SUPPORTS_SFML)

  typedef sf::Window* ezWindowHandle;
  #define INVALID_WINDOW_HANDLE_VALUE (sf::Window*)(0)

  /// \brief This struct defines which graphics API should be initialized on a window
  struct ezGraphicsAPI
  {
    /// \brief This struct defines which graphics API should be initialized on a window
    enum Enum
    {
      OpenGL,           ///< SFML always creates a window with an OpenGL context, no way around that
      Default = OpenGL
    };
  };

#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)

  typedef HWND ezWindowHandle;
  #define INVALID_WINDOW_HANDLE_VALUE (ezWindowHandle)(0)

  /// \brief This struct defines which graphics API should be initialized on a window
  struct ezGraphicsAPI
  {
    /// \brief This struct defines which graphics API should be initialized on a window
    enum Enum
    {
      None,                 ///< Creates a window that has no graphics context, allows for custom context creation
      Direct3D11,           ///< Initializes a Direct3D 11 context on the window
      OpenGL,               ///< Initializes an OpenGL context on the window
      Default = Direct3D11
    };
  };

#else
  #error "Missing Platform Code!"
#endif

struct EZ_SYSTEM_DLL ezWindowCreationDesc
{

  ezWindowCreationDesc()
    : m_GraphicsAPI(ezGraphicsAPI::Default),
    m_ClientAreaSize(1280, 720),
    m_Title("ezWindow"),
    m_uiWindowNumber(0),
    m_bFullscreenWindow(false),
    m_bResizable(false),
    m_bWindowsUseDevmodeFullscreen(false)
  {
  }

  /// \brief Which Graphics API to use. Note that not different platforms support different APIs.
  ezGraphicsAPI::Enum m_GraphicsAPI;

  ezVec2U32 m_WindowPosition;
  ezSizeU32 m_ClientAreaSize;

  ezHybridString<64> m_Title;

  /// \brief The number of the window. This is mostly used for setting up the input system, which then reports
  /// different mouse positions for each window.
  ezUInt8 m_uiWindowNumber;

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

  /// \brief Called when the window gets focus or loses focus.
  virtual void OnFocusMessage(bool bHasFocus) {}


#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  /// \brief Called on any window message.
  ///
  /// You can use this function for example to dispatch the message to another system.
  ///
  /// \remarks
  ///   Will be called <i>after</i> the On[...]Message callbacks!
  ///
  /// \see OnResizeMessage
  virtual void OnWindowMessage(HWND hWnd, UINT Msg, WPARAM WParam, LPARAM LParam) {}

  #if EZ_DISABLED(EZ_SUPPORTS_SFML)
    /// \brief Returns the window's device context.
    HDC GetWindowDC() const { return m_hDC; }

    /// \brief Returns the window's OpenGL context. This might not be initialized, if the window was set up with a Direct3D context.
    HGLRC GetOpenGLRC() const { return m_hRC; }
  #endif

#else
  #error "Missing code for ezWindow on this platform!"
#endif

  /// \brief Returns the input device that is attached to this window and typically provides mouse / keyboard input.
  ezStandardInputDevice* GetInputDevice() const { return m_pInputDevice; }

  /// \brief Swaps the back buffer of the graphics API to the front.
  virtual void PresentFrame();

protected:

  /// Description at creation time. ezWindow will not update this in any method other than Initialize.
  /// \remarks That means that messages like Resize will also have no effect on this variable.
  ezWindowCreationDesc m_CreationDescription;

  /// \brief Called by 'Initialize' to set up the graphics context.
  virtual ezResult CreateGraphicsContext();

  /// \brief Called by 'Destroy' to clean up the graphics context.
  virtual ezResult DestroyGraphicsContext();
  
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #if EZ_DISABLED(EZ_SUPPORTS_SFML)

    ezResult CreateContextOpenGL();
    ezResult DestroyContextOpenGL();

    ezResult CreateContextDirect3D11();
    ezResult DestroyContextDirect3D11();

    HDC m_hDC;
    HGLRC m_hRC;
  #endif
#endif

private:

  bool m_bInitialized;

  ezStandardInputDevice* m_pInputDevice;

  mutable ezWindowHandle m_WindowHandle;
};

