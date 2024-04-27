#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

bool IsArrayView(const ezGALTextureCreationDescription& texDesc, const ezGALResourceViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiArraySize > 1;
}

const vk::DescriptorBufferInfo& ezGALResourceViewVulkan::GetBufferInfo() const
{
  // Vulkan buffers get constantly swapped out for new ones so the vk::Buffer pointer is not persistent.
  // We need to acquire the latest one on every request for rendering.
  m_resourceBufferInfo.buffer = static_cast<const ezGALBufferVulkan*>(GetResource())->GetVkBuffer();
  return m_resourceBufferInfo;
}

ezGALResourceViewVulkan::ezGALResourceViewVulkan(ezGALResourceBase* pResource, const ezGALResourceViewCreationDescription& Description)
  : ezGALResourceView(pResource, Description)
{
}

ezGALResourceViewVulkan::~ezGALResourceViewVulkan() {}

ezResult ezGALResourceViewVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  const ezGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  const ezGALBuffer* pBuffer = nullptr;
  if (!m_Description.m_hBuffer.IsInvalidated())
    pBuffer = pDevice->GetBuffer(m_Description.m_hBuffer);

  if (pTexture == nullptr && pBuffer == nullptr)
  {
    ezLog::Error("No valid texture handle or buffer handle given for resource view creation!");
    return EZ_FAILURE;
  }

  if (pTexture)
  {
    auto pParentTexture = static_cast<const ezGALTextureVulkan*>(pTexture->GetParentResource());
    auto image = pParentTexture->GetImage();
    const ezGALTextureCreationDescription& texDesc = pTexture->GetDescription();

    const bool bIsArrayView = IsArrayView(texDesc, m_Description);
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
    if (texDesc.m_Type == ezGALTextureType::Texture3D) // no array support
    {
      viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(texDesc.m_Type, false);
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));
    }
    else if (m_Description.m_uiArraySize == 1) // can be array or not
    {
      viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(texDesc.m_Type, false);
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));
      viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(texDesc.m_Type, true);
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfoArray.imageView));
    }
    else // Can only be array
    {
      viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(texDesc.m_Type, true);
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfoArray.imageView));
    }
  }
  else if (pBuffer)
  {
    if (!pBuffer->GetDescription().m_bAllowRawViews && m_Description.m_bRawView)
    {
      ezLog::Error("Trying to create a raw view for a buffer with no raw view flag is invalid!");
      return EZ_FAILURE;
    }

    auto pParentBuffer = static_cast<const ezGALBufferVulkan*>(pBuffer);
    if (pBuffer->GetDescription().m_bUseAsStructuredBuffer)
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
      ezGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat;
      if (viewFormat == ezGALResourceFormat::Invalid)
        viewFormat = ezGALResourceFormat::RUInt;

      vk::BufferViewCreateInfo viewCreateInfo;
      viewCreateInfo.buffer = pParentBuffer->GetVkBuffer();
      viewCreateInfo.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_format;
      viewCreateInfo.offset = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
      viewCreateInfo.range = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;

      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createBufferView(&viewCreateInfo, nullptr, &m_bufferView));
    }
  }

  return EZ_SUCCESS;
}

ezResult ezGALResourceViewVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_resourceImageInfo.imageView);
  pVulkanDevice->DeleteLater(m_resourceImageInfoArray.imageView);
  m_resourceImageInfo = vk::DescriptorImageInfo();
  m_resourceImageInfoArray = vk::DescriptorImageInfo();
  m_resourceBufferInfo = vk::DescriptorBufferInfo();
  pVulkanDevice->DeleteLater(m_bufferView);
  return EZ_SUCCESS;
}


