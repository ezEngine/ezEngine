#include <RendererGL/PCH.h>
#include <RendererGL/Basics.h>
#include <RendererGL/Resources/RenderTargetViewGL.h>
#include <RendererGL/Device/DeviceGL.h>

#include <RendererGL/glew/glew.h>

#include <RendererFoundation/Resources/Texture.h>
#include <Foundation/Logging/Log.h>

ezGALRenderTargetViewGL::ezGALRenderTargetViewGL(const ezGALRenderTargetViewCreationDescription& Description)
  : ezGALRenderTargetView(Description),
  m_BindingTarget(EZ_RENDERERGL_INVALID_ID)
{
}

ezGALRenderTargetViewGL::~ezGALRenderTargetViewGL()
{
}

ezResult ezGALRenderTargetViewGL::InitPlatform(ezGALDevice* pDevice)
{
  const ezGALTexture* pTexture = pDevice->GetTexture(m_Description.m_hTexture);
  if (pTexture == nullptr)
  {
    ezLog::Error("Invalid texture handle in RenderTargetView!");
    return EZ_FAILURE;
  }

  const ezGALDeviceGL* pDeviceGL = static_cast<ezGALDeviceGL*>(pDevice);
  ezGALResourceFormat::Enum ViewFormat = m_Description.m_OverrideViewFormat;
  if (ViewFormat == ezGALResourceFormat::Invalid)
    ViewFormat = pTexture->GetDescription().m_Format;
  const ezGALFormatLookupEntryGL& formatDesc = pDeviceGL->GetFormatLookupTable().GetFormatInfo(ViewFormat);

  // Determine binding and check if format is usable.
  if (m_Description.m_RenderTargetType == ezGALRenderTargetType::DepthStencil)
  {
    // Determine attachment type.
    if (formatDesc.m_eDepthStencilType != EZ_RENDERERGL_INVALID_ID)
    {
      m_BindingTarget = GL_DEPTH_STENCIL_ATTACHMENT;
    }
    else if (formatDesc.m_eDepthOnlyType != EZ_RENDERERGL_INVALID_ID)
    {
      m_BindingTarget = GL_DEPTH_ATTACHMENT;
    }
    else if (formatDesc.m_eStencilOnlyType != EZ_RENDERERGL_INVALID_ID)
    {
      m_BindingTarget = GL_STENCIL_ATTACHMENT;
    }
    else
    {
      ezLog::Error("The given resource format  %i is not supported for depth/stencil rendertargets!", ViewFormat);
      return EZ_FAILURE;
    }
  }
  else if (m_Description.m_RenderTargetType == ezGALRenderTargetType::Color)
  {
    // Binding target is always color. Needs to be incremented by the slot number when creating FBO.
    m_BindingTarget = GL_COLOR_ATTACHMENT0;

    if (formatDesc.m_eRenderTarget == EZ_RENDERERGL_INVALID_ID)
    {
      ezLog::Error("The OverrideViewFormat %i is not supported for color rendertargets!", formatDesc.m_eRenderTarget);
      return EZ_FAILURE;
    }
  }
  else
  {
    EZ_REPORT_FAILURE("Rendertarget type %i not yet implemented!", m_Description.m_RenderTargetType);
  }

  return EZ_SUCCESS;
}

ezResult ezGALRenderTargetViewGL::DeInitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}
