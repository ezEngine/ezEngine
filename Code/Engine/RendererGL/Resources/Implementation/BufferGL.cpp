#include <RendererGL/PCH.h>
#include <RendererGL/Basics.h>
#include <RendererGL/Resources/BufferGL.h>
#include <RendererGL/Device/DeviceGL.h>
#include <RendererGL/Context/ContextGL.h>

#include <RendererGL/glew/glew.h>


ezGALBufferGL::ezGALBufferGL(const ezGALBufferCreationDescription& Description)
  : ezGALBuffer(Description),
  m_BufferHandle(EZ_RENDERERGL_INVALID_ID)
{
}

ezGALBufferGL::~ezGALBufferGL()
{
}


ezResult ezGALBufferGL::InitPlatform(ezGALDevice* pDevice, const void* pInitialData)
{
  ezGALContextGL* pContextGL = static_cast<ezGALContextGL*>(pDevice->GetPrimaryContext());

  // Why initial target is not crucial but can matter: https://www.opengl.org/wiki/Buffer_Object
  // "In the technical sense, the target a buffer is bound to does not matter for the purposes of creating the memory storage for it.
  //  However, OpenGL implementations are allowed to make judgments about your intended use of the buffer object based on the first target you bind it to.
  //  So if you intend for your buffer object to be used as a vertex array buffer, you should bind that buffer to GL_ARRAY_BUFFER first.
  //  You may later use it as a GL_TRANSFORM_FEEDBACK buffer for readback or whatever, but binding it to GL_ARRAY_BUFFER gives the implementation important
  //  information about how you plan to use it overall.
  ezGALContextGL::GLBufferBinding::Enum bindingTarget = static_cast<ezGALContextGL::GLBufferBinding::Enum>(m_Description.m_BufferType);

  // Target modifier.
  if (m_Description.m_bUseAsStructuredBuffer || m_Description.m_bAllowUAV)
  {
#ifdef EZ_RENDERERGL_GL4
    bindingTarget = ezGALContextGL::GLBufferBinding::StorageBuffer;
#else
    ezLog::Warning("Structured buffers and UAVs are not supported by this OpenGL version!");
#endif
  }
  else if (m_Description.m_bUseForIndirectArguments)
  {
#ifdef EZ_RENDERERGL_GL4
    bindingTarget = ezGALContextGL::GLBufferBinding::IndirectDraw;
#else
    ezLog::Warning("Indirect draw buffers are not supported by this OpenGL version!");
#endif
  }
  else if (m_Description.m_bStreamOutputTarget)
  {
    bindingTarget = ezGALContextGL::GLBufferBinding::TransformFeedBack;
  }

  // Create buffer.
  m_BufferHandle = static_cast<ezGALDeviceGL*>(pDevice)->GetUnusedGLBufferId();

  // To ensure that the buffer will be reset, create object to reset the state when the stackframe is left.
  ezGALContextGL::ScopedBufferBinding scopedBinding(pContextGL, bindingTarget, m_BufferHandle);

#ifdef EZ_RENDERERGL_GL4
  // For OpenGL4 use newer glBufferStorage function.

  // Determine access.  
  GLbitfield accessFlags = 0;
  if (!m_Description.m_ResourceAccess.m_bImmutable)
  {
    // Dynamic does not mean dynamic in a DirectX sense, it just means updatable using glBufferSubData.
    // Note that glCopyBufferSubData and glClearBufferSubData will always work.
    accessFlags |= GL_DYNAMIC_STORAGE_BIT;

    /// \todo Allow also CPU write since there is currently no resource flag to determine if it should be illegal.
    accessFlags |= GL_MAP_WRITE_BIT;

    if (m_Description.m_BufferType == ezGALBufferType::ConstantBuffer)
    {
      // GL_MAP_PERSISTENT_BIT makes it possible write to the buffer while it is in use. This is useful to update constants in a uniform buffer while it is still sst.
      accessFlags |= GL_MAP_PERSISTENT_BIT;
    }
  }
  if (m_Description.m_ResourceAccess.m_bReadBack)
  {
    accessFlags |= GL_MAP_READ_BIT;
  }

  if (EZ_GL_CALL(glBufferStorage, ezGALContextGL::s_GALBufferBindingToGL[bindingTarget], m_Description.m_uiTotalSize, pInitialData, accessFlags) != EZ_SUCCESS)
    return EZ_FAILURE;
#else
  // For older OpenGL implementations fall back to glBufferData.

  // Determine access.  
  GLenum accessFlags = GL_STATIC_DRAW; /// \todo Introduce helper-function that maps resource access to OpenGL usage.
  
  if (EZ_GL_CALL(glBufferData, ezGALContextGL::s_GALBufferBindingToGL[bindingTarget], m_Description.m_uiTotalSize, pInitialData, accessFlags) != EZ_SUCCESS)
    return EZ_FAILURE;
#endif


  return EZ_SUCCESS;
}

ezResult ezGALBufferGL::DeInitPlatform(ezGALDevice* pDevice)
{
  if (m_BufferHandle != EZ_RENDERERGL_INVALID_ID)
  {
    if (EZ_GL_CALL(glDeleteBuffers, 1, &m_BufferHandle) != EZ_SUCCESS)
      return EZ_FAILURE;
    m_BufferHandle = EZ_RENDERERGL_INVALID_ID;
  }
  
  return EZ_SUCCESS;
}