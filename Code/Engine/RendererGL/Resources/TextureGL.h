#pragma once

#include <RendererFoundation/Resources/Texture.h>

class ezGALTextureGL : public ezGALTexture
{
public:

  /// \brief Returns OpenGL texture handle.
  glTextureId GetGLTextureHandle() const;

  /// \brief Returns OpenGL texture binding point.
  glBindingTarget GetGLTextureBindingType() const;


protected:

  friend class ezGALDeviceGL;
  friend class ezMemoryUtils;

  ezGALTextureGL(const ezGALTextureCreationDescription& Description);

  ~ezGALTextureGL();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, const ezArrayPtr<ezGALSystemMemoryDescription>* pInitialData) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  glTextureId m_TextureHandle;
};

#include <RendererGL/Resources/Implementation/TextureGL_inl.h>
