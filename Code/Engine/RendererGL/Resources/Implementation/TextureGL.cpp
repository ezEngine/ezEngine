#include <RendererGL/PCH.h>
#include <RendererGL/Basics.h>
#include <RendererGL/Resources/TextureGL.h>
#include <RendererGL/Device/DeviceGL.h>
#include <RendererGL/Context/ContextGL.h>

#include <RendererGL/glew/glew.h>

ezGALTextureGL::ezGALTextureGL(const ezGALTextureCreationDescription& Description)
  : ezGALTexture(Description),
  m_TextureHandle(EZ_RENDERERGL_INVALID_ID)
{
}

ezGALTextureGL::~ezGALTextureGL()
{
}

ezResult ezGALTextureGL::InitPlatform(ezGALDevice* pDevice, const ezArrayPtr<ezGALSystemMemoryDescription>* pInitialData)
{
  ezGALDeviceGL* pDeviceGL = static_cast<ezGALDeviceGL*>(pDevice);
  ezGALContextGL* pContextGL = static_cast<ezGALContextGL*>(pDevice->GetPrimaryContext());

  // Translate & check format.
  const ezGALFormatLookupEntryGL& formatInfo = pDeviceGL->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format);

  if (formatInfo.m_eStorage == EZ_RENDERERGL_INVALID_ID)
  {
    ezLog::Error("Texture format %i is not supported!", m_Description.m_Format);
    return EZ_FAILURE;
  }

  // Create texture.
  m_TextureHandle = static_cast<ezGALDeviceGL*>(pDevice)->GetUnusedGLTextureId();

  // To ensure that the buffer will be reset, create object to reset the state when the stackframe is left.
  ezGALContextGL::ScopedTextureBinding scopedBinding(pContextGL, m_Description.m_Type, m_TextureHandle);

  ezUInt32 bindingTarget = ezGALContextGL::s_GALTextureTypeToGL[m_Description.m_Type];

  // Create texture & fill.
  // For GL4 use new glTexStorageX commands, otherwise use old glTexImageX
  switch (m_Description.m_Type)
  {
  case ezGALTextureType::Texture2D:
    if (m_Description.m_SampleCount == 1)
    {
      // The Texture storage API makes creating textures much more predictable and results in less type confusion. Use it whenever possible:
      bool bUseTextureStorage = GLEW_ARB_texture_storage == GL_TRUE;
#ifdef EZ_RENDERERGL_GL4
      bUseTextureStorage = true;
#endif
      ezUInt32 baseInternalFormat = GL_RGBA; /// \todo
      ezUInt32 formatType = GL_UNSIGNED_BYTE; /// \todo

      if(bUseTextureStorage)
      {
        if (EZ_GL_CALL(glTexStorage2D, bindingTarget, m_Description.m_uiMipSliceCount, formatInfo.m_eStorage, m_Description.m_uiWidth, m_Description.m_uiHeight) != EZ_SUCCESS)
          return EZ_FAILURE;
      }
      else
      {
        // Define valid mipmap range. See https://www.opengl.org/wiki/Common_Mistakes#Creating_a_complete_texture
        EZ_GL_CALL(glTexParameteri, bindingTarget, GL_TEXTURE_BASE_LEVEL, 0);
        EZ_GL_CALL(glTexParameteri, bindingTarget, GL_TEXTURE_MAX_LEVEL, m_Description.m_uiMipSliceCount);

        // Need a correct "baseInternalFormat" and a "formatType" additionally to the actual storage format.
        if (formatInfo.m_eDepthStencilType != EZ_RENDERERGL_INVALID_ID)
        {
          baseInternalFormat = GL_DEPTH_STENCIL;
          formatType = GL_UNSIGNED_INT;
        }
        else if (formatInfo.m_eDepthOnlyType != EZ_RENDERERGL_INVALID_ID)
        {
          baseInternalFormat = GL_DEPTH_COMPONENT;
          formatType = GL_UNSIGNED_INT;
        }
        else if (formatInfo.m_eDepthStencilType != EZ_RENDERERGL_INVALID_ID)
        {
          baseInternalFormat = GL_STENCIL_COMPONENTS;
          formatType = GL_UNSIGNED_INT;
        }

        // Special format hack.
        if (formatInfo.m_eStorage == GL_DEPTH24_STENCIL8)
        {
          formatType = GL_UNSIGNED_INT_24_8;
        }
      }

      unsigned int currentWidth = m_Description.m_uiWidth;
      unsigned int currentHeight = m_Description.m_uiHeight;
      for (ezUInt32 level = 0; level < m_Description.m_uiMipSliceCount; ++level)
      {
        /// \todo Assert for pInitialData[level] sizes? Or is this the job of ezGALDevice::CreateTexture
        void* pData = pInitialData == nullptr ? nullptr : (*pInitialData)[level].m_pData;
        if (bUseTextureStorage)
        {
          if (!pData)
            break;
          if (EZ_GL_CALL(glTexSubImage2D, bindingTarget, level, 0, 0, currentWidth, currentHeight, /* TODO */ GL_RGBA, GL_UNSIGNED_BYTE, (*pInitialData)[level].m_pData) != EZ_SUCCESS)
            return EZ_FAILURE;
        }
        else
        {
          if (EZ_GL_CALL(glTexImage2D, bindingTarget, 0, formatInfo.m_eStorage, m_Description.m_uiWidth, m_Description.m_uiHeight, 0, /* TODO */ baseInternalFormat, formatType, pData) != EZ_SUCCESS)
            return EZ_FAILURE;
        }

        currentWidth /= 2;
        currentHeight /= 2;
      }
    }
    else
    {
      EZ_REPORT_FAILURE("Multisampling 2D Textures not yet implemented!");
    }

    break;

  case ezGALTextureType::TextureCube:
    EZ_ASSERT_NOT_IMPLEMENTED;
    break;

  case ezGALTextureType::Texture3D:
    EZ_ASSERT_NOT_IMPLEMENTED;
    break;

  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezGALTextureGL::DeInitPlatform(ezGALDevice* pDevice)
{
  if (m_TextureHandle != EZ_RENDERERGL_INVALID_ID)
  {
    if (EZ_GL_CALL(glDeleteTextures, 1, &m_TextureHandle) != EZ_SUCCESS)
      return EZ_FAILURE;
    m_TextureHandle = EZ_RENDERERGL_INVALID_ID;
  }

  return EZ_SUCCESS;
}