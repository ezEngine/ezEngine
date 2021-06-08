#include <RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>

bool IsArrayView(const ezGALTextureCreationDescription& texDesc, const ezGALResourceViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
}

ezGALResourceViewVulkan::ezGALResourceViewVulkan(ezGALResourceBase* pResource, const ezGALResourceViewCreationDescription& Description)
  : ezGALResourceView(pResource, Description)
  , m_resourceBinding{}
  , m_resourceBindingData{}
{
}

ezGALResourceViewVulkan::~ezGALResourceViewVulkan() {}

ezResult ezGALResourceViewVulkan::InitPlatform(ezGALDevice* pDevice)
{
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

  ezGALResourceFormat::Enum ViewFormat = m_Description.m_OverrideViewFormat;

  if (pTexture)
  {
    if (ViewFormat == ezGALResourceFormat::Invalid)
      ViewFormat = pTexture->GetDescription().m_Format;
  }
  else if (pBuffer)
  {
    if (ViewFormat == ezGALResourceFormat::Invalid)
      ViewFormat = ezGALResourceFormat::RUInt;

    if (!pBuffer->GetDescription().m_bAllowRawViews && m_Description.m_bRawView)
    {
      ezLog::Error("Trying to create a raw view for a buffer with no raw view flag is invalid!");
      return EZ_FAILURE;
    }
  }

  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  m_resourceBinding.descriptorCount = 1;
  m_resourceBindingData.descriptorCount = 1;

  m_resourceBinding.descriptorType = pTexture ? vk::DescriptorType::eCombinedImageSampler : vk::DescriptorType::eStorageBuffer;
  m_resourceBindingData.descriptorType = m_resourceBinding.descriptorType;
  m_resourceBindingData.pBufferInfo = pTexture ? nullptr : &m_resourceBufferInfo;
  m_resourceBindingData.pImageInfo = pTexture ? &m_resourceImageInfo : nullptr;

  if (pTexture)
  {
    auto image = static_cast<const ezGALTextureVulkan*>(pTexture->GetParentResource())->GetImage();
    const ezGALTextureCreationDescription& texDesc = pTexture->GetDescription();

    const bool bIsArrayView = IsArrayView(texDesc, m_Description);

    m_resourceImageInfo.imageLayout = vk::ImageLayout::eGeneral;
    ezGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat == ezGALResourceFormat::Invalid ? texDesc.m_Format : m_Description.m_OverrideViewFormat;
    vk::ImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;
    viewCreateInfo.image = image;
    viewCreateInfo.subresourceRange.aspectMask = ezGALResourceFormat::IsDepthFormat(viewFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
    viewCreateInfo.subresourceRange.layerCount = m_Description.m_uiArraySize;
    viewCreateInfo.subresourceRange.levelCount = m_Description.m_uiMipLevelsToUse;

    switch (texDesc.m_Type)
    {
      case ezGALTextureType::Texture2D:
      case ezGALTextureType::Texture2DProxy:

        if (!bIsArrayView)
        {
          // TODO what to to about multisampled textures in vulkan
          // views/descriptors?
          //if (texDesc.m_SampleCount == ezGALMSAASampleCount::None)
          //{
          viewCreateInfo.viewType = vk::ImageViewType::e2D;
          //}
          //else
          //{
          //
          //}
        }
        else
        {
          //if (texDesc.m_SampleCount == ezGALMSAASampleCount::None)
          //{
          viewCreateInfo.viewType = vk::ImageViewType::e2DArray;
          viewCreateInfo.subresourceRange.baseArrayLayer = m_Description.m_uiFirstArraySlice;
          viewCreateInfo.subresourceRange.baseMipLevel = m_Description.m_uiMostDetailedMipLevel;
          //}
          //else
          //{
          //}
        }

        break;

      case ezGALTextureType::TextureCube:

        if (!bIsArrayView)
        {
          viewCreateInfo.viewType = vk::ImageViewType::eCube;
        }
        else
        {
          viewCreateInfo.viewType = vk::ImageViewType::eCubeArray;
          viewCreateInfo.subresourceRange.baseArrayLayer = m_Description.m_uiFirstArraySlice;
          viewCreateInfo.subresourceRange.baseMipLevel = m_Description.m_uiMostDetailedMipLevel;
        }

        break;

      case ezGALTextureType::Texture3D:

        viewCreateInfo.viewType = vk::ImageViewType::e3D;

        break;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        return EZ_FAILURE;
    }

    m_imageView = pVulkanDevice->GetVulkanDevice().createImageView(viewCreateInfo);

    if (!m_imageView)
    {
      return EZ_FAILURE;
    }

    m_resourceImageInfo.imageView = m_imageView;
  }
  else if (pBuffer)
  {
    vk::Buffer buffer = static_cast<const ezGALBufferVulkan*>(pBuffer)->GetVkBuffer();

    m_resourceBufferInfo.buffer = buffer;
    m_resourceBufferInfo.offset = m_Description.m_uiFirstElement;
    m_resourceBufferInfo.range = m_Description.m_uiNumElements;
  }

  return EZ_SUCCESS;
}

ezResult ezGALResourceViewVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  if (m_imageView)
  {
    pVulkanDevice->GetVulkanDevice().destroyImageView(m_imageView);
  }

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_ResourceViewVulkan);
