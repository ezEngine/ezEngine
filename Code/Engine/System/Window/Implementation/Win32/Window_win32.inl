#include <System/PCH.h>
#include <Foundation/Basics.h>
#include <System/Basics.h>
#include <System/Window/Window.h>
#include <Foundation/Logging/Log.h>

static LRESULT CALLBACK ezWindowsMessageFuncTrampoline(HWND hWnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
  ezWindow* pWindow = reinterpret_cast<ezWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

  if (pWindow != NULL && pWindow->IsInitialized())
  {
    switch (Msg)
    {
      // do this really always by default?
    case WM_PAINT:
      {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
      }
      return 0;

    case WM_CLOSE:
      pWindow->Destroy();
      return 0;

    case WM_SIZE:
      {
        ezSizeU32 size(LOWORD (LParam), HIWORD (LParam));
        pWindow->OnResizeMessage(size);
        ezLog::Info("Window resized to (%i, %i)", size.width, size.height);
      }
      break;
    }

    pWindow->OnWindowMessage(hWnd, Msg, WParam, LParam);
  } 

  return DefWindowProcW(hWnd, Msg, WParam, LParam);
}


ezResult ezWindow::Initialize()
{
  EZ_LOG_BLOCK("ezWindow")

  if (m_bInitialized)
    Destroy();

  EZ_ASSERT_API(m_CreationDescription.m_ClientAreaSize.HasNonZeroArea(), "The client area size can't be zero sized!");

  // Initialize window class
  WNDCLASSEXW windowClass;
  ezMemoryUtils::ZeroFill(&windowClass);
  windowClass.cbSize         = sizeof(WNDCLASSEXW);
  windowClass.style          = CS_HREDRAW | CS_VREDRAW;
  windowClass.hInstance      = GetModuleHandleW(NULL);
  windowClass.hIcon          = LoadIcon(NULL, IDI_APPLICATION); /// \todo Expose icon functionality somehow
  windowClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
  windowClass.hbrBackground  = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
  windowClass.lpszClassName  = L"ezWindow";
  windowClass.lpfnWndProc    = ezWindowsMessageFuncTrampoline;

  if (!RegisterClassExW(&windowClass)) // \todo test & support for multiple windows
  {
    ezLog::Error("Failed to create ezWindow window class!");
    return EZ_FAILURE;
  }

  // setup fullscreen mode
  if (m_CreationDescription.m_bFullscreenWindow && m_CreationDescription.m_bWindowsUseDevmodeFullscreen)
  {
    DEVMODEW dmScreenSettings;

    ezMemoryUtils::ZeroFill(&dmScreenSettings);
    dmScreenSettings.dmSize = sizeof (DEVMODEW);
    dmScreenSettings.dmPelsWidth = m_CreationDescription.m_ClientAreaSize.width;
    dmScreenSettings.dmPelsHeight = m_CreationDescription.m_ClientAreaSize.height;
    dmScreenSettings.dmBitsPerPel = 32;
    dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

    if (ChangeDisplaySettingsW(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
    {
      m_CreationDescription.m_bFullscreenWindow = false;
      ezLog::Warning("Failed to created fullscreen window. Will created windowed window."); 
    }
  }


  // setup window style
  DWORD dwWindowStyle = 0;
  if (!m_CreationDescription.m_bFullscreenWindow)
    dwWindowStyle = WS_OVERLAPPEDWINDOW;
  else
    dwWindowStyle = WS_POPUP;
 
  // Create rectangle for window
  ezUInt32 x = 0, y = 0;
  if (!m_CreationDescription.m_bFullscreenWindow)
  {
    x = m_CreationDescription.m_WindowPosition.x;
    y = m_CreationDescription.m_WindowPosition.y;
  }
  RECT Rect = {x, y, x + m_CreationDescription.m_ClientAreaSize.width, y + m_CreationDescription.m_ClientAreaSize.height};
  AdjustWindowRect(&Rect, dwWindowStyle, FALSE);

  // Account for left or top placed task bars
  if (!m_CreationDescription.m_bFullscreenWindow)
  {
    RECT RectWorkArea = {0};
    SystemParametersInfo(SPI_GETWORKAREA, 0, &RectWorkArea, 0);

    x += RectWorkArea.left;
    y += RectWorkArea.top;

    int dx = x - Rect.left;
    int dy = y - Rect.top;

    Rect.left += dx;
    Rect.right += dx;

    Rect.top += dy;
    Rect.bottom += dy;
  } 

  // create window
  ezStringWChar sTitelWChar(m_CreationDescription.m_Title.GetData());
  const wchar_t* sTitelWChar_ = sTitelWChar.GetData();
  m_WindowHandle = CreateWindowW(L"ezWindow", sTitelWChar_, dwWindowStyle, 
                                  Rect.left, Rect.top, Rect.right - Rect.left, Rect.bottom - Rect.top, 
                                  NULL, NULL, windowClass.hInstance, NULL);
  if (m_WindowHandle == INVALID_HANDLE_VALUE)
  {
    ezLog::Error("Failed to create window.");
    return EZ_FAILURE;
  }

  // safe window pointer for lookup in ezWindowsMessageFuncTrampoline
  SetWindowLongPtrW(m_WindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  // activate window
  ShowWindow(m_WindowHandle, SW_SHOW);

  m_bInitialized = true;
  ezLog::Success("Created window successfully.");

  return EZ_SUCCESS;
}

ezResult ezWindow::Destroy()
{
  if (m_CreationDescription.m_bFullscreenWindow && m_CreationDescription.m_bWindowsUseDevmodeFullscreen)
    ChangeDisplaySettings (NULL, 0);

  DestroyWindow(GetNativeWindowHandle());
  SetWindowLongPtrW(GetNativeWindowHandle(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(NULL));

  UnregisterClassW(L"ezWindow", GetModuleHandle(NULL));

  m_bInitialized = false;
  m_WindowHandle = INVALID_WINDOW_HANDLE_VALUE;

  ezLog::Success("Window destroyed.");
  return EZ_SUCCESS;
}

ezWindow::WindowMessageResult ezWindow::ProcessWindowMessages()
{
  if (!m_bInitialized)
    return Quit;

  MSG msg = {0};
  while (PeekMessageW(&msg, m_WindowHandle, 0, 0, PM_REMOVE))	
  {
    if (msg.message == WM_QUIT)
    {
      Destroy();
      return Quit;
    }

    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  return Continue;
}