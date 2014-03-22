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

  // Generate texture of the swap chain.
  ezGALTextureCreationDescription TexDesc;
  TexDesc.m_uiWidth = m_Description.m_pWindow->GetCreationDescription().m_ClientAreaSize.width;
  TexDesc.m_uiHeight = m_Description.m_pWindow->GetCreationDescription().m_ClientAreaSize.height;
  TexDesc.m_Format = m_Description.m_BackBufferFormat;
  TexDesc.m_SampleCount = m_Description.m_SampleCount;
  TexDesc.m_bCreateRenderTarget = true;

  if (m_Description.m_bAllowScreenshots)
    TexDesc.m_ResourceAccess.m_bReadBack = true;

  ezGALTextureHandle hBackBufferTexture = pDevice->CreateTexture(TexDesc);
  EZ_ASSERT(!hBackBufferTexture.IsInvalidated(), "Couldn't create backbuffer texture object!");


  // Create rendertarget view
  ezGALRenderTargetViewCreationDescription RTViewDesc;
  RTViewDesc.m_bReadOnly = true;
  RTViewDesc.m_hTexture = hBackBufferTexture;
  RTViewDesc.m_RenderTargetType = ezGALRenderTargetType::Color;
  RTViewDesc.m_uiFirstSlice = 0;
  RTViewDesc.m_uiMipSlice = 0;
  RTViewDesc.m_uiSliceCount = 1;

  ezGALRenderTargetViewHandle hBackBufferRenderTargetView = pDevice->CreateRenderTargetView(RTViewDesc);
  EZ_ASSERT(!hBackBufferRenderTargetView.IsInvalidated(), "Couldn't create backbuffer rendertarget view!");

  SetBackBufferObjects(hBackBufferTexture, hBackBufferRenderTargetView);


  // Create render target view config that contains only the backbuffer for copy operation later.
  ezGALRenderTargetConfigCreationDescription RTConfigDesc;
  RTConfigDesc.m_hColorTargets[0] = hBackBufferRenderTargetView;
  RTConfigDesc.m_uiColorTargetCount = 1;
  m_BackBufferRTConfig = pDevice->CreateRenderTargetConfig(RTConfigDesc);

  return EZ_SUCCESS;
}

ezResult ezGALSwapChainGL::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALSwapChain::DeInitPlatform(pDevice);

  return EZ_SUCCESS;
}

void ezGALSwapChainGL::SwapBuffers(ezGALDevice* pDevice)
{
  // Yes this hurts badly, but we need to copy the backbuffer texture to the actual buffer since there is now way the backbuffer could be used as texture otherwise!
  // Todo: Use copy function provided by pDevice, reset target afterwards
 /* EZ_GL_CALL(glBindFramebuffer,GL_DRAW_FRAMEBUFFER, 0);
  
  const ezGALRenderTargetConfigGL* pRenderTargetConfig = static_cast<const ezGALRenderTargetConfigGL*>(pDevice->GetRenderTargetConfig(m_BackBufferRTConfig));
  EZ_ASSERT(pRenderTargetConfig != NULL, "Backbuffer rendertarget-config is NULL!");

  EZ_GL_CALL(glBindFramebuffer, GL_READ_FRAMEBUFFER, pRenderTargetConfig->GetGLBufferHandle());

  EZ_GL_CALL(glBlitFramebuffer, 0, 0, m_Description.m_pWindow->GetCreationDescription().m_ClientAreaSize.width, m_Description.m_pWindow->GetCreationDescription().m_ClientAreaSize.height,
                                0, 0, m_Description.m_pWindow->GetCreationDescription().m_ClientAreaSize.width, m_Description.m_pWindow->GetCreationDescription().m_ClientAreaSize.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

                                */
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

