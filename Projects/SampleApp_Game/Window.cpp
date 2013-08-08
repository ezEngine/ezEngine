#include "Application.h"
#include <Core/Input/InputManager.h>
#include <InputWindows/InputDeviceWindows.h>
#include <gl/GL.h>

static HWND g_hWnd = NULL;
static HDC g_hDC = NULL;
static HGLRC g_hRC = NULL;

static LRESULT CALLBACK WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  ezInputDeviceWindows::GetDevice()->WindowMessage(hWnd, msg, wParam, lParam);

  switch (msg)
  {
  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      BeginPaint (hWnd, &ps);
      EndPaint (hWnd, &ps);
    }
    return (0);

  case WM_SIZE:
    {
      SampleGameApp* pApp = (SampleGameApp*) ezApplication::GetApplicationInstance();

      pApp->m_uiResolutionX = LOWORD (lParam);
      pApp->m_uiResolutionY = HIWORD (lParam);
    }
    break;
  }

  return (DefWindowProc (hWnd, msg, wParam, lParam));
}

static void CreateContextOGL()
{
  int iColorBits = 24;
  int iDepthBits = 24;
  int iBPC = 8;

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

  HDC hDC = GetDC (g_hWnd);

  if (hDC == NULL)
    return;

  int iPixelformat = ChoosePixelFormat (hDC, &pfd);
  if (iPixelformat == 0)
    return;

  if (!SetPixelFormat (hDC, iPixelformat, &pfd))
    return;

  HGLRC hRC = wglCreateContext (hDC);
  if (hRC == NULL)
    return;

  if (!wglMakeCurrent (hDC, hRC))
    return;

  g_hDC = hDC;
  g_hRC = hRC;

  SetFocus(g_hWnd);
  SetForegroundWindow(g_hWnd);
}

static void DestroyContextOGL()
{
  if (g_hRC)
  {
    wglMakeCurrent (NULL, NULL);
    wglDeleteContext (g_hRC);
    g_hRC = NULL;
  }

  if (g_hDC)
  {
    ReleaseDC (g_hWnd, g_hDC);
    g_hDC = NULL;
  }
}


void SampleGameApp::CreateAppWindow()
{
  WNDCLASSEX wc;
  ZeroMemory (&wc, sizeof (wc));
  wc.cbSize = sizeof (wc);
  wc.lpszClassName = m_szAppName;
  wc.hInstance = GetModuleHandle(NULL);
  wc.lpfnWndProc = WndProc;
  wc.hCursor = LoadCursor (NULL, IDC_ARROW);
  wc.hIcon = LoadIcon (NULL, IDI_APPLICATION);
  wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
  RegisterClassEx (&wc);


  if (m_bFullscreen)
  {
    DEVMODE dmScreenSettings;

    memset (&dmScreenSettings, 0, sizeof (DEVMODE));
    dmScreenSettings.dmSize = sizeof (DEVMODE);
    dmScreenSettings.dmPelsWidth = m_uiResolutionX;
    dmScreenSettings.dmPelsHeight = m_uiResolutionY;
    dmScreenSettings.dmBitsPerPel = 32;
    dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

    if (ChangeDisplaySettings (&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
      m_bFullscreen = false;
  }

  DWORD dwExStyle = WS_EX_APPWINDOW;
  DWORD dwStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

  if (!m_bFullscreen)
    dwStyle |= WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_VISIBLE;
  else
    dwStyle |= WS_POPUP;

  RECT WindowRect;
  WindowRect.top = 0;
  WindowRect.left = 0;
  WindowRect.right = m_uiResolutionX;
  WindowRect.bottom = m_uiResolutionY;

  if (!AdjustWindowRectEx (&WindowRect, dwStyle, false, dwExStyle))
    return;

  const ezUInt32 uiWinWidth = WindowRect.right - WindowRect.left;
  const ezUInt32 uiWinHeight= WindowRect.bottom - WindowRect.top;

  g_hWnd = CreateWindowEx (dwExStyle, wc.lpszClassName, wc.lpszClassName, dwStyle, 0, 0,
    uiWinWidth, uiWinHeight, NULL, NULL, wc.hInstance, NULL);

  if (g_hWnd == INVALID_HANDLE_VALUE)
    return;

  ShowWindow (g_hWnd, SW_SHOW);
  SetFocus (g_hWnd);
  SetForegroundWindow (g_hWnd);

  CreateContextOGL();
}

void SampleGameApp::DestroyAppWindow(void)
{
  DestroyContextOGL();

  if (g_hWnd)
  {
    if (m_bFullscreen)
      ChangeDisplaySettings (NULL, 0);

    DestroyWindow (g_hWnd);
    g_hWnd = NULL;
  }

  UnregisterClass (m_szAppName, GetModuleHandle(NULL));
}

void SampleGameApp::GameLoop()
{
  MSG msg;
  m_bActiveRenderLoop = true;

  while (m_bActiveRenderLoop)
  {
    if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))	
    {
      if (msg.message == WM_QUIT)
        break;

      TranslateMessage (&msg);
      DispatchMessage (&msg);
    }
    else
    {
      RenderSingleFrame();
      SwapBuffers(g_hDC);
      Sleep(10);
    }
  }
}




