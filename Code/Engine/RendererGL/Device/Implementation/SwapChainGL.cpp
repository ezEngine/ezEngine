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


ezResult ezGALSwapChainGL::InitPlatform(ezGALDevice* pDevice)
{
  // \todo: Currently the init is done by the ezWindow.
  // This needs to be moved here. Also it has to be investigated how to create multiple "swapchains" and do initialization on linux.
  // Workarounds needs to be found for the framebuffer access limitations: At least in wGL it is usually not possible to access the default framebuffer like a texture (both depth and color)

  // V-Sync stuff can be done using WGL_EXT_swap_control/GLX_EXT_swap_control http://www.opengl.org/wiki/Swap_Interval 

  // Windows V-Sync off:
  // wglSwapIntervalEXT(0);
  // Windows V-Sync on:
  // wglSwapIntervalEXT(1);


  // It is not possible to use back or depth/stencil buffer as texture. Therefore the texture handles will stay invalid to signalize that the hardware back buffer is meant.

  ezGALRenderTargetConfigCreationDescription RTConfigDesc;
  RTConfigDesc.m_bHardwareBackBuffer = true;
  ezGALRenderTargetConfigHandle hRenderTargetConfig = pDevice->CreateRenderTargetConfig(RTConfigDesc);
  EZ_ASSERT(!hRenderTargetConfig.IsInvalidated(), "Couldn't create backbuffer render target config!");


  SetBackBufferObjects(hRenderTargetConfig, ezGALTextureHandle(), ezGALTextureHandle());

  return EZ_SUCCESS;
}

ezResult ezGALSwapChainGL::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALSwapChain::DeInitPlatform(pDevice);

  return EZ_SUCCESS;
}

void ezGALSwapChainGL::SwapBuffers(ezGALDevice* pDevice)
{
#ifdef EZ_PLATFORM_WINDOWS
  if (!::SwapBuffers(m_Description.m_pWindow->GetWindowDC()))
  {
    ezLog::Error("Failed to swap buffers for the windows with number \"%i\".", m_Description.m_pWindow->GetCreationDescription().m_uiWindowNumber);
  }
#else
  EZ_ASSERT_NOT_IMPLEMENTED;
  // something with glXSwapBuffers
#endif
}

