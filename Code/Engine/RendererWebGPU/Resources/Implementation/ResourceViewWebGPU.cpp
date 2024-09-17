#include <RendererWebGPU/RendererWebGPUPCH.h>

#include <RendererWebGPU/Device/DeviceWebGPU.h>
#include <RendererWebGPU/Resources/BufferWebGPU.h>
#include <RendererWebGPU/Resources/ResourceViewWebGPU.h>
#include <RendererWebGPU/Resources/TextureWebGPU.h>

wgpu::TextureFormat ToWGPU(ezGALResourceFormat::Enum format);

ezGALTextureResourceViewWebGPU::ezGALTextureResourceViewWebGPU(ezGALTexture* pResource, const ezGALTextureResourceViewCreationDescription& Description)
  : ezGALTextureResourceView(pResource, Description)
{
}

ezGALTextureResourceViewWebGPU::~ezGALTextureResourceViewWebGPU() = default;

ezResult ezGALTextureResourceViewWebGPU::InitPlatform(ezGALDevice* pDevice)
{
  // EZ_WEBGPU_TRACE();

  ezGALTextureWebGPU* pTexture = (ezGALTextureWebGPU*)pDevice->GetTexture(m_Description.m_hTexture);

  wgpu::TextureViewDescriptor desc;
  desc.arrayLayerCount = m_Description.m_uiArraySize;
  desc.baseArrayLayer = m_Description.m_uiFirstArraySlice;
  desc.mipLevelCount = m_Description.m_uiMipLevelsToUse;
  desc.baseMipLevel = m_Description.m_uiMostDetailedMipLevel;

  if (m_Description.m_OverrideViewFormat != ezGALResourceFormat::Invalid)
  {
    desc.format = ToWGPU(m_Description.m_OverrideViewFormat);
  }
  else
  {
    desc.format = ToWGPU(pTexture->GetDescription().m_Format);
  }

  m_TextureView = pTexture->m_Texture.CreateView(&desc);

  return EZ_SUCCESS;
}

ezResult ezGALTextureResourceViewWebGPU::DeInitPlatform(ezGALDevice* pDevice)
{
  m_TextureView = nullptr;
  return EZ_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////

ezGALBufferResourceViewWebGPU::ezGALBufferResourceViewWebGPU(ezGALBuffer* pResource, const ezGALBufferResourceViewCreationDescription& Description)
  : ezGALBufferResourceView(pResource, Description)
{
}

ezGALBufferResourceViewWebGPU::~ezGALBufferResourceViewWebGPU() = default;

ezResult ezGALBufferResourceViewWebGPU::InitPlatform(ezGALDevice* pDevice)
{
  EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}

ezResult ezGALBufferResourceViewWebGPU::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}
