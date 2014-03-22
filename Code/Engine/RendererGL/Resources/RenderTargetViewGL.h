#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

/// \brief OpenGL has no concept of render-target-views. Thus this class just contains configuration for bindings.
///
/// However, it could perform some checks and determines the GL bindings to be used.
/// Framebuffer objects are represented by ezGALRenderTargetConfigGL
class ezGALRenderTargetViewGL : public ezGALRenderTargetView
{
public:

  glBindingTarget GetBindingTarget() const;

private:

  friend class ezGALDeviceGL;
  friend class ezMemoryUtils;

  ezGALRenderTargetViewGL(const ezGALRenderTargetViewCreationDescription& Description);

  virtual ~ezGALRenderTargetViewGL();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) EZ_OVERRIDE;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) EZ_OVERRIDE;

  glBindingTarget m_BindingTarget;
};

#include <RendererGL/Resources/Implementation/RenderTargetViewGL_inl.h>