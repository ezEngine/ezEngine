#include "Window.h"
#include <gl/GL.h>
#include <Foundation/Logging/Log.h>

GameWindow::GameWindow()
{
  m_CreationDescription.m_ClientAreaSize.width = 500;
  m_CreationDescription.m_ClientAreaSize.height = 500;
  m_CreationDescription.m_Title = "SampleApp_Game";
  m_CreationDescription.m_bFullscreenWindow = false;
  m_CreationDescription.m_bResizable = true;
  Initialize();
  CreateGraphicsContext();
}

GameWindow::~GameWindow()
{
  DestroyGraphicsContext();
  Destroy();
}

void GameWindow::OnResizeMessage(const ezSizeU32& newWindowSize)
{
  ezLog::Info("Resolution changed to %i * %i", newWindowSize.width, newWindowSize.height);

  m_CreationDescription.m_ClientAreaSize = newWindowSize;
}

ezSizeU32 GameWindow::GetResolution() const
{
  return m_CreationDescription.m_ClientAreaSize;
}

void GameWindow::PresentFrame()
{
  ::SwapBuffers(m_hDC);
}

ezResult GameWindow::CreateGraphicsContext()
{
  return CreateContextOpenGL();
}

ezResult GameWindow::DestroyGraphicsContext()
{
  return DestroyContextOpenGL();
}

ezResult GameWindow::CreateContextOpenGL()
{
  EZ_LOG_BLOCK("GameWindow::CreateContextOpenGL");

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

ezResult GameWindow::DestroyContextOpenGL()
{
  EZ_LOG_BLOCK("GameWindow::DestroyContextOpenGL");

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

