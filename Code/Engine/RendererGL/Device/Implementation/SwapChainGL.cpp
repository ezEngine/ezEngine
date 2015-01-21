#include <RendererGL/PCH.h>
#include <RendererGL/Device/DeviceGL.h>
#include <RendererGL/Device/SwapChainGL.h>
#include <RendererGL/Resources/RenderTargetConfigGL.h>

#include <Foundation/Logging/Log.h>
#include <System/Window/Window.h>

#include <RendererGL/glew/glew.h>
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <RendererGL/glew/wglew.h>
#else
  #include <RendererGL/glew/glxew.h>
#endif


ezGALSwapChainGL::ezGALSwapChainGL(const ezGALSwapChainCreationDescription& Description)
  : ezGALSwapChain(Description)
{
}

ezGALSwapChainGL::~ezGALSwapChainGL()
{
}

void ezGALSwapChainGL::SetVSync(bool active)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (WGL_EXT_swap_control)
  {
    EZ_GL_CALL(wglSwapIntervalEXT, active ? 1 : 0);
  }
#else
  EZ_ASSERT_NOT_IMPLEMENTED;
#endif
}

ezResult ezGALSwapChainGL::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceGL* pDeviceGL = static_cast<ezGALDeviceGL*>(pDevice);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  CreateContextWindows();
#else
  EZ_ASSERT_NOT_IMPLEMENTED;
#endif

  // Device needs a context to be fully initialized.
  if (pDeviceGL->EnsureInternOpenGLInit() != EZ_SUCCESS)
    return EZ_FAILURE;

  SetVSync(m_Description.m_bVerticalSynchronization);


  // It is not possible to use back or depth/stencil buffer as texture. Therefore the texture handles will stay invalid to signalize that the hardware back buffer is meant.
  ezGALRenderTargetConfigCreationDescription RTConfigDesc;
  RTConfigDesc.m_bHardwareBackBuffer = true;
  ezGALRenderTargetConfigHandle hRenderTargetConfig = pDevice->CreateRenderTargetConfig(RTConfigDesc);
  EZ_ASSERT_DEV(!hRenderTargetConfig.IsInvalidated(), "Couldn't create backbuffer render target config!");


  SetBackBufferObjects(hRenderTargetConfig, ezGALTextureHandle(), ezGALTextureHandle());

  return EZ_SUCCESS;
}

ezResult ezGALSwapChainGL::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALSwapChain::DeInitPlatform(pDevice);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  DestroyContextWindows();
#endif

  return EZ_SUCCESS;
}

void ezGALSwapChainGL::SwapBuffers(ezGALDevice* pDevice)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (!::SwapBuffers(GetWindowDC()))
  {
    ezLog::Error("Failed to swap buffers for the window \"%p\".", m_Description.m_pWindow->GetNativeWindowHandle());
  }
#else
  EZ_ASSERT_NOT_IMPLEMENTED;
  // something with glXSwapBuffers
#endif
}

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

ezResult ezGALSwapChainGL::CreateContextWindows()
{
  EZ_LOG_BLOCK("ezGALSwapChainGL::CreateContextWindows");

  int iColorBits = 24;
  int iDepthBits = 24;
  int iBPC = 8;

  HWND hWnd = m_Description.m_pWindow->GetNativeWindowHandle();

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

  wglMakeCurrent(m_hDC, m_hRC);

  ezLog::Success("OpenGL graphics context is initialized.");

  return EZ_SUCCESS;

failure:
  ezLog::Error("Failed to initialize the graphics context.");

  DestroyContextWindows();
  return EZ_FAILURE;
}

ezResult ezGALSwapChainGL::DestroyContextWindows()
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

    ReleaseDC(m_Description.m_pWindow->GetNativeWindowHandle(), m_hDC);
    m_hDC = NULL;
  }

  ezLog::Success("OpenGL graphics context is destroyed.");

  return EZ_SUCCESS;
}

#endif