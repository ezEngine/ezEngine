#include <RendererGL/PCH.h>
#include <RendererGL/Basics.h>

#include <RendererGL/Resources/RenderTargetConfigGL.h>
#include <RendererGL/Device/DeviceGL.h>
#include <RendererGL/Resources/TextureGL.h>

#include <RendererGL/Resources/RenderTargetViewGL.h>


#include <RendererGL/glew/glew.h>

ezGALRenderTargetConfigGL::ezGALRenderTargetConfigGL(const ezGALRenderTargetConfigCreationDescription& Description)
  : ezGALRenderTargetConfig(Description)
{
}

ezGALRenderTargetConfigGL::~ezGALRenderTargetConfigGL()
{
}

ezResult ezGALRenderTargetConfigGL::InitPlatform(ezGALDevice* pDevice)
{
  // Special case: Hardware Backbuffer?
  if (m_Description.m_bHardwareBackBuffer)
  {
    m_FramebufferHandle = 0;
    return EZ_SUCCESS;
  }


  ezGALDeviceGL* pDeviceGL = static_cast<ezGALDeviceGL*>(pDevice);
  ezArrayPtr<GLuint> drawBuffers;

  if (EZ_GL_CALL(glGenFramebuffers, 1, &m_FramebufferHandle) != EZ_SUCCESS)
    return EZ_FAILURE;

  // To ensure that the buffer will be reset, create object to reset the state when the stackframe is left.
  /// \todo State handling by primary context. REMOVE ALL glGetIntegerv ANYWHERE!
  struct BufferReset
  {
    BufferReset()
    {
      EZ_GL_CALL(glGetIntegerv, GL_DRAW_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&bufferBoundBefore));
    }
    ~BufferReset()
    {
      EZ_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, bufferBoundBefore);
    }
    glFramebuffer bufferBoundBefore;
  } resetObject;


  // Need to bind framebuffer to add attachments.
  if (EZ_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_FramebufferHandle) != EZ_SUCCESS)
    return EZ_FAILURE;

  // Add depth-stencil attachment.
  if (!m_Description.m_hDepthStencilTarget.IsInvalidated())
  {
    // Create attachment.
    if (AttachRenderTargetViewToFramebuffer(0, m_Description.m_hDepthStencilTarget, pDeviceGL) == EZ_FAILURE)
      return EZ_FAILURE;
  }

  // Add color attachments.
  for (ezUInt32 i = 0; i<m_Description.m_uiColorTargetCount; ++i)
  {
    // Create attachment.
    if (AttachRenderTargetViewToFramebuffer(i, m_Description.m_hColorTargets[i], pDeviceGL) == EZ_FAILURE)
      return EZ_FAILURE;
  }


  // setup draw buffers
  drawBuffers = EZ_DEFAULT_NEW_ARRAY(GLuint, m_Description.m_uiColorTargetCount);
  for (ezUInt32 i = 0; i < drawBuffers.GetCount(); ++i)
    drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
  EZ_GL_CALL(glDrawBuffers, drawBuffers.GetCount(), drawBuffers.GetPtr());
  EZ_DEFAULT_DELETE_ARRAY(drawBuffers);


  // Additional error checking
  GLenum framebufferStatus = EZ_GL_RET_CALL(glCheckFramebufferStatus, GL_FRAMEBUFFER);

  if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
  {
    switch (framebufferStatus)
    {
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      ezLog::Error("ezGALRenderTargetConfigGL platform-init failed with GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT. See http://www.khronos.org/opengles/sdk/docs/man/xhtml/glCheckFramebufferStatus.xml for more informations!");
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
      ezLog::Error("ezGALRenderTargetConfigGL platform-init failed with GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS. See http://www.khronos.org/opengles/sdk/docs/man/xhtml/glCheckFramebufferStatus.xml for more informations!");
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      ezLog::Error("ezGALRenderTargetConfigGL platform-init failed with GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT. See http://www.khronos.org/opengles/sdk/docs/man/xhtml/glCheckFramebufferStatus.xml for more informations!");
      break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
      ezLog::Error("ezGALRenderTargetConfigGL platform-init failed with GL_FRAMEBUFFER_UNSUPPORTED. See http://www.khronos.org/opengles/sdk/docs/man/xhtml/glCheckFramebufferStatus.xml for more informations!");
      break;

    default:
      ezLog::Error("ezGALRenderTargetConfigGL platform-init failed due to an incomplete framebuffer with the unknown error code %i", framebufferStatus);
      break;
    }
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezGALRenderTargetConfigGL::DeInitPlatform(ezGALDevice* pDevice)
{
  if (m_FramebufferHandle != EZ_RENDERERGL_INVALID_ID)
  {
    if (EZ_GL_CALL(glDeleteFramebuffers, 1, &m_FramebufferHandle) != EZ_SUCCESS)
      return EZ_FAILURE;
    m_FramebufferHandle = EZ_RENDERERGL_INVALID_ID;
  }

  return EZ_SUCCESS;
}

ezResult ezGALRenderTargetConfigGL::AttachRenderTargetViewToFramebuffer(ezUInt32 index, const ezGALRenderTargetViewHandle& RenderTargetViewHandle, const ezGALDeviceGL* pDeviceGL)
{
  const ezGALRenderTargetViewGL* pRenderTargetView = static_cast<const ezGALRenderTargetViewGL*>(pDeviceGL->GetRenderTargetView(RenderTargetViewHandle));
  if (!pRenderTargetView)
  {
    ezLog::Error("Invalid RenderTargetView-handle for ezGALRenderTargetConfigGL creation!");
    return EZ_FAILURE;
  }

  const ezGALRenderTargetViewCreationDescription& targetViewDesc = pRenderTargetView->GetDescription();

  const ezGALTextureGL* pTexture = static_cast<const ezGALTextureGL*>(pDeviceGL->GetTexture(targetViewDesc.m_hTexture));
  if (!pTexture)
  {
    ezLog::Error("RenderTargetView contains invalid texture handle!");
    return EZ_FAILURE;
  }

  glBindingTarget attachmentType = pRenderTargetView->GetBindingTarget() + index;
  EZ_ASSERT_DEV(attachmentType != EZ_RENDERERGL_INVALID_ID, "Invalid framebuffer attachment type. Usually caused by broken RenderTargetViews."); // Assertion since this should already by reported much earlier.

  if (targetViewDesc.m_uiFirstSlice > 0)
  {
    return EZ_GL_CALL(glFramebufferTextureLayer, GL_FRAMEBUFFER, attachmentType, pTexture->GetGLTextureHandle(), targetViewDesc.m_uiMipSlice, targetViewDesc.m_uiFirstSlice);
  }
  else
  {
    return EZ_GL_CALL(glFramebufferTexture, GL_FRAMEBUFFER, attachmentType, pTexture->GetGLTextureHandle(), targetViewDesc.m_uiMipSlice);
  }
}