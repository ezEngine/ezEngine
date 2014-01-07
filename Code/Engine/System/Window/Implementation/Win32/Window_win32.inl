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

    case WM_SETFOCUS:
      pWindow->OnFocusMessage(true);
      return 0;

    case WM_KILLFOCUS:
      pWindow->OnFocusMessage(false);
      return 0;

    case WM_SIZE:
      {
        ezSizeU32 size(LOWORD (LParam), HIWORD (LParam));
        pWindow->OnResizeMessage(size);
        ezLog::Info("Window resized to (%i, %i)", size.width, size.height);
      }
      break;
    }

    if (pWindow->GetInputDevice())
      pWindow->GetInputDevice()->WindowMessage(hWnd, Msg, WParam, LParam);

    pWindow->OnWindowMessage(hWnd, Msg, WParam, LParam);
  } 

  return DefWindowProcW(hWnd, Msg, WParam, LParam);
}

ezResult ezWindow::Initialize()
{
  EZ_LOG_BLOCK("ezWindow::Initialize", m_CreationDescription.m_Title.GetData());

  if (m_bInitialized)
    Destroy();

  EZ_ASSERT_API(m_CreationDescription.m_ClientAreaSize.HasNonZeroArea(), "The client area size can't be zero sized!");

  // Initialize window class
  WNDCLASSEXW windowClass;
  ezMemoryUtils::ZeroFill(&windowClass);
  windowClass.cbSize         = sizeof(WNDCLASSEXW);
  windowClass.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  windowClass.hInstance      = GetModuleHandleW(NULL);
  windowClass.hIcon          = LoadIcon(NULL, IDI_APPLICATION); /// \todo Expose icon functionality somehow
  windowClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
  windowClass.hbrBackground  = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
  windowClass.lpszClassName  = L"ezWin32Window";
  windowClass.lpfnWndProc    = ezWindowsMessageFuncTrampoline;

  if (!RegisterClassExW(&windowClass)) // \todo test & support for multiple windows
  {
    ezLog::Error("Failed to create ezWindow window class!");
    return EZ_FAILURE;
  }

  // setup fullscreen mode
  if (m_CreationDescription.m_bFullscreenWindow && m_CreationDescription.m_bWindowsUseDevmodeFullscreen)
  {
    ezLog::Dev("Setting up fullscreen mode.");

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
      ezLog::SeriousWarning("Failed to created fullscreen window. Falling back to non-fullscreen mode."); 
    }
  }
  

  // setup window style
  DWORD dwExStyle = WS_EX_APPWINDOW;
  DWORD dwWindowStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

  if (!m_CreationDescription.m_bFullscreenWindow)
  {
    ezLog::Dev("Window is fullscreen.");
    dwWindowStyle |= WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_VISIBLE;
  }
  else
  {
    ezLog::Dev("Window is not fullscreen.");
    dwWindowStyle |= WS_POPUP;
  }

  if(m_CreationDescription.m_bResizable)
  {
    ezLog::Dev("Window is resizable.");
    dwWindowStyle |= WS_MAXIMIZEBOX | WS_THICKFRAME;
  }

 
  // Create rectangle for window
  RECT Rect = {0, 0, m_CreationDescription.m_ClientAreaSize.width, m_CreationDescription.m_ClientAreaSize.height};

  // Account for left or top placed task bars
  if (!m_CreationDescription.m_bFullscreenWindow)
  {
    // Adjust for borders and bars etc.
    AdjustWindowRectEx(&Rect, dwWindowStyle, FALSE, dwExStyle);

    // top left position now be negative (due to AdjustWindowRectEx)
    // move
    Rect.right -= Rect.left;
    Rect.bottom -= Rect.top;
    // apply user translation
    Rect.left = m_CreationDescription.m_WindowPosition.x;
    Rect.top = m_CreationDescription.m_WindowPosition.y;
    Rect.right += m_CreationDescription.m_WindowPosition.x;
    Rect.bottom += m_CreationDescription.m_WindowPosition.y;

    // move into work area
    RECT RectWorkArea = {0};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &RectWorkArea, 0);

    int dx = RectWorkArea.left - Rect.left;
    int dy = RectWorkArea.top - Rect.top;

    Rect.left += dx;
    Rect.right += dx;
    Rect.top += dy;
    Rect.bottom += dy;
  }

  const int iLeft   = Rect.left;
  const int iTop    = Rect.top;
  const int iWidth  = Rect.right - Rect.left;
  const int iHeight = Rect.bottom - Rect.top;

  ezLog::Info("Window Dimensions: %i * %i at left/top origin (%i, %i).", iWidth, iHeight, iLeft, iTop);


  // create window
  ezStringWChar sTitelWChar(m_CreationDescription.m_Title.GetData());
  const wchar_t* sTitelWCharRaw = sTitelWChar.GetData();
  m_WindowHandle = CreateWindowExW(dwExStyle, windowClass.lpszClassName, sTitelWCharRaw, dwWindowStyle, 
                                  iLeft, iTop, iWidth, iHeight, 
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
  SetActiveWindow(m_WindowHandle);
  SetForegroundWindow(m_WindowHandle);
  SetFocus(m_WindowHandle);

  m_bInitialized = true;
  ezLog::Success("Created window successfully.");

  m_pInputDevice = EZ_DEFAULT_NEW(ezStandardInputDevice)(m_CreationDescription.m_uiWindowNumber);

  return CreateGraphicsContext();
}

ezResult ezWindow::Destroy()
{
  EZ_LOG_BLOCK("ezWindow::Destroy");

  ezResult Res = EZ_SUCCESS;

  if (DestroyGraphicsContext() == EZ_FAILURE)
  {
    ezLog::SeriousWarning("DestroyGraphicsContext failed.");
    Res = EZ_FAILURE;
  }

  EZ_DEFAULT_DELETE(m_pInputDevice);

  if (m_CreationDescription.m_bFullscreenWindow && m_CreationDescription.m_bWindowsUseDevmodeFullscreen)
    ChangeDisplaySettingsW(NULL, 0);

  HWND hWindow = GetNativeWindowHandle();
  if (!DestroyWindow(hWindow))
  {
    ezLog::SeriousWarning("DestroyWindow failed.");
    Res = EZ_FAILURE;
  }

  // the following line of code is a work around, because 'LONG_PTR pNull = reinterpret_cast<LONG_PTR>(NULL)' crashes the VS 2010 32 Bit compiler :-(
  LONG_PTR pNull = 0;
  SetWindowLongPtrW(hWindow, GWLP_USERDATA, pNull);

  if (!UnregisterClassW(L"ezWin32Window", GetModuleHandleW(NULL)))
  {
    ezLog::SeriousWarning("UnregisterClassW failed.");
    Res = EZ_FAILURE;
  }

  m_bInitialized = false;
  m_WindowHandle = INVALID_WINDOW_HANDLE_VALUE;

  if (Res == EZ_SUCCESS)
    ezLog::Success("Window destroyed.");
  else
    ezLog::SeriousWarning("There were problems to destroy the Window properly.");

  return Res;
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

void ezWindow::PresentFrame()
{
  switch (m_CreationDescription.m_GraphicsAPI)
  {
  case ezGraphicsAPI::None:
    EZ_REPORT_FAILURE("Cannot present a frame when no graphics API is initialized.");
    return;

  case ezGraphicsAPI::Direct3D11:
    {
      EZ_REPORT_FAILURE("Not implemented.");
    }
    break;

  case ezGraphicsAPI::OpenGL:
    {
      ::SwapBuffers(m_hDC);
    }
    break;
  }
}

ezResult ezWindow::CreateGraphicsContext()
{
  switch (m_CreationDescription.m_GraphicsAPI)
  {
  case ezGraphicsAPI::None:
    return EZ_SUCCESS;

  case ezGraphicsAPI::Direct3D11:
    return CreateContextDirect3D11();

  case ezGraphicsAPI::OpenGL:
    return CreateContextOpenGL();

  default:
    EZ_REPORT_FAILURE("Unknown Graphics API selected.");
  }

  return EZ_FAILURE;
}

ezResult ezWindow::DestroyGraphicsContext()
{
  switch (m_CreationDescription.m_GraphicsAPI)
  {
  case ezGraphicsAPI::None:
    return EZ_SUCCESS;

  case ezGraphicsAPI::Direct3D11:
    return DestroyContextDirect3D11();

  case ezGraphicsAPI::OpenGL:
    return DestroyContextOpenGL();

  default:
    EZ_REPORT_FAILURE("Unknown Graphics API selected.");
  }

  return EZ_FAILURE;
}

ezResult ezWindow::CreateContextOpenGL()
{
  EZ_LOG_BLOCK("ezWindow::CreateContextOpenGL");

  int iColorBits = 24;
  int iDepthBits = 24;
  int iBPC = 8;

  HWND hWnd = GetNativeWindowHandle();

  PIXELFORMATDESCRIPTOR pfd =
  {
    sizeof (PIXELFORMATDESCRIPTOR),
    1, // Version
    PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SWAP_EXCHANGE, // Flags
    PFD_TYPE_RGBA, // Pixeltype
    iColorBits, // Color Bits
    iBPC, 0, iBPC, 0, iBPC, 0, iBPC, 0,// Red Bits / Red Shift, Green Bits / Shift, Blue Bits / Shift, Alpha Bits / Shift
    0, 0, 0, 0, 0, // Accum Bits (total), Accum Bits Red, Green, Blue, Alpha
    iDepthBits, 8, // Depth, Stencil Bits
    0, // Aux Buffers
    PFD_MAIN_PLANE, // Layer Type (ignored)
    0, 0, 0, 0 // ignored deprecated flags
  };

  m_hDC = GetDC(hWnd);

  if (m_hDC == NULL)
  {
    ezLog::Error("Could not retrieve the Window DC");
    goto failure;
  }

  int iPixelformat = ChoosePixelFormat(m_hDC, &pfd);
  if (iPixelformat == 0)
  {
    ezLog::Error("ChoosePixelFormat failed.");
    goto failure;
  }

  if (!SetPixelFormat(m_hDC, iPixelformat, &pfd))
  {
    ezLog::Error("SetPixelFormat failed.");
    goto failure;
  }

  m_hRC = wglCreateContext(m_hDC);
  if (m_hRC == NULL)
  {
    ezLog::Error("wglCreateContext failed.");
    goto failure;
  }

  if (!wglMakeCurrent(m_hDC, m_hRC))
  {
    ezLog::Error("wglMakeCurrent failed.");
    goto failure;
  }

  SetFocus(hWnd);
  SetForegroundWindow(hWnd);

  ezLog::Success("OpenGL graphics context is initialized.");

  return EZ_SUCCESS;

failure:
  ezLog::Error("Failed to initialize the graphics context.");

  DestroyContextOpenGL();
  return EZ_FAILURE;
}

ezResult ezWindow::DestroyContextOpenGL()
{
  EZ_LOG_BLOCK("ezWindow::DestroyContextOpenGL");

  if (!m_hRC && !m_hDC)
    return EZ_SUCCESS;

  if (m_hRC)
  {
    ezLog::Dev("Destroying the RC.");

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(m_hRC);
    m_hRC = NULL;
  }

  if (m_hDC)
  {
    ezLog::Dev("Destroying the DC.");

    ReleaseDC(GetNativeWindowHandle(), m_hDC);
    m_hDC = NULL;
  }

  ezLog::Success("OpenGL graphics context is destroyed.");

  return EZ_SUCCESS;
}

ezResult ezWindow::CreateContextDirect3D11()
{
  EZ_LOG_BLOCK("ezWindow::CreateContextDirect3D11");

  // asserts removed since the code will be moved to a renderplugin anyway
  //EZ_ASSERT(m_CreationDescription.m_GraphicsAPI != ezGraphicsAPI::Direct3D11, "Context creation for Direct3D11 is not yet implemented.");

  //return EZ_FAILURE;
  return EZ_SUCCESS;
}

ezResult ezWindow::DestroyContextDirect3D11()
{
  EZ_LOG_BLOCK("ezWindow::DestroyContextDirect3D11");

  //EZ_ASSERT(m_CreationDescription.m_GraphicsAPI != ezGraphicsAPI::Direct3D11, "Context creation for Direct3D11 is not yet implemented.");

  return EZ_SUCCESS;
}





