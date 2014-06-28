#include <RendererGL/PCH.h>
#include <RendererGL/Shader/ShaderGL.h>
#include <Foundation/Logging/Log.h>

#include <RendererGL/glew/glew.h>


ezGALShaderGL::ezGALShaderGL(const ezGALShaderCreationDescription& Description)
  : ezGALShader(Description)
  , m_Program(EZ_RENDERERGL_INVALID_ID)

  , m_VertexShader(EZ_RENDERERGL_INVALID_ID)
  , m_PixelShader(EZ_RENDERERGL_INVALID_ID)
#if defined(EZ_RENDERERGL_GL4) || defined(EZ_RENDERERGL_GL3)
  , m_GeometryShader(EZ_RENDERERGL_INVALID_ID)
#endif
#ifdef EZ_RENDERERGL_GL4
  , m_HullShader(EZ_RENDERERGL_INVALID_ID)
  , m_DomainShader(EZ_RENDERERGL_INVALID_ID)
  , m_ComputeShader(EZ_RENDERERGL_INVALID_ID)
#endif
{
}

ezGALShaderGL::~ezGALShaderGL()
{
}

ezResult ezGALShaderGL::CompileShader(glProgramId& dstProgram, glShaderId& dstShader, ezUInt32 glShaderType, const void* szRawSource)
{
  dstShader = glCreateShader(glShaderType);

  if (EZ_GL_CALL(glShaderSource, dstShader, 1, reinterpret_cast<const GLchar**>(&szRawSource), (const int*)nullptr) != EZ_SUCCESS)
    return EZ_FAILURE;

  if (EZ_GL_CALL(glCompileShader, dstShader) != EZ_SUCCESS)
    return EZ_FAILURE;


  // Additional error handling
  GLint shaderCompileStatus;
  if (EZ_GL_CALL(glGetShaderiv, dstShader, GL_COMPILE_STATUS, &shaderCompileStatus) != EZ_SUCCESS)
    return EZ_FAILURE;

  if (shaderCompileStatus == GL_FALSE)
  {
    GLint infologLength = 0;
    GLsizei charsWritten = 0;

    if (EZ_GL_CALL(glGetShaderiv, dstShader, GL_INFO_LOG_LENGTH, &infologLength) != EZ_SUCCESS)
      return EZ_FAILURE;

    if (infologLength > 0)
    {
      ezArrayPtr<char> pInfoLog = EZ_DEFAULT_NEW_ARRAY(char, infologLength);
      if (EZ_GL_CALL(glGetShaderInfoLog, dstShader, infologLength, &charsWritten, pInfoLog.GetPtr()) != EZ_SUCCESS)
        return EZ_FAILURE;

      pInfoLog[charsWritten] = '\0';

      if (strlen(pInfoLog.GetPtr()) > 0)
      {
        /// \todo We need a name for the shader, otherwise this log message is not very useful!
        ezLog::Error("Shader compile failed! Output: %s", pInfoLog.GetPtr());
      }

      EZ_DEFAULT_DELETE_ARRAY(pInfoLog);
    }
  }

  // Attach to 
  if (EZ_GL_CALL(glAttachShader, dstProgram, dstShader) != EZ_SUCCESS)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezGALShaderGL::InitPlatform(ezGALDevice* pDevice)
{
  /// \todo Maybe it would be a good idea to check if incoming shaders were already compiled
  // This way a "screenTri VS" wouldn't need to be compiled again and again.
  
  // Create program. 
  /// \todo This won't work as the macro always calls the version that uses vargs.
  //m_Program = EZ_GL_RET_CALL(glCreateProgram);
  m_Program = ezGLIsCall("glCreateProgram", glCreateProgram);
    
  // Create and compile shaders.
  if (m_Description.HasByteCodeForStage(ezGALShaderStage::VertexShader))
  {
    if (CompileShader(m_Program, m_VertexShader, GL_VERTEX_SHADER, m_Description.m_ByteCodes[ezGALShaderStage::VertexShader]->GetByteCode()) != EZ_SUCCESS)
      return EZ_FAILURE;
  }
  if (m_Description.HasByteCodeForStage(ezGALShaderStage::PixelShader))
  {
    if (CompileShader(m_Program, m_PixelShader, GL_FRAGMENT_SHADER, m_Description.m_ByteCodes[ezGALShaderStage::PixelShader]->GetByteCode()) != EZ_SUCCESS)
      return EZ_FAILURE;
  }
#if defined(EZ_RENDERERGL_GL4) || defined(EZ_RENDERERGL_GL3)
  if (m_Description.HasByteCodeForStage(ezGALShaderStage::GeometryShader))
  {
    if (CompileShader(m_Program, m_GeometryShader, GL_GEOMETRY_SHADER, m_Description.m_ByteCodes[ezGALShaderStage::GeometryShader]->GetByteCode()) != EZ_SUCCESS)
      return EZ_FAILURE;
  }
#endif
#ifdef EZ_RENDERERGL_GL4
  if (m_Description.HasByteCodeForStage(ezGALShaderStage::HullShader))
  {
    if (CompileShader(m_Program, m_HullShader, GL_TESS_CONTROL_SHADER, m_Description.m_ByteCodes[ezGALShaderStage::HullShader]->GetByteCode()) != EZ_SUCCESS)
      return EZ_FAILURE;
  }
  if (m_Description.HasByteCodeForStage(ezGALShaderStage::DomainShader))
  {
    if (CompileShader(m_Program, m_DomainShader, GL_TESS_EVALUATION_SHADER, m_Description.m_ByteCodes[ezGALShaderStage::DomainShader]->GetByteCode()) != EZ_SUCCESS)
      return EZ_FAILURE;
  }
  if (m_Description.HasByteCodeForStage(ezGALShaderStage::ComputeShader))
  {
    if (CompileShader(m_Program, m_ComputeShader, GL_COMPUTE_SHADER, m_Description.m_ByteCodes[ezGALShaderStage::ComputeShader]->GetByteCode()) != EZ_SUCCESS)
      return EZ_FAILURE;
  }
#endif
  

  // Link program.
 if (EZ_GL_CALL(glLinkProgram, m_Program) != EZ_SUCCESS)
    return EZ_FAILURE;

  GLint programLinked;
  if (EZ_GL_CALL(glGetProgramiv, m_Program, GL_LINK_STATUS, &programLinked) != EZ_SUCCESS)
    return EZ_FAILURE;

  if (programLinked == GL_FALSE)
  {
    GLint infologLength = 0;
    GLsizei charsWritten = 0;

    if (EZ_GL_CALL(glGetProgramiv, m_Program, GL_INFO_LOG_LENGTH, &infologLength) != EZ_SUCCESS)
      return EZ_FAILURE;
    
    if (infologLength > 0)
    {
      ezArrayPtr<char> pInfoLog = EZ_DEFAULT_NEW_ARRAY(char, infologLength);
      if (EZ_GL_CALL(glGetProgramInfoLog, m_Program, infologLength, &charsWritten, pInfoLog.GetPtr()) != EZ_SUCCESS)
        return EZ_FAILURE;

      pInfoLog[charsWritten] = '\0';

      if (strlen(pInfoLog.GetPtr()) > 0)
      {
        /// \todo We need a name for the shader, otherwise this log message is not very useful!
        ezLog::Error("Shader program linking failed! Output: %s", pInfoLog.GetPtr());
      }
    }

    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezGALShaderGL::DeInitPlatform(ezGALDevice* pDevice)
{
  if (m_Program != EZ_RENDERERGL_INVALID_ID)
  {
    if (EZ_GL_CALL(glDeleteProgram, m_Program) != EZ_SUCCESS)
      return EZ_FAILURE;
  }

  if (m_VertexShader != EZ_RENDERERGL_INVALID_ID)
  {
    if (EZ_GL_CALL(glDeleteShader, m_VertexShader) != EZ_SUCCESS)
      return EZ_FAILURE;
  }
  if (m_PixelShader != EZ_RENDERERGL_INVALID_ID)
  {
    if (EZ_GL_CALL(glDeleteShader, m_PixelShader) != EZ_SUCCESS)
      return EZ_FAILURE;
  }
#if defined(EZ_RENDERERGL_GL4) || defined(EZ_RENDERERGL_GL3)
  if (m_GeometryShader != EZ_RENDERERGL_INVALID_ID)
  {
    if (EZ_GL_CALL(glDeleteShader, m_GeometryShader) != EZ_SUCCESS)
      return EZ_FAILURE;
  }
#endif
#ifdef EZ_RENDERERGL_GL4
  if (m_HullShader != EZ_RENDERERGL_INVALID_ID)
  {
    if (EZ_GL_CALL(glDeleteShader, m_HullShader) != EZ_SUCCESS)
      return EZ_FAILURE;
  }
  if (m_DomainShader != EZ_RENDERERGL_INVALID_ID)
  {
    if (EZ_GL_CALL(glDeleteShader, m_DomainShader) != EZ_SUCCESS)
      return EZ_FAILURE;
  }
  if (m_ComputeShader != EZ_RENDERERGL_INVALID_ID)
  {
    if (EZ_GL_CALL(glDeleteShader, m_ComputeShader) != EZ_SUCCESS)
      return EZ_FAILURE;
  }
#endif

  return EZ_SUCCESS;
}
