#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

/// \brief OpenGL has no concept of render-target-views. Thus this class just contains configuration for bindings.
///
/// If a invalid texture handle is given, it is assumed that this represents the hardware backbuffer.
/// Framebuffer objects are represented by ezGALRenderTargetConfigGL.
class ezGALRenderTargetViewGL : public ezGALRenderTargetView
{
public:

  glBindingTarget GetBindingTarget() const;

private:

  friend class ezGALDeviceGL;
  friend class ezMemoryUtils;

  ezGALRenderTargetViewGL(const ezGALRenderTargetViewCreationDescription& Description);

  virtual ~ezGALRenderTargetViewGL();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  glBindingTarget m_BindingTarget;
};

#include <RendererGL/Resources/Implementation/RenderTargetViewGL_inl.h>