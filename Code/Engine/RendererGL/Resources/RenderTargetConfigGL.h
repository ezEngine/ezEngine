
#pragma once

#include <RendererFoundation/Resources/RenderTargetConfig.h>

class ezGALRenderTargetViewGL;
class ezGALDeviceGL;

/// \brief Every render target config maps to a OpenGL framebuffer-object.
class ezGALRenderTargetConfigGL : public ezGALRenderTargetConfig
{
public:

  /// \brief Returns OpenGL fbo-handle.
  glFramebuffer GetGLBufferHandle() const;

protected:

  friend class ezGALDeviceGL;
  friend class ezMemoryUtils;

  ezGALRenderTargetConfigGL(const ezGALRenderTargetConfigCreationDescription& Description);

  virtual ~ezGALRenderTargetConfigGL();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;


  /// Intern helper function to attach a texture to the bound framebuffer.
  ezResult AttachRenderTargetViewToFramebuffer(ezUInt32 index, const ezGALRenderTargetViewHandle& RenderTargetViewHandle, const ezGALDeviceGL* pDeviceGL);


  glFramebuffer m_FramebufferHandle;
};

#include <RendererGL/Resources/Implementation/RenderTargetConfigGL_inl.h>
