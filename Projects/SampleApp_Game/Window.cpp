#include "Window.h"
#include <InputWindows/InputDeviceWindows.h>
#include <gl/GL.h>

GameWindow::GameWindow() :
   m_hDC(0),
   m_hRC(0)
{
  m_CreationDescription.m_ClientAreaSize.width = 500;
  m_CreationDescription.m_ClientAreaSize.height = 500;
  m_CreationDescription.m_Title = "SampleApp_Game";
  Initialize();
  CreateContextOGL();
}

GameWindow::~GameWindow()
{
  DestroyContextOGL();
  Destroy();
}

void GameWindow::OnResizeMessage(const ezSizeU32& newWindowSize)
{
  m_CreationDescription.m_ClientAreaSize = newWindowSize;
}

void GameWindow::CreateContextOGL()
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

  HDC hDC = GetDC(GetNativeWindowHandle());

  if (hDC == NULL)
    return;

  int iPixelformat = ChoosePixelFormat(hDC, &pfd);
  if (iPixelformat == 0)
    return;

  if (!SetPixelFormat(hDC, iPixelformat, &pfd))
    return;

  HGLRC hRC = wglCreateContext(hDC);
  if (hRC == NULL)
    return;

  if (!wglMakeCurrent(hDC, hRC))
    return;

  m_hDC = hDC;
  m_hRC = hRC;

  SetFocus(GetNativeWindowHandle());
  SetForegroundWindow(GetNativeWindowHandle());
}

void GameWindow::DestroyContextOGL()
{
  if (m_hRC)
  {
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(m_hRC);
    m_hRC = NULL;
  }

  if (m_hDC)
  {
    ReleaseDC(GetNativeWindowHandle(), m_hDC);
    m_hDC = NULL;
  }
}

void GameWindow::SwapBuffers()
{
  ::SwapBuffers(m_hDC);
}

ezSizeU32 GameWindow::GetResolution() const
{
  return m_CreationDescription.m_ClientAreaSize;
}

void GameWindow::OnWindowMessage(HWND hWnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
  ezInputDeviceWindows::GetDevice()->WindowMessage(hWnd, Msg, WParam, LParam);
}