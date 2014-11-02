
#include <RendererGL/PCH.h>
#include <RendererGL/Basics.h>
#include <RendererGL/State/StateGL.h>

#include <RendererGL/glew/glew.h>

// Blend state

ezGALBlendStateGL::ezGALBlendStateGL(const ezGALBlendStateCreationDescription& Description)
  : ezGALBlendState(Description)
{
}

ezGALBlendStateGL::~ezGALBlendStateGL()
{
}

ezResult ezGALBlendStateGL::InitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}

ezResult ezGALBlendStateGL::DeInitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}

// Depth Stencil state

ezGALDepthStencilStateGL::ezGALDepthStencilStateGL(const ezGALDepthStencilStateCreationDescription& Description)
  : ezGALDepthStencilState(Description)
{
}

ezGALDepthStencilStateGL::~ezGALDepthStencilStateGL()
{
}

ezResult ezGALDepthStencilStateGL::InitPlatform(ezGALDevice* pDevice) 
{
  return EZ_SUCCESS;
}

ezResult ezGALDepthStencilStateGL::DeInitPlatform(ezGALDevice* pDevice) 
{
  return EZ_SUCCESS;
}


// Rasterizer state

ezGALRasterizerStateGL::ezGALRasterizerStateGL(const ezGALRasterizerStateCreationDescription& Description)
  : ezGALRasterizerState(Description)
{
}

ezGALRasterizerStateGL::~ezGALRasterizerStateGL()
{
}



ezResult ezGALRasterizerStateGL::InitPlatform(ezGALDevice* pDevice) 
{
  return EZ_SUCCESS;
}


ezResult ezGALRasterizerStateGL::DeInitPlatform(ezGALDevice* pDevice) 
{
  return EZ_SUCCESS;
}


// Sampler state

const ezUInt32 ezGALSamplerStateGL::s_GALAdressModeToGL[ezGALTextureAddressMode::ENUM_COUNT] =
{
  GL_REPEAT,              // Wrap
  GL_MIRRORED_REPEAT,     // Mirror
  GL_CLAMP_TO_EDGE,       // Clamp
  GL_CLAMP_TO_BORDER,     // Border
  GL_MIRROR_CLAMP_TO_EDGE // MirrorOnce
};

ezGALSamplerStateGL::ezGALSamplerStateGL(const ezGALSamplerStateCreationDescription& Description)
  : ezGALSamplerState(Description),
  m_SamplerStateID(EZ_RENDERERGL_INVALID_ID)
{
}

ezGALSamplerStateGL::~ezGALSamplerStateGL()
{
}

ezResult ezGALSamplerStateGL::InitPlatform(ezGALDevice* pDevice) 
{
  if (EZ_GL_CALL(glGenSamplers, 1, &m_SamplerStateID) != EZ_SUCCESS)
    return EZ_FAILURE;


  // Adress mode.
  if (EZ_GL_CALL(glSamplerParameteri, m_SamplerStateID, GL_TEXTURE_WRAP_S, s_GALAdressModeToGL[m_Description.m_AddressU]) != EZ_SUCCESS)
    return EZ_FAILURE;
  if (EZ_GL_CALL(glSamplerParameteri, m_SamplerStateID, GL_TEXTURE_WRAP_T, s_GALAdressModeToGL[m_Description.m_AddressV]) != EZ_SUCCESS)
    return EZ_FAILURE;
  if (EZ_GL_CALL(glSamplerParameteri, m_SamplerStateID, GL_TEXTURE_WRAP_R, s_GALAdressModeToGL[m_Description.m_AddressW]) != EZ_SUCCESS)
    return EZ_FAILURE;

  glSamplerParameterfv(m_SamplerStateID, GL_TEXTURE_BORDER_COLOR, m_Description.m_BorderColor.GetData());


  // Filter.
  GLint minFilterGl;
  if (m_Description.m_MinFilter == ezGALTextureFilterMode::Point)
    minFilterGl = m_Description.m_MipFilter == ezGALTextureFilterMode::Point ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_LINEAR;
  else
    minFilterGl = m_Description.m_MipFilter == ezGALTextureFilterMode::Point ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;

  if (EZ_GL_CALL(glSamplerParameteri, m_SamplerStateID, GL_TEXTURE_MIN_FILTER, minFilterGl) != EZ_SUCCESS)
    return EZ_FAILURE;
  if (EZ_GL_CALL(glSamplerParameteri, m_SamplerStateID, GL_TEXTURE_MAG_FILTER, m_Description.m_MagFilter == ezGALTextureFilterMode::Point ? GL_NEAREST : GL_LINEAR) != EZ_SUCCESS)
    return EZ_FAILURE;

  if (GLEW_EXT_texture_filter_anisotropic)
  {
    if (EZ_GL_CALL(glSamplerParameteri,m_SamplerStateID, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_Description.m_uiMaxAnisotropy) != EZ_SUCCESS)
      return EZ_FAILURE;
  }


  /// \todo
  /*

    ezGALCompareFunc::Enum m_SampleCompareFunc;

    float m_fMipLodBias;
    float m_fMinMip;
    float m_fMaxMip;

  */

  return EZ_SUCCESS;
}


ezResult ezGALSamplerStateGL::DeInitPlatform(ezGALDevice* pDevice) 
{
  if (m_SamplerStateID != EZ_RENDERERGL_INVALID_ID)
  {
    if (EZ_GL_CALL(glDeleteSamplers, 1, &m_SamplerStateID) != EZ_SUCCESS)
      return EZ_FAILURE;
  }
  return EZ_SUCCESS;
}