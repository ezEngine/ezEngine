#include <Core/CorePCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP) && EZ_DISABLED(EZ_SUPPORTS_GLFW)

#  include <Core/System/Window.h>
#  include <Foundation/Basics.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <Foundation/System/SystemInformation.h>

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
  EZ_LOG_BLOCK("ezWindow::Initialize", m_CreationDescription.m_Title.GetData());

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
      ezLog::Error("Failed to create ezWindow window class! (error code '{0}')", ezArgErrorCode(error));
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
  DWORD dwExStyle = WS_EX_APPWINDOW;
  DWORD dwWindowStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

  if (m_CreationDescription.m_bSetForegroundOnInit && !ezSystemInformation::IsDebuggerAttached())
  {
    // use WS_EX_TOPMOST to force that the window shows up on top
    // this is the only thing that seems to be working reliably
    // but to prevent the window from staying on top, we need to remove this flag later again (see SetWindowPos)
    dwExStyle |= WS_EX_TOPMOST;
  }

  if (m_CreationDescription.m_WindowMode == ezWindowMode::WindowFixedResolution || m_CreationDescription.m_WindowMode == ezWindowMode::WindowResizable)
  {
    ezLog::Dev("Window is not fullscreen.");
    dwWindowStyle |= WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
  }
  else
  {
    ezLog::Dev("Window is fullscreen.");
    dwWindowStyle |= WS_POPUP;
  }

  if (m_CreationDescription.m_WindowMode == ezWindowMode::WindowResizable)
  {
    ezLog::Dev("Window is resizable.");
    dwWindowStyle |= WS_MAXIMIZEBOX | WS_THICKFRAME;
  }


  // Create rectangle for window
  RECT Rect = {0, 0, (LONG)m_CreationDescription.m_Resolution.width, (LONG)m_CreationDescription.m_Resolution.height};

  // Account for left or top placed task bars
  if (m_CreationDescription.m_WindowMode == ezWindowMode::WindowFixedResolution || m_CreationDescription.m_WindowMode == ezWindowMode::WindowResizable)
  {
    // Adjust for borders and bars etc.
    AdjustWindowRectEx(&Rect, dwWindowStyle, FALSE, dwExStyle);

    // top left position now may be negative (due to AdjustWindowRectEx)
    // move
    Rect.right -= Rect.left;
    Rect.bottom -= Rect.top;
    // apply user translation
    Rect.left = m_CreationDescription.m_Position.x;
    Rect.top = m_CreationDescription.m_Position.y;
    Rect.right += m_CreationDescription.m_Position.x;
    Rect.bottom += m_CreationDescription.m_Position.y;

    // move into work area
    RECT RectWorkArea = {0};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &RectWorkArea, 0);

    Rect.left += RectWorkArea.left;
    Rect.right += RectWorkArea.left;
    Rect.top += RectWorkArea.top;
    Rect.bottom += RectWorkArea.top;
  }

  const int iWidth = Rect.right - Rect.left;
  const int iHeight = Rect.bottom - Rect.top;

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

  // Force size change to the desired size if CreateWindowExW 'fixed' the size to fit into your current monitor.
  if (m_CreationDescription.m_WindowMode == ezWindowMode::WindowFixedResolution &&
      (m_CreationDescription.m_Resolution.width != ezUInt32(r.right - r.left) ||
        m_CreationDescription.m_Resolution.height != ezUInt32(r.bottom - r.top)))
  {
    ::SetWindowPos(windowHandle, HWND_NOTOPMOST, 0, 0, iWidth, iHeight, SWP_NOSENDCHANGING | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
    GetClientRect(windowHandle, &r);
  }

  m_CreationDescription.m_Resolution.width = r.right - r.left;
  m_CreationDescription.m_Resolution.height = r.bottom - r.top;



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

  EZ_LOG_BLOCK("ezWindow::Destroy");

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
  auto windowHandle = ezMinWindows::ToNative(m_hWindowHandle);
  BOOL res = ::SetWindowPos(windowHandle, HWND_NOTOPMOST, 0, 0, newWindowSize.width, newWindowSize.height, SWP_NOSENDCHANGING | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
  return res != FALSE ? EZ_SUCCESS : EZ_FAILURE;
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

void ezWindowWin::OnResize(const ezSizeU32& newWindowSize)
{
  ezLog::Info("Window resized to ({0}, {1})", newWindowSize.width, newWindowSize.height);
}

ezWindowHandle ezWindowWin::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}

#endif
