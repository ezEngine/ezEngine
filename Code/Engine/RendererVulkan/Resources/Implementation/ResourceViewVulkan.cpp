#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

ezGALTextureResourceViewVulkan::ezGALTextureResourceViewVulkan(ezGALTexture* pResource, const ezGALTextureResourceViewCreationDescription& Description)
  : ezGALTextureResourceView(pResource, Description)
{
}

ezGALTextureResourceViewVulkan::~ezGALTextureResourceViewVulkan() = default;

ezResult ezGALTextureResourceViewVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  const ezGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  if (pTexture == nullptr)
  {
    ezLog::Error("No valid texture handle given for resource view creation!");
    return EZ_FAILURE;
  }

  auto pParentTexture = static_cast<const ezGALTextureVulkan*>(pTexture->GetParentResource());
  auto image = pParentTexture->GetImage();
  const ezGALTextureCreationDescription& texDesc = pTexture->GetDescription();

  const bool bIsDepth = ezGALResourceFormat::IsDepthFormat(pTexture->GetDescription().m_Format);

  ezGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat == ezGALResourceFormat::Invalid ? texDesc.m_Format : m_Description.m_OverrideViewFormat;
  vk::ImageViewCreateInfo viewCreateInfo;
  viewCreateInfo.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_format;
  viewCreateInfo.image = image;
  viewCreateInfo.subresourceRange = ezConversionUtilsVulkan::GetSubresourceRange(texDesc, m_Description);
  viewCreateInfo.subresourceRange.aspectMask &= ~vk::ImageAspectFlagBits::eStencil;


  m_resourceImageInfo.imageLayout = ezConversionUtilsVulkan::GetDefaultLayout(pParentTexture->GetImageFormat());

  m_range = viewCreateInfo.subresourceRange;

  const ezEnum<ezGALTextureType> type = m_Description.m_OverrideViewType != ezGALTextureType::Invalid ? m_Description.m_OverrideViewType : texDesc.m_Type;
  switch (type)
  {
    case ezGALTextureType::Texture2D:
    case ezGALTextureType::Texture2DShared:
    case ezGALTextureType::Texture2DProxy:
      viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(ezGALTextureType::Texture2D);
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));
      break;

    case ezGALTextureType::Texture2DArray:
      viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(type);
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));
      break;

    case ezGALTextureType::TextureCubeArray:
      viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(type);
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));
      break;

    case ezGALTextureType::Texture3D:
      viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(type);
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));
      break;

    case ezGALTextureType::TextureCube:
      viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(type);
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return EZ_SUCCESS;
}

ezResult ezGALTextureResourceViewVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_resourceImageInfo.imageView);
  m_resourceImageInfo = vk::DescriptorImageInfo();
  return EZ_SUCCESS;
}

/////////////////////////////////////////////////////////

const vk::DescriptorBufferInfo& ezGALBufferResourceViewVulkan::GetBufferInfo() const
{
  // Vulkan buffers get constantly swapped out for new ones so the vk::Buffer pointer is not persistent.
  // We need to acquire the latest one on every request for rendering.
  m_resourceBufferInfo.buffer = static_cast<const ezGALBufferVulkan*>(GetResource())->GetVkBuffer();
  return m_resourceBufferInfo;
}

ezGALBufferResourceViewVulkan::ezGALBufferResourceViewVulkan(ezGALBuffer* pResource, const ezGALBufferResourceViewCreationDescription& Description)
  : ezGALBufferResourceView(pResource, Description)
{
}

ezGALBufferResourceViewVulkan::~ezGALBufferResourceViewVulkan() = default;

ezResult ezGALBufferResourceViewVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  const ezGALBuffer* pBuffer = pDevice->GetBuffer(m_Description.m_hBuffer);
  auto pParentBuffer = static_cast<const ezGALBufferVulkan*>(pBuffer);

  m_resourceBufferInfo.offset = m_Description.m_uiByteOffset;
  m_resourceBufferInfo.range = m_Description.m_uiByteCount;

  if (m_Description.m_ResourceType == ezGALShaderResourceType::TexelBuffer)
  {
    vk::BufferViewCreateInfo viewCreateInfo;
    viewCreateInfo.buffer = pParentBuffer->GetVkBuffer();
    viewCreateInfo.offset = m_Description.m_uiByteOffset;
    viewCreateInfo.range = m_Description.m_uiByteCount;
    viewCreateInfo.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_format;
    VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createBufferView(&viewCreateInfo, nullptr, &m_bufferView));
  }

  return EZ_SUCCESS;
}

ezResult ezGALBufferResourceViewVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  m_resourceBufferInfo = vk::DescriptorBufferInfo();
  pVulkanDevice->DeleteLater(m_bufferView);
  return EZ_SUCCESS;
}
