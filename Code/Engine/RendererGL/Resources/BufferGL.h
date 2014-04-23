
#pragma once

#include <RendererFoundation/Resources/Buffer.h>

class EZ_RENDERERGL_DLL ezGALBufferGL : public ezGALBuffer
{
public:

  /// \brief Returns OpenGL buffer handle.
  ///
  /// Either a VertexArray or a VertexBuffer (aka ArrayObject).
  glBufferId GetGLBufferHandle() const;

protected:

  friend class ezGALDeviceGL;
  friend class ezMemoryUtils;

  ezGALBufferGL(const ezGALBufferCreationDescription& Description);

  virtual ~ezGALBufferGL();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, const void* pInitialData) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;


  /// Internal OpenGL buffer handle. Either a VertexArray or a VertexBuffer (aka ArrayObject).
  glBufferId m_BufferHandle;
};

#include <RendererGL/Resources/Implementation/BufferGL_inl.h>
