
#pragma once

#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererGL/Basics.h>

class EZ_RENDERERGL_DLL ezGALShaderGL : public ezGALShader
{
public:

  /// \brief Returns the handle of the linked OpenGL shader program.
  EZ_FORCE_INLINE glProgramId GetGLShaderProgram() const;

  EZ_FORCE_INLINE glShaderId GetGLVertexShader() const;

  EZ_FORCE_INLINE glShaderId GetGLPixelShader() const;

#if defined(EZ_RENDERERGL_GL4) || defined(EZ_RENDERERGL_GL3)

  EZ_FORCE_INLINE glShaderId GetGLGeometryShader() const;

#endif

#ifdef EZ_RENDERERGL_GL4

  EZ_FORCE_INLINE glShaderId GetGLHullShader() const;

  EZ_FORCE_INLINE glShaderId GetGLDomainShader() const;

  EZ_FORCE_INLINE glShaderId GetGLComputeShader() const;

#endif

protected:

  friend class ezGALDeviceGL;
  friend class ezMemoryUtils;

  ezGALShaderGL(const ezGALShaderCreationDescription& description);

  virtual ~ezGALShaderGL();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  /// Intern helper function to compile and attach a shader.
  static ezResult CompileShader(glProgramId& dstProgram, glShaderId& dstShader, ezUInt32 glShaderType, const void* szRawSource);

  glProgramId m_Program;

  glShaderId m_VertexShader;
  glShaderId m_PixelShader;

#if defined(EZ_RENDERERGL_GL4) || defined(EZ_RENDERERGL_GL3)
  glShaderId m_GeometryShader;
#endif

#ifdef EZ_RENDERERGL_GL4
  glShaderId m_HullShader;
  glShaderId m_DomainShader;
  glShaderId m_ComputeShader;
#endif

};

#include <RendererGL/Shader/Implementation/ShaderGL_inl.h>
