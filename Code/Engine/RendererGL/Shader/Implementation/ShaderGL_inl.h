
glProgramId ezGALShaderGL::GetGLShaderProgram() const
{
  return m_Program;
}

glShaderId ezGALShaderGL::GetGLVertexShader() const
{
  return m_VertexShader;
}

#if defined(EZ_RENDERERGL_GL4) || defined(EZ_RENDERERGL_GL3)
glShaderId ezGALShaderGL::GetGLGeometryShader() const
{
  return m_GeometryShader;
}
#endif

#ifdef EZ_RENDERERGL_GL4
glShaderId ezGALShaderGL::GetGLDomainShader() const
{
  return m_DomainShader;
}
glShaderId ezGALShaderGL::GetGLHullShader() const
{
  return m_HullShader;
}
glShaderId ezGALShaderGL::GetGLPixelShader() const
{
  return m_PixelShader;
}

glShaderId ezGALShaderGL::GetGLComputeShader() const
{
  return m_ComputeShader;
}
#endif