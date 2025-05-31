#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

ezGALTextureUnorderedAccessViewVulkan::ezGALTextureUnorderedAccessViewVulkan(
  ezGALTexture* pResource, const ezGALTextureUnorderedAccessViewCreationDescription& Description)
  : ezGALTextureUnorderedAccessView(pResource, Description)
{
}

ezGALTextureUnorderedAccessViewVulkan::~ezGALTextureUnorderedAccessViewVulkan() = default;

ezResult ezGALTextureUnorderedAccessViewVulkan::InitPlatform(ezGALDevice* pDevice)
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

  ezGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat == ezGALResourceFormat::Invalid ? texDesc.m_Format : m_Description.m_OverrideViewFormat;
  vk::ImageViewCreateInfo viewCreateInfo;
  viewCreateInfo.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_format;
  viewCreateInfo.image = image;
  viewCreateInfo.subresourceRange = ezConversionUtilsVulkan::GetSubresourceRange(texDesc, m_Description);
  const ezEnum<ezGALTextureType> type = m_Description.m_OverrideViewType != ezGALTextureType::Invalid ? m_Description.m_OverrideViewType : pTexture->GetDescription().m_Type;
  viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(type);

  m_resourceImageInfo.imageLayout = vk::ImageLayout::eGeneral;

  m_range = viewCreateInfo.subresourceRange;
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));
  pVulkanDevice->SetDebugName("UAV-Texture", m_resourceImageInfo.imageView);

  return EZ_SUCCESS;
}

ezResult ezGALTextureUnorderedAccessViewVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_resourceImageInfo.imageView);
  m_resourceImageInfo = vk::DescriptorImageInfo();
  return EZ_SUCCESS;
}

/////////////////////////////////////////////////////

const vk::DescriptorBufferInfo& ezGALBufferUnorderedAccessViewVulkan::GetBufferInfo() const
{
  // Vulkan buffers get constantly swapped out for new ones so the vk::Buffer pointer is not persistent.
  // We need to acquire the latest one on every request for rendering.
  m_resourceBufferInfo.buffer = static_cast<const ezGALBufferVulkan*>(GetResource())->GetVkBuffer();
  return m_resourceBufferInfo;
}

ezGALBufferUnorderedAccessViewVulkan::ezGALBufferUnorderedAccessViewVulkan(
  ezGALBuffer* pResource, const ezGALBufferUnorderedAccessViewCreationDescription& Description)
  : ezGALBufferUnorderedAccessView(pResource, Description)
{
}

ezGALBufferUnorderedAccessViewVulkan::~ezGALBufferUnorderedAccessViewVulkan() = default;

ezResult ezGALBufferUnorderedAccessViewVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  const ezGALBuffer* pBuffer = pDevice->GetBuffer(m_Description.m_hBuffer);
  auto pParentBuffer = static_cast<const ezGALBufferVulkan*>(pBuffer);

  m_resourceBufferInfo.offset = m_Description.m_uiByteOffset;
  m_resourceBufferInfo.range = m_Description.m_uiByteCount;

  if (m_Description.m_ResourceType == ezGALShaderResourceType::TexelBufferRW)
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

ezResult ezGALBufferUnorderedAccessViewVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  m_resourceBufferInfo = vk::DescriptorBufferInfo();
  pVulkanDevice->DeleteLater(m_bufferView);
  return EZ_SUCCESS;
}
