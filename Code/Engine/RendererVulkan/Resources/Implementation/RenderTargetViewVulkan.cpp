#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>

bool IsArrayView(const ezGALTextureCreationDescription& texDesc, const ezGALRenderTargetViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstSlice > 0;
}

ezGALRenderTargetViewVulkan::ezGALRenderTargetViewVulkan(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description)
  : ezGALRenderTargetView(pTexture, Description)
{
}

ezGALRenderTargetViewVulkan::~ezGALRenderTargetViewVulkan() {}

ezResult ezGALRenderTargetViewVulkan::InitPlatform(ezGALDevice* pDevice)
{
  const ezGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  if (pTexture == nullptr)
  {
    ezLog::Error("No valid texture handle given for render target view creation!");
    return EZ_FAILURE;
  }

  const ezGALTextureCreationDescription& texDesc = pTexture->GetDescription();
  ezGALResourceFormat::Enum viewFormat = texDesc.m_Format;

  if (m_Description.m_OverrideViewFormat != ezGALResourceFormat::Invalid)
    viewFormat = m_Description.m_OverrideViewFormat;

  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  vk::Format vkViewFormat = vk::Format::eUndefined;

  const bool bIsDepthFormat = ezGALResourceFormat::IsDepthFormat(viewFormat);
  if (bIsDepthFormat)
  {
    vkViewFormat = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eDepthStencilType;
  }
  else
  {
    vkViewFormat = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eRenderTarget;
  }

  if (vkViewFormat == vk::Format::eUndefined)
  {
    ezLog::Error("Couldn't get Vulkan format for view!");
    return EZ_FAILURE;
  }

  auto pTextureVulkan = static_cast<const ezGALTextureVulkan*>(pTexture->GetParentResource());
  vk::Image vkImage = pTextureVulkan->GetImage();
  const bool bIsArrayView = IsArrayView(texDesc, m_Description);

  if (pTextureVulkan->GetFormatOverrideEnabled())
  {
    vkViewFormat = pTextureVulkan->GetImageFormat();
  }

  vk::ImageViewCreateInfo imageViewCreationInfo;
  if (bIsDepthFormat)
  {
    imageViewCreationInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    if (texDesc.m_Format == ezGALResourceFormat::D24S8)
    {
      imageViewCreationInfo.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
    }
  }
  else
  {
    imageViewCreationInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  }

  imageViewCreationInfo.image = vkImage;
  imageViewCreationInfo.format = vkViewFormat;

  if (!bIsArrayView)
  {
    imageViewCreationInfo.viewType = vk::ImageViewType::e2D;
    imageViewCreationInfo.subresourceRange.baseMipLevel = m_Description.m_uiMipLevel;
    imageViewCreationInfo.subresourceRange.levelCount = 1;
    imageViewCreationInfo.subresourceRange.layerCount = 1;
  }
  else
  {
    imageViewCreationInfo.viewType = vk::ImageViewType::e2DArray;
    imageViewCreationInfo.subresourceRange.baseMipLevel = m_Description.m_uiMipLevel;
    imageViewCreationInfo.subresourceRange.levelCount = 1;
    imageViewCreationInfo.subresourceRange.baseArrayLayer = m_Description.m_uiFirstSlice;
    imageViewCreationInfo.subresourceRange.layerCount = m_Description.m_uiSliceCount;
  }
  m_range = imageViewCreationInfo.subresourceRange;
  m_bfullRange = m_range == pTextureVulkan->GetFullRange();

  VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&imageViewCreationInfo, nullptr, &m_imageView));
  pVulkanDevice->SetDebugName("RTV", m_imageView);
  return EZ_SUCCESS;
}

ezResult ezGALRenderTargetViewVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_imageView);
  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_RenderTargetViewVulkan);
