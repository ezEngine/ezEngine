
#include <RendererGL/PCH.h>
#include <RendererGL/Device/DeviceGL.h>
#include <RendererGL/Device/SwapChainGL.h>
#include <RendererGL/Shader/ShaderGL.h>
#include <RendererGL/Resources/BufferGL.h>
#include <RendererGL/Resources/TextureGL.h>
#include <RendererGL/Resources/RenderTargetConfigGL.h>
#include <RendererGL/Resources/RenderTargetViewGL.h>
#include <RendererGL/Shader/VertexDeclarationGL.h>
#include <RendererGL/State/StateGL.h>
#include <RendererGL/Resources/ResourceViewGL.h>
#include <RendererGL/Context/ContextGL.h>

#include <RendererGL/glew/glew.h>

#include <Foundation/Logging/Log.h>
#include <System/Window/Window.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Types/ArrayPtr.h>


ezGALDeviceGL::ezGALDeviceGL(const ezGALDeviceCreationDescription& Description)
  : ezGALDevice(Description),
  m_bGLInitialized(false)
{
}

ezGALDeviceGL::~ezGALDeviceGL()
{
}

// OpenGL specific functions.

// Debug output function used for ezGALDeviceGL::SetupDebugOutput.
static void GLEWAPIENTRY DebugOutput(GLenum Source,
                                  GLenum Type,
                                  GLuint uiId,
                                  GLenum Severity,
                                  GLsizei iLength,
                                  const GLchar* szMessage,
                                  const void* pUserParam)
{
  ezLogMsgType::Enum eventType = ezLogMsgType::InfoMsg;
  ezString debSource, debType, debSev;

  if (Source == GL_DEBUG_SOURCE_API_ARB)
    debSource = "OpenGL";
  else if (Source == GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB)
    debSource = "Windows";
  else if (Source == GL_DEBUG_SOURCE_SHADER_COMPILER_ARB)
    debSource = "Shader Compiler";
  else if (Source == GL_DEBUG_SOURCE_THIRD_PARTY_ARB)
    debSource = "Third Party";
  else if (Source == GL_DEBUG_SOURCE_APPLICATION_ARB)
    debSource = "Application";
  else if (Source == GL_DEBUG_SOURCE_OTHER_ARB)
    debSource = "Other";

  if (Type == GL_DEBUG_TYPE_ERROR_ARB)
  {
    eventType = ezLogMsgType::ErrorMsg;
    debType = "error";
  }
  else if (Type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB)
  {
    eventType = ezLogMsgType::WarningMsg;
    debType = "deprecated behavior";
  }
  else if (Type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB)
  {
    eventType = ezLogMsgType::SeriousWarningMsg;
    debType = "undefined behavior";
  }
  else if (Type == GL_DEBUG_TYPE_PORTABILITY_ARB)
  {
    eventType = ezLogMsgType::WarningMsg;
    debType = "portability";
  }
  else if (Type == GL_DEBUG_TYPE_PERFORMANCE_ARB)
  {
    eventType = ezLogMsgType::WarningMsg;
    debType = "performance";
  }
  else if (Type == GL_DEBUG_TYPE_OTHER_ARB)
  {
    debType = "message";
  }

  EZ_ASSERT_ALWAYS(Severity != GL_DEBUG_SEVERITY_HIGH_ARB, "Fatal GL error occurred: %s: %s(high) %d: %s", debSource.GetData(), debType.GetData(), uiId, szMessage);

  if (Severity == GL_DEBUG_SEVERITY_MEDIUM_ARB)
    debSev = "medium";
  else if (Severity == GL_DEBUG_SEVERITY_LOW_ARB)
    debSev = "low";

  switch (eventType)
  {
  case ezLogMsgType::WarningMsg:
    ezLog::Warning("%s: %s(%s) %d: %s\n", debSource.GetData(), debType.GetData(), debSev.GetData(), uiId, szMessage);
    break;
  case ezLogMsgType::SeriousWarningMsg:
    ezLog::SeriousWarning("%s: %s(%s) %d: %s\n", debSource.GetData(), debType.GetData(), debSev.GetData(), uiId, szMessage);
    break;
  case ezLogMsgType::InfoMsg:
    ezLog::Info("%s: %s(%s) %d: %s\n", debSource.GetData(), debType.GetData(), debSev.GetData(), uiId, szMessage);
    break;
  default:
    break;
  }
}

void ezGALDeviceGL::SetupDebugOutput(ezGALDeviceGL::DebugMessageSeverity::Enum minMessageSeverity)
{
  if (!GLEW_KHR_debug)
  {
    ezLog::Warning("OpenGL Extension KHR_DEBUG is not supported. Debug output is not possible.");
    return;
  }

  EZ_GL_CALL(glEnable, GL_DEBUG_OUTPUT);
  EZ_GL_CALL(glEnable, GL_DEBUG_OUTPUT_SYNCHRONOUS);

  switch (minMessageSeverity)
  {
  // Fall-through is intended since every severity level needs to be activated separately!
  case DebugMessageSeverity::LOW:
    EZ_GL_CALL(glDebugMessageControl, GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_TRUE);
  case DebugMessageSeverity::MEDIUM:
    EZ_GL_CALL(glDebugMessageControl, GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
  case DebugMessageSeverity::HIGH:
    EZ_GL_CALL(glDebugMessageControl, GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
  }

  EZ_GL_CALL(glDebugMessageCallback, &DebugOutput, (const void*) nullptr);
}


glBufferId ezGALDeviceGL::GetUnusedGLBufferId()
{
  if (m_BufferHandlePool.GetCount() == 0)
    EnsureGLBufferPoolSize();

  glBufferId buffer = m_BufferHandlePool.PeekBack();
  m_BufferHandlePool.PopBack();

  return buffer;
}

void ezGALDeviceGL::EnsureGLBufferPoolSize(ezUInt32 uiPoolSize)
{
  if (m_BufferHandlePool.GetCount() >= uiPoolSize)
    return;

  ezUInt32 uiOldCount = m_BufferHandlePool.GetCount();
  ezUInt32 uiNumNewBuffers = uiPoolSize - uiOldCount;
  m_BufferHandlePool.SetCount(uiPoolSize);

  EZ_GL_CALL(glGenBuffers, uiNumNewBuffers, static_cast<ezArrayPtr<glBufferId>>(m_BufferHandlePool).GetPtr() + uiOldCount);
}

glTextureId ezGALDeviceGL::GetUnusedGLTextureId()
{
  if (m_TextureHandlePool.GetCount() == 0)
    EnsureGLTexturePoolSize();

  glTextureId buffer = m_TextureHandlePool.PeekBack();
  m_TextureHandlePool.PopBack();

  return buffer;
}

void ezGALDeviceGL::EnsureGLTexturePoolSize(ezUInt32 uiPoolSize)
{
  if (m_TextureHandlePool.GetCount() >= uiPoolSize)
    return;

  ezUInt32 uiOldCount = m_TextureHandlePool.GetCount();
  ezUInt32 uiNumNewTextures = uiPoolSize - uiOldCount;
  m_TextureHandlePool.SetCount(uiPoolSize);

  EZ_GL_CALL(glGenTextures, uiNumNewTextures, static_cast<ezArrayPtr<glBufferId>>(m_TextureHandlePool).GetPtr() + uiOldCount);
}

// Init & shutdown functions.


ezResult ezGALDeviceGL::InitPlatform() 
{
  EZ_LOG_BLOCK("ezGALDeviceGL::InitPlatform() ");

  // Can't do anything OpenGL related here since glewInit will be called in the primary swap chain

  ezProjectionDepthRange::Default = ezProjectionDepthRange::MinusOneToOne;

  return EZ_SUCCESS;
}

ezResult ezGALDeviceGL::EnsureInternOpenGLInit()
{
  if (m_bGLInitialized)
    return EZ_SUCCESS;

  // Fill lookup table.
  FillFormatLookupTable();

  // Load OpenGL functions.
  GLenum glewErrorCode = glewInit();
  if (glewErrorCode != GLEW_OK)
  {
    ezLog::Error("glewInit failed with: %s", glewGetErrorString(glewErrorCode));
    return EZ_FAILURE;
  }

  // Create primary swap context.
  m_pPrimaryContext = EZ_DEFAULT_NEW(ezGALContextGL)(this);

  // Check OpenGL version support.
#if defined(EZ_RENDERERGL_GL4)
  if (!GLEW_VERSION_4_4)
  {
    ezLog::Error("No OpenGL 4.4 Core Profile support!");
    return EZ_FAILURE;
  }
#elif defined(EZ_RENDERER_GL3)
  if (!GLEW_VERSION_3_3)
  {
    ezLog::Error("No OpenGL 3.3 Core Profile support!");
    return EZ_FAILURE;
  }
#elif defined(EZ_RENDERER_GLES3)
  if (!GLEW_VERSION_3_0)
  {
    ezLog::Error("No OpenGL ES 3 Core Profile support!");
    return EZ_FAILURE;
  }
#endif

  // Debug messaging.
  if (m_Description.m_bDebugDevice)
  {
    SetupDebugOutput();
  }

  /// \todo Get features of the device (depending on feature level, CheckFormat* functions etc.)

  m_bGLInitialized = true;

  return EZ_SUCCESS;
}

ezResult ezGALDeviceGL::ShutdownPlatform()
{
  if (EZ_GL_CALL(glDeleteBuffers, m_BufferHandlePool.GetCount(), static_cast<ezArrayPtr<glBufferId>>(m_BufferHandlePool).GetPtr()) != EZ_SUCCESS)
    return EZ_FAILURE;
  m_BufferHandlePool.Clear();

  if (EZ_GL_CALL(glDeleteTextures, m_TextureHandlePool.GetCount(), static_cast<ezArrayPtr<glBufferId>>(m_TextureHandlePool).GetPtr()) != EZ_SUCCESS)
    return EZ_FAILURE;
  m_TextureHandlePool.Clear();

  EZ_DEFAULT_DELETE(m_pPrimaryContext);

  return EZ_SUCCESS;
}


// State creation functions

ezGALBlendState* ezGALDeviceGL::CreateBlendStatePlatform(const ezGALBlendStateCreationDescription& Description)
{
  return DefaultCreate<ezGALBlendStateGL, ezGALBlendStateCreationDescription>(Description);
}

void ezGALDeviceGL::DestroyBlendStatePlatform(ezGALBlendState* pBlendState)
{
  DefaultDestroy<ezGALBlendStateGL, ezGALBlendState>(pBlendState);
}

ezGALDepthStencilState* ezGALDeviceGL::CreateDepthStencilStatePlatform(const ezGALDepthStencilStateCreationDescription& Description)
{
  return DefaultCreate<ezGALDepthStencilStateGL, ezGALDepthStencilStateCreationDescription>(Description);
}

void ezGALDeviceGL::DestroyDepthStencilStatePlatform(ezGALDepthStencilState* pDepthStencilState)
{
  DefaultDestroy<ezGALDepthStencilStateGL, ezGALDepthStencilState>(pDepthStencilState);
}

ezGALRasterizerState* ezGALDeviceGL::CreateRasterizerStatePlatform(const ezGALRasterizerStateCreationDescription& Description)
{
  return DefaultCreate<ezGALRasterizerStateGL, ezGALRasterizerStateCreationDescription>(Description);
}

void ezGALDeviceGL::DestroyRasterizerStatePlatform(ezGALRasterizerState* pRasterizerState)
{
  DefaultDestroy<ezGALRasterizerStateGL, ezGALRasterizerState>(pRasterizerState);
}

ezGALSamplerState* ezGALDeviceGL::CreateSamplerStatePlatform(const ezGALSamplerStateCreationDescription& Description)
{
  return DefaultCreate<ezGALSamplerStateGL, ezGALSamplerStateCreationDescription>(Description);
}

void ezGALDeviceGL::DestroySamplerStatePlatform(ezGALSamplerState* pSamplerState)
{
  DefaultDestroy<ezGALSamplerStateGL, ezGALSamplerState>(pSamplerState);
}


// Resource creation functions

ezGALShader* ezGALDeviceGL::CreateShaderPlatform(const ezGALShaderCreationDescription& Description)
{
  return DefaultCreate<ezGALShaderGL, ezGALShaderCreationDescription>(Description);
}

void ezGALDeviceGL::DestroyShaderPlatform(ezGALShader* pShader)
{
  DefaultDestroy<ezGALShaderGL, ezGALShader>(pShader);
}

ezGALBuffer* ezGALDeviceGL::CreateBufferPlatform(const ezGALBufferCreationDescription& Description, const void* pInitialData)
{
  return DefaultCreate<ezGALBufferGL, ezGALBufferCreationDescription>(Description, pInitialData);
}

void ezGALDeviceGL::DestroyBufferPlatform(ezGALBuffer* pBuffer)
{
  DefaultDestroy<ezGALBufferGL, ezGALBuffer>(pBuffer);
}

ezGALTexture* ezGALDeviceGL::CreateTexturePlatform(const ezGALTextureCreationDescription& Description, const ezArrayPtr<ezGALSystemMemoryDescription>* pInitialData)
{
  return DefaultCreate<ezGALTextureGL, ezGALTextureCreationDescription>(Description, pInitialData);
}

void ezGALDeviceGL::DestroyTexturePlatform(ezGALTexture* pTexture)
{
  DefaultDestroy<ezGALTextureGL, ezGALTexture>(pTexture);
}

ezGALResourceView* ezGALDeviceGL::CreateResourceViewPlatform(const ezGALResourceViewCreationDescription& Description)
{
  return DefaultCreate<ezGALResourceViewGL, ezGALResourceViewCreationDescription>(Description);
}

void ezGALDeviceGL::DestroyResourceViewPlatform(ezGALResourceView* pResourceView)
{
  DefaultDestroy<ezGALResourceViewGL, ezGALResourceView>(pResourceView);
}

ezGALRenderTargetView* ezGALDeviceGL::CreateRenderTargetViewPlatform(const ezGALRenderTargetViewCreationDescription& Description)
{
  return DefaultCreate<ezGALRenderTargetViewGL, ezGALRenderTargetViewCreationDescription>(Description);
}

void ezGALDeviceGL::DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView)
{
  ezGALRenderTargetViewGL* pGLRenderTargetView = static_cast<ezGALRenderTargetViewGL*>(pRenderTargetView);
  pGLRenderTargetView->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pGLRenderTargetView);
}


// Other rendering creation functions

ezGALSwapChain* ezGALDeviceGL::CreateSwapChainPlatform(const ezGALSwapChainCreationDescription& Description)
{
  return DefaultCreate<ezGALSwapChainGL, ezGALSwapChainCreationDescription>(Description);
}

void ezGALDeviceGL::DestroySwapChainPlatform(ezGALSwapChain* pSwapChain)
{
  DefaultDestroy<ezGALSwapChainGL, ezGALSwapChain>(pSwapChain);
}

ezGALFence* ezGALDeviceGL::CreateFencePlatform()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
}

void ezGALDeviceGL::DestroyFencePlatform(ezGALFence* pFence)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezGALQuery* ezGALDeviceGL::CreateQueryPlatform(const ezGALQueryCreationDescription& Description)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
}

void ezGALDeviceGL::DestroyQueryPlatform(ezGALQuery* pQuery)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezGALRenderTargetConfig* ezGALDeviceGL::CreateRenderTargetConfigPlatform(const ezGALRenderTargetConfigCreationDescription& Description)
{
  return DefaultCreate<ezGALRenderTargetConfigGL, ezGALRenderTargetConfigCreationDescription>(Description);
}

void ezGALDeviceGL::DestroyRenderTargetConfigPlatform(ezGALRenderTargetConfig* pRenderTargetConfig)
{
  DefaultDestroy<ezGALRenderTargetConfigGL, ezGALRenderTargetConfig>(pRenderTargetConfig);
}

ezGALVertexDeclaration* ezGALDeviceGL::CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description)
{
  return DefaultCreate<ezGALVertexDeclarationGL, ezGALVertexDeclarationCreationDescription>(Description);
}

void ezGALDeviceGL::DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration)
{
  DefaultDestroy<ezGALVertexDeclarationGL, ezGALVertexDeclaration>(pVertexDeclaration);
}


void ezGALDeviceGL::GetQueryDataPlatform(ezGALQuery* pQuery, ezUInt64* puiRendererdPixels)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}





// Swap chain functions

void ezGALDeviceGL::PresentPlatform(ezGALSwapChain* pSwapChain)
{
  ezGALSwapChainGL* pSwapChainGL = static_cast<ezGALSwapChainGL*>(pSwapChain);
  pSwapChainGL->SwapBuffers(this);
}

// Misc functions

void ezGALDeviceGL::BeginFramePlatform()
{
}

void ezGALDeviceGL::EndFramePlatform()
{
}

void ezGALDeviceGL::FlushPlatform()
{
  glFlush();
}

void ezGALDeviceGL::FinishPlatform()
{
  glFinish();
}

void ezGALDeviceGL::SetPrimarySwapChainPlatform(ezGALSwapChain* pSwapChain)
{
  ezGALSwapChainGL* pSwapChainGL = static_cast<ezGALSwapChainGL*>(pSwapChain);
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  wglMakeCurrent(pSwapChainGL->GetWindowDC(), pSwapChainGL->GetOpenGLRC());
#endif
}

void ezGALDeviceGL::FillCapabilitiesPlatform()
{
  ezLog::Warning("Not implemented!");
}



void ezGALDeviceGL::FillFormatLookupTable()
{
  // Good source for format support (GL3.3)- probably very NVIDIA specific but with OpenGL versions annotated.
  // https://developer.nvidia.com/sites/default/files/akamai/gamedev/docs/nv_ogl_texture_formats.pdf?download=1
 

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBFloat,
    ezGALFormatLookupEntryGL(GL_RGB32F).RT(GL_RGB32F).RV(GL_RGB32F).VA(GL_FLOAT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGFloat,
    ezGALFormatLookupEntryGL(GL_RG32F).RT(GL_RG32F).RV(GL_RG32F).VA(GL_FLOAT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RFloat,
    ezGALFormatLookupEntryGL(GL_R32F).RT(GL_R32F).RV(GL_R32F).VA(GL_FLOAT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::D24S8,
        ezGALFormatLookupEntryGL(GL_DEPTH24_STENCIL8).DS(GL_DEPTH24_STENCIL8));

  /// \todo glEnable(GL_FRAMEBUFFER_SRGB) should be called to guarantee that all writes to framebuffers with sRGB formats are handled properly
  // However, atm I'am not sure what happens to the backbuffer since it is not possible to specify a format for it.

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUByteNormalizedsRGB,
    ezGALFormatLookupEntryGL(GL_SRGB8).RT(GL_SRGB8).RV(GL_SRGB8));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUByteNormalizedsRGB,
    ezGALFormatLookupEntryGL(GL_SRGB8_ALPHA8).RT(GL_SRGB8_ALPHA8).RV(GL_SRGB8_ALPHA8));



  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUByteNormalized,
    ezGALFormatLookupEntryGL(GL_RGB8).RT(GL_RGB8).RV(GL_RGB8).VA(GL_UNSIGNED_BYTE));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUByteNormalized,
    ezGALFormatLookupEntryGL(GL_RGBA8).RT(GL_RGBA8).RV(GL_RGBA8).VA(GL_UNSIGNED_BYTE));

  /// \todo fill out formats.
  // Can we do parts of that automatically?
}