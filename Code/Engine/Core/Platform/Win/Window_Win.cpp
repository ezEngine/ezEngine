#include <Core/CorePCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP) && EZ_DISABLED(EZ_SUPPORTS_GLFW)

#  include <Core/System/Window.h>
#  include <Foundation/Basics.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <Foundation/Platform/Win/Utils/WinDpiUtils.h>
#  include <Foundation/System/SystemInformation.h>

/// Computes the Windows styles that correspond to the given window description.
///
/// bAllowForeground is false when the window already exists, because WS_EX_TOPMOST is only meant to
/// force a fresh window to the front and is removed again afterwards.
static void WindowStylesFromDescription(const ezWindowCreationDesc& desc, bool bAllowForeground, DWORD& out_uiWindowStyle, DWORD& out_uiExStyle)
{
  out_uiExStyle = WS_EX_APPWINDOW;
  out_uiWindowStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

  if (bAllowForeground && desc.m_bSetForegroundOnInit && !ezSystemInformation::IsDebuggerAttached())
  {
    // use WS_EX_TOPMOST to force that the window shows up on top
    // this is the only thing that seems to be working reliably
    // but to prevent the window from staying on top, we need to remove this flag later again (see SetWindowPos)
    out_uiExStyle |= WS_EX_TOPMOST;
  }

  if (desc.m_WindowMode == ezWindowMode::WindowFixedResolution || desc.m_WindowMode == ezWindowMode::WindowResizable)
  {
    ezLog::Dev("Window is not fullscreen.");
    out_uiWindowStyle |= WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
  }
  else
  {
    ezLog::Dev("Window is fullscreen.");
    out_uiWindowStyle |= WS_POPUP;
  }

  if (desc.m_WindowMode == ezWindowMode::WindowResizable)
  {
    ezLog::Dev("Window is resizable.");
    out_uiWindowStyle |= WS_MAXIMIZEBOX | WS_THICKFRAME;
  }
}

static LRESULT CALLBACK ezWindowsMessageFuncTrampoline(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  ezWindowWin* pWindow = reinterpret_cast<ezWindowWin*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

  if (pWindow != nullptr && pWindow->IsInitialized())
  {
    if (auto pInput = ezDynamicCast<ezInputDeviceMouseKeyboard_Win*>(pWindow->GetInputDevice()))
    {
      pInput->WindowMessage(msg, wparam, lparam);
    }

    switch (msg)
    {
      case WM_CLOSE:
        pWindow->OnClickClose();
        return 0;

      case WM_SETFOCUS:
        pWindow->OnFocus(true);
        return 0;

      case WM_KILLFOCUS:
        pWindow->OnFocus(false);
        return 0;

      case WM_SIZE:
      {
        ezSizeU32 size(LOWORD(lparam), HIWORD(lparam));
        pWindow->OnVisibleChange(wparam != SIZE_MINIMIZED);
        if (size.width > 0 && size.height > 0)
          pWindow->OnResize(size);
      }
      break;

      case WM_SYSKEYDOWN:
      {
        // filter this message out, otherwise pressing ALT will give focus to the system menu, locking out other actions
        // until ALT is pressed again, which is typically not desired
        return 0;
      }

      case WM_MOVE:
      {
        pWindow->OnWindowMove((int)(short)LOWORD(lparam), (int)(short)HIWORD(lparam));
      }
      break;

      case WM_DPICHANGED:
      {
        // sent when the window is dragged onto a display with a different scaling, and also when the scaling
        // of the display that it is on is changed while it is there
        const UINT uiNewDpi = LOWORD(wparam);
        const RECT* pSuggestedRect = reinterpret_cast<const RECT*>(lparam);

        const ezWindowMode::Enum mode = pWindow->GetCreationDescription().m_WindowMode;

        if (mode == ezWindowMode::WindowResizable)
        {
          // follow the OS suggestion: the physical window size stays, the client area resolution changes
          ::SetWindowPos(hWnd, nullptr, pSuggestedRect->left, pSuggestedRect->top, pSuggestedRect->right - pSuggestedRect->left,
            pSuggestedRect->bottom - pSuggestedRect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        else if (mode == ezWindowMode::WindowFixedResolution)
        {
          // the requested client area resolution is kept, only the decorations change size
          const ezSizeU32 res = pWindow->GetClientAreaSize();
          const ezSizeU32 size = ezWindowsDpiUtils::ComputeWindowSizeForDpi(res, (DWORD)GetWindowLongPtrW(hWnd, GWL_STYLE), (DWORD)GetWindowLongPtrW(hWnd, GWL_EXSTYLE), uiNewDpi);

          ::SetWindowPos(hWnd, nullptr, pSuggestedRect->left, pSuggestedRect->top, size.width, size.height, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        // fullscreen modes cover an entire display, their size doesn't depend on its scaling

        pWindow->OnContentScaleChanged(ezWindowsDpiUtils::DpiToContentScale(uiNewDpi));
        return 0;
      }
    }

    pWindow->OnWindowMessage(ezMinWindows::FromNative(hWnd), msg, wparam, lparam);
  }

  return DefWindowProcW(hWnd, msg, wparam, lparam);
}

ezWindowWin::~ezWindowWin()
{
  DestroyWindow();
}

ezResult ezWindowWin::InitializeWindow()
{
  EZ_LOG_BLOCK("ezWindowWin::Initialize", m_CreationDescription.m_Title.GetData());

  if (m_bInitialized)
  {
    DestroyWindow();
  }

  EZ_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");

  // Initialize window class
  WNDCLASSEXW windowClass = {};
  windowClass.cbSize = sizeof(WNDCLASSEXW);
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.hInstance = GetModuleHandleW(nullptr);
  windowClass.hIcon = LoadIcon(GetModuleHandleW(nullptr), MAKEINTRESOURCE(101)); /// \todo Expose icon functionality somehow (101 == IDI_ICON1, see resource.h)
  windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
  windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
  windowClass.lpszClassName = L"ezWin32Window";
  windowClass.lpfnWndProc = ezWindowsMessageFuncTrampoline;

  if (!RegisterClassExW(&windowClass)) /// \todo test & support for multiple windows
  {
    DWORD error = GetLastError();

    if (error != ERROR_CLASS_ALREADY_EXISTS)
    {
      ezLog::Error("Failed to create ezWindowWin window class! (error code '{0}')", ezArgErrorCode(error));
      return EZ_FAILURE;
    }
  }

  // setup fullscreen mode
  if (m_CreationDescription.m_WindowMode == ezWindowMode::FullscreenFixedResolution)
  {
    ezLog::Dev("Changing display resolution for fullscreen mode to {0}*{1}", m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height);

    DEVMODEW dmScreenSettings = {};
    dmScreenSettings.dmSize = sizeof(DEVMODEW);
    dmScreenSettings.dmPelsWidth = m_CreationDescription.m_Resolution.width;
    dmScreenSettings.dmPelsHeight = m_CreationDescription.m_Resolution.height;
    dmScreenSettings.dmBitsPerPel = 32;
    dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

    if (ChangeDisplaySettingsW(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
    {
      m_CreationDescription.m_WindowMode = ezWindowMode::FullscreenBorderlessNativeResolution;
      EZ_SUCCEED_OR_RETURN(m_CreationDescription.AdjustWindowSizeAndPosition());

      ezLog::Error("Failed to change display resolution for fullscreen window. Falling back to borderless window.");
    }
  }


  // setup window style
  DWORD dwExStyle = 0;
  DWORD dwWindowStyle = 0;
  WindowStylesFromDescription(m_CreationDescription, true, dwWindowStyle, dwExStyle);


  // Create rectangle for window
  ezRectI32 Rect(m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height);

  // The size of the decorations depends on the scaling of the display that the window ends up on, which isn't
  // known before it exists, so it is guessed from the requested position and corrected after creation (see below).
  const UINT uiCreationDpi = ezWindowsDpiUtils::GetDpiAtPosition(m_CreationDescription.m_Position.x, m_CreationDescription.m_Position.y);

  // Account for left or top placed task bars
  if (m_CreationDescription.m_WindowMode == ezWindowMode::WindowFixedResolution || m_CreationDescription.m_WindowMode == ezWindowMode::WindowResizable)
  {
    // Adjust for borders and bars etc.
    ezWindowsDpiUtils::AdjustWindowRectForDpi(Rect, dwWindowStyle, dwExStyle, uiCreationDpi);

    // apply user translation
    Rect.x = m_CreationDescription.m_Position.x;
    Rect.y = m_CreationDescription.m_Position.y;

    // move into work area
    RECT RectWorkArea = {0};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &RectWorkArea, 0);

    Rect.x += RectWorkArea.left;
    Rect.y += RectWorkArea.top;
  }

  const int iWidth = Rect.width;
  const int iHeight = Rect.height;

  ezLog::Info("Window Dimensions: {0}*{1} at left/top origin ({2}, {3}).", iWidth, iHeight, m_CreationDescription.m_Position.x, m_CreationDescription.m_Position.y);


  // create window
  ezStringWChar sTitleWChar(m_CreationDescription.m_Title.GetData());
  const wchar_t* sTitleWCharRaw = sTitleWChar.GetData();
  m_hWindowHandle = ezMinWindows::FromNative(CreateWindowExW(dwExStyle, windowClass.lpszClassName, sTitleWCharRaw, dwWindowStyle, m_CreationDescription.m_Position.x, m_CreationDescription.m_Position.y, iWidth, iHeight, nullptr, nullptr, windowClass.hInstance, nullptr));

  if (m_hWindowHandle == INVALID_HANDLE_VALUE)
  {
    ezLog::Error("Failed to create window.");
    return EZ_FAILURE;
  }

  auto windowHandle = ezMinWindows::ToNative(m_hWindowHandle);

  // safe window pointer for lookup in ezWindowsMessageFuncTrampoline
  SetWindowLongPtrW(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  // show window and activate if required
  ShowWindow(windowHandle, m_CreationDescription.m_bSetForegroundOnInit ? SW_SHOWDEFAULT : SW_SHOWNOACTIVATE);
  if (m_CreationDescription.m_bSetForegroundOnInit)
  {
    SetActiveWindow(windowHandle);
    SetFocus(windowHandle);
    SetForegroundWindow(windowHandle);
  }

  RECT r;
  GetClientRect(windowHandle, &r);

  // Force size change to the desired size if CreateWindowExW 'fixed' the size to fit into your current monitor,
  // or if the window ended up on a display with a different scaling than was assumed above.
  if (m_CreationDescription.m_WindowMode == ezWindowMode::WindowFixedResolution &&
      (m_CreationDescription.m_Resolution.width != ezUInt32(r.right - r.left) ||
        m_CreationDescription.m_Resolution.height != ezUInt32(r.bottom - r.top)))
  {
    const ezSizeU32 size = ezWindowsDpiUtils::ComputeWindowSizeForDpi(m_CreationDescription.m_Resolution, dwWindowStyle, dwExStyle, ezWindowsDpiUtils::GetWindowDpi(m_hWindowHandle));

    ::SetWindowPos(windowHandle, HWND_NOTOPMOST, 0, 0, size.width, size.height, SWP_NOSENDCHANGING | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
    GetClientRect(windowHandle, &r);
  }

  m_CreationDescription.m_Resolution.width = r.right - r.left;
  m_CreationDescription.m_Resolution.height = r.bottom - r.top;
  m_fContentScaleFactor = ezWindowsDpiUtils::DpiToContentScale(ezWindowsDpiUtils::GetWindowDpi(m_hWindowHandle));



  m_bInitialized = true;
  ezLog::Success("Created window successfully. Resolution is {0}*{1}", GetClientAreaSize().width, GetClientAreaSize().height);

  auto pInput = EZ_DEFAULT_NEW(ezInputDeviceMouseKeyboard_Win, ezMinWindows::FromNative(windowHandle));
  pInput->SetClipMouseCursor(m_CreationDescription.m_bClipMouseCursor ? ezMouseCursorClipMode::ClipToWindowImmediate : ezMouseCursorClipMode::NoClip);
  pInput->SetShowMouseCursor(m_CreationDescription.m_bShowMouseCursor);

  m_pInputDevice = std::move(pInput);

  return EZ_SUCCESS;
}

void ezWindowWin::DestroyWindow()
{
  if (!m_bInitialized)
    return;

  if (auto pInput = ezDynamicCast<ezInputDeviceMouseKeyboard_Win*>(GetInputDevice()))
  {
    pInput->SetClipMouseCursor(ezMouseCursorClipMode::NoClip);
  }

  EZ_LOG_BLOCK("ezWindowWin::Destroy");

  m_pInputDevice = nullptr;

  if (m_CreationDescription.m_WindowMode == ezWindowMode::FullscreenFixedResolution)
    ChangeDisplaySettingsW(nullptr, 0);

  HWND hWindow = ezMinWindows::ToNative(GetNativeWindowHandle());
  // the following line of code is a work around, because 'LONG_PTR pNull = reinterpret_cast<LONG_PTR>(nullptr)' crashes the VS 2010 32 Bit
  // compiler :-(
  LONG_PTR pNull = 0;
  // Set the window ptr to null before calling DestroyWindow as it might trigger callbacks and we are potentially already in the destructor, making any virtual function call unsafe.
  SetWindowLongPtrW(hWindow, GWLP_USERDATA, pNull);

  if (!::DestroyWindow(hWindow))
  {
    ezLog::SeriousWarning("DestroyWindow failed.");
  }

  // actually nobody cares about this, all Window Classes are cleared when the application closes
  // in the mean time, having multiple windows will just result in errors when one is closed,
  // as the Window Class must not be in use anymore when one calls UnregisterClassW
  // if (!UnregisterClassW(L"ezWin32Window", GetModuleHandleW(nullptr)))
  //{
  //  ezLog::SeriousWarning("UnregisterClassW failed.");
  //  Res = EZ_FAILURE;
  //}

  m_bInitialized = false;
  m_hWindowHandle = INVALID_WINDOW_HANDLE_VALUE;

  ezLog::Success("Window destroyed.");
}

ezResult ezWindowWin::Resize(const ezSizeU32& newWindowSize)
{
  if (!m_bInitialized)
    return EZ_FAILURE;

  auto windowHandle = ezMinWindows::ToNative(m_hWindowHandle);

  ezSizeU32 size = newWindowSize;

  // SetWindowPos wants the size of the entire window, so the decorations have to be added to the client size
  if (!ezWindowMode::IsFullscreen(m_CreationDescription.m_WindowMode))
  {
    const DWORD dwWindowStyle = (DWORD)GetWindowLongPtrW(windowHandle, GWL_STYLE);
    const DWORD dwExStyle = (DWORD)GetWindowLongPtrW(windowHandle, GWL_EXSTYLE);

    size = ezWindowsDpiUtils::ComputeWindowSizeForDpi(newWindowSize, dwWindowStyle, dwExStyle, ezWindowsDpiUtils::GetWindowDpi(m_hWindowHandle));
  }

  BOOL res = ::SetWindowPos(windowHandle, HWND_NOTOPMOST, 0, 0, size.width, size.height, SWP_NOSENDCHANGING | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
  return res != FALSE ? EZ_SUCCESS : EZ_FAILURE;
}

ezResult ezWindowWin::Reconfigure(const ezWindowCreationDesc& desc)
{
  if (!m_bInitialized)
    return EZ_FAILURE;

  // this mode changes the resolution of the display itself, which is only done during window creation
  if (desc.m_WindowMode == ezWindowMode::FullscreenFixedResolution || m_CreationDescription.m_WindowMode == ezWindowMode::FullscreenFixedResolution)
    return EZ_FAILURE;

  // only take the placement over, everything else keeps working the way the window was created
  ezWindowCreationDesc newDesc = m_CreationDescription;
  newDesc.m_WindowMode = desc.m_WindowMode;
  newDesc.m_iMonitor = desc.m_iMonitor;
  newDesc.m_Resolution = desc.m_Resolution;
  newDesc.m_bCenterWindowOnDisplay = desc.m_bCenterWindowOnDisplay;

  // for a borderless window this takes the resolution of the target monitor, in every mode it computes the position
  EZ_SUCCEED_OR_RETURN(newDesc.AdjustWindowSizeAndPosition());

  auto windowHandle = ezMinWindows::ToNative(m_hWindowHandle);

  DWORD dwExStyle = 0;
  DWORD dwWindowStyle = 0;
  WindowStylesFromDescription(newDesc, false, dwWindowStyle, dwExStyle);

  // these bits are the current state of the window, not part of the description. Dropping WS_VISIBLE would hide
  // the window: it would keep displaying its last frame, but stop receiving input.
  dwWindowStyle |= (DWORD)GetWindowLongPtrW(windowHandle, GWL_STYLE) & (WS_VISIBLE | WS_MINIMIZE | WS_MAXIMIZE | WS_DISABLED);

  ezSizeU32 size = newDesc.m_Resolution;

  // the window may be moving to a display with a different scaling, so the decorations are sized for the
  // display at the target position, not for the one the window is currently on
  if (!ezWindowMode::IsFullscreen(newDesc.m_WindowMode))
  {
    size = ezWindowsDpiUtils::ComputeWindowSizeForDpi(newDesc.m_Resolution, dwWindowStyle, dwExStyle, ezWindowsDpiUtils::GetDpiAtPosition(newDesc.m_Position.x, newDesc.m_Position.y));
  }

  // the description has to be up to date before the window messages arrive, because the handlers read from it
  m_CreationDescription = newDesc;

  SetWindowLongPtrW(windowHandle, GWL_STYLE, (LONG_PTR)dwWindowStyle);
  SetWindowLongPtrW(windowHandle, GWL_EXSTYLE, (LONG_PTR)dwExStyle);

  // SWP_FRAMECHANGED is what makes the changed styles take effect
  if (::SetWindowPos(windowHandle, HWND_NOTOPMOST, newDesc.m_Position.x, newDesc.m_Position.y, size.width, size.height,
        SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_NOZORDER) == FALSE)
  {
    // the styles were already changed, so the window is left in a mixed state
    ezLog::Error("Failed to reposition the window while reconfiguring it.");
    return EZ_FAILURE;
  }

  // store what the client area actually ended up being
  RECT clientRect;
  GetClientRect(windowHandle, &clientRect);
  m_CreationDescription.m_Resolution.width = clientRect.right - clientRect.left;
  m_CreationDescription.m_Resolution.height = clientRect.bottom - clientRect.top;

  ezLog::Success("Reconfigured window. Resolution is {0}*{1}", GetClientAreaSize().width, GetClientAreaSize().height);
  return EZ_SUCCESS;
}

void ezWindowWin::ProcessWindowMessages()
{
  if (!m_bInitialized)
    return;

  MSG msg = {0};
  while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
  {
    if (msg.message == WM_QUIT)
    {
      DestroyWindow();
      return;
    }

    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  if (m_CreationDescription.m_bSetForegroundOnInit)
  {
    // remove the WS_EX_TOPMOST flag again
    m_CreationDescription.m_bSetForegroundOnInit = false;
    HWND hWindow = ezMinWindows::ToNative(GetNativeWindowHandle());
    SetWindowPos(hWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
  }
}

ezWindowHandle ezWindowWin::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}

#endif
