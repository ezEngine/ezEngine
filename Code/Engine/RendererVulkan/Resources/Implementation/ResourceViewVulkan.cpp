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
  m_resourceImageInfoArray.imageLayout = m_resourceImageInfo.imageLayout;

  m_range = viewCreateInfo.subresourceRange;

  switch (texDesc.m_Type)
  {
    case ezGALTextureType::Texture2D: // can be array or not
      viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(ezGALTextureType::Texture2D);
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));

      viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(ezGALTextureType::Texture2DArray);
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfoArray.imageView));
      break;

    case ezGALTextureType::Texture2DProxy: // Can only be array
    case ezGALTextureType::Texture2DArray:
    case ezGALTextureType::TextureCubeArray:
      viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(texDesc.m_Type);
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfoArray.imageView));
      break;

    case ezGALTextureType::Texture3D: // no array support
    case ezGALTextureType::Texture2DShared:
      viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(texDesc.m_Type);
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));
      break;

    case ezGALTextureType::TextureCube: // can be array or not
      viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(ezGALTextureType::TextureCube);
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));

      viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(ezGALTextureType::TextureCubeArray);
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfoArray.imageView));
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (texDesc.m_Type == ezGALTextureType::Texture3D)
  {
  }
  else if (m_Description.m_uiArraySize == 1) // can be array or not
  {
  }
  else
  {
  }

  return EZ_SUCCESS;
}

ezResult ezGALTextureResourceViewVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_resourceImageInfo.imageView);
  pVulkanDevice->DeleteLater(m_resourceImageInfoArray.imageView);
  m_resourceImageInfo = vk::DescriptorImageInfo();
  m_resourceImageInfoArray = vk::DescriptorImageInfo();
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

  const ezGALBuffer* pBuffer = nullptr;
  if (!m_Description.m_hBuffer.IsInvalidated())
    pBuffer = pDevice->GetBuffer(m_Description.m_hBuffer);

  if (pBuffer == nullptr)
  {
    ezLog::Error("No valid buffer handle given for resource view creation!");
    return EZ_FAILURE;
  }

  if (!pBuffer->GetDescription().m_BufferFlags.IsSet(ezGALBufferUsageFlags::ByteAddressBuffer) && m_Description.m_bRawView)
  {
    ezLog::Error("Trying to create a raw view for a buffer with no raw view flag is invalid!");
    return EZ_FAILURE;
  }

  auto pParentBuffer = static_cast<const ezGALBufferVulkan*>(pBuffer);
  if (pBuffer->GetDescription().m_BufferFlags.IsSet(ezGALBufferUsageFlags::StructuredBuffer))
  {
    m_resourceBufferInfo.offset = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
    m_resourceBufferInfo.range = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;
  }
  else if (m_Description.m_bRawView)
  {
    m_resourceBufferInfo.offset = sizeof(ezUInt32) * m_Description.m_uiFirstElement;
    m_resourceBufferInfo.range = sizeof(ezUInt32) * m_Description.m_uiNumElements;
  }
  else
  {
    ezGALResourceFormat::Enum viewFormat = m_Description.m_Format;
    if (viewFormat == ezGALResourceFormat::Invalid)
      viewFormat = ezGALResourceFormat::RUInt;

    vk::BufferViewCreateInfo viewCreateInfo;
    viewCreateInfo.buffer = pParentBuffer->GetVkBuffer();
    viewCreateInfo.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_format;
    viewCreateInfo.offset = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
    viewCreateInfo.range = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;

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
