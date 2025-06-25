#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Memory/MemoryUtils.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

vk::Extent3D ezGALTextureVulkan::GetMipLevelSize(const ezGALTextureCreationDescription& description, ezUInt32 uiMipLevel)
{
  vk::Extent3D size = {description.m_uiWidth, description.m_uiHeight, description.m_uiDepth};
  size.width = ezMath::Max(1u, size.width >> uiMipLevel);
  size.height = ezMath::Max(1u, size.height >> uiMipLevel);
  size.depth = ezMath::Max(1u, size.depth >> uiMipLevel);
  return size;
}

vk::ImageSubresourceRange ezGALTextureVulkan::GetFullRange() const
{
  vk::ImageSubresourceRange range;
  range.aspectMask = GetAspectMask();
  range.baseArrayLayer = 0;
  range.baseMipLevel = 0;
  range.layerCount = m_Description.m_uiArraySize;
  range.levelCount = m_Description.m_uiMipLevelCount;

  if (m_Description.m_Type == ezGALTextureType::TextureCube || m_Description.m_Type == ezGALTextureType::TextureCubeArray)
  {
    range.layerCount *= 6;
  }

  return range;
}

vk::ImageAspectFlags ezGALTextureVulkan::GetAspectMask() const
{
  vk::ImageAspectFlags mask = ezConversionUtilsVulkan::IsDepthFormat(m_imageFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (ezConversionUtilsVulkan::IsStencilFormat(m_imageFormat))
    mask |= vk::ImageAspectFlagBits::eStencil;
  return mask;
}

vk::DescriptorImageInfo ezGALTextureVulkan::GetDescriptorImageInfo(ezGALTextureRange textureRange, ezEnum<ezGALShaderResourceType> resourceType, ezEnum<ezGALShaderTextureType> textureType, ezEnum<ezGALResourceFormat> overrideViewFormat) const
{
  vk::DescriptorImageInfo imageInfo;
  View view;
  view.m_TextureRange = textureRange;
  view.m_ResourceType = resourceType;
  view.m_TextureType = textureType;
  view.m_OverrideViewFormat = overrideViewFormat;

  if (!m_TextureViews.TryGetValue(view, imageInfo))
  {
    const ezGALResourceFormat::Enum viewFormat = overrideViewFormat == ezGALResourceFormat::Invalid ? m_Description.m_Format : overrideViewFormat;

    vk::ImageViewCreateInfo viewCreateInfo;
    viewCreateInfo.image = m_image;
    viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(textureType);
    viewCreateInfo.format = m_pDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_format;
    viewCreateInfo.subresourceRange = ezConversionUtilsVulkan::GetSubresourceRange(viewFormat, textureRange);
    viewCreateInfo.subresourceRange.aspectMask &= ~vk::ImageAspectFlagBits::eStencil;

    VK_ASSERT_DEV(m_pDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &imageInfo.imageView));
    imageInfo.imageLayout = ezConversionUtilsVulkan::GetDefaultLayout(m_imageFormat);
    if (resourceType == ezGALShaderResourceType::TextureRW)
      imageInfo.imageLayout = vk::ImageLayout::eGeneral;
    m_TextureViews.Insert(view, imageInfo);
  }

  return imageInfo;
}

ezGALTextureVulkan::ezGALTextureVulkan(const ezGALTextureCreationDescription& Description)
  : ezGALTexture(Description)
{
}

ezGALTextureVulkan::~ezGALTextureVulkan() = default;

ezResult ezGALTextureVulkan::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  m_pDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  vk::ImageFormatListCreateInfo imageFormats;
  vk::ImageCreateInfo createInfo = {};

  m_imageFormat = ComputeImageFormat(m_pDevice, m_Description.m_Format, createInfo, imageFormats);
  ComputeCreateInfo(m_pDevice, m_Description, createInfo, m_stages, m_access, m_preferredLayout);

  if (m_Description.m_pExisitingNativeObject != nullptr)
  {
    m_image = static_cast<VkImage>(m_Description.m_pExisitingNativeObject);
    m_pDevice->GetInitContext().InitTexture(this, createInfo, pInitialData);
  }
  else
  {
    ezVulkanAllocationCreateInfo allocInfo;
    ComputeAllocInfo(allocInfo);

    vk::ImageFormatProperties props2;
    VK_ASSERT_DEBUG(m_pDevice->GetVulkanPhysicalDevice().getImageFormatProperties(createInfo.format, createInfo.imageType, createInfo.tiling, createInfo.usage, createInfo.flags, &props2));
    VK_SUCCEED_OR_RETURN_EZ_FAILURE(ezMemoryAllocatorVulkan::CreateImage(createInfo, allocInfo, m_image, m_alloc, &m_allocInfo));
    m_pDevice->GetInitContext().InitTexture(this, createInfo, pInitialData);
  }
  return EZ_SUCCESS;
}


vk::Format ezGALTextureVulkan::ComputeImageFormat(const ezGALDeviceVulkan* pDevice, ezEnum<ezGALResourceFormat> galFormat, vk::ImageCreateInfo& ref_createInfo, vk::ImageFormatListCreateInfo& ref_imageFormats)
{
  const ezGALFormatLookupEntryVulkan& format = pDevice->GetFormatLookupTable().GetFormatInfo(galFormat);

  ref_createInfo.flags |= vk::ImageCreateFlagBits::eMutableFormat;
  if (pDevice->GetExtensions().m_bImageFormatList && !format.m_mutableFormats.IsEmpty())
  {
    ref_createInfo.pNext = &ref_imageFormats;

    ref_imageFormats.viewFormatCount = format.m_mutableFormats.GetCount();
    ref_imageFormats.pViewFormats = format.m_mutableFormats.GetData();
  }

  ref_createInfo.format = format.m_format;
  return ref_createInfo.format;
}

void ezGALTextureVulkan::ComputeCreateInfo(const ezGALDeviceVulkan* m_pDevice, const ezGALTextureCreationDescription& m_Description, vk::ImageCreateInfo& createInfo, vk::PipelineStageFlags& m_stages, vk::AccessFlags& m_access, vk::ImageLayout& m_preferredLayout)
{
  EZ_ASSERT_DEBUG(createInfo.format != vk::Format::eUndefined, "No storage format available for given format: {0}", m_Description.m_Format);

  const bool bIsDepth = ezConversionUtilsVulkan::IsDepthFormat(createInfo.format);

  m_stages = vk::PipelineStageFlagBits::eTransfer;
  m_access = vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite;
  m_preferredLayout = vk::ImageLayout::eGeneral;

  createInfo.initialLayout = vk::ImageLayout::eUndefined;
  createInfo.sharingMode = vk::SharingMode::eExclusive;
  createInfo.pQueueFamilyIndices = nullptr;
  createInfo.queueFamilyIndexCount = 0;
  createInfo.tiling = vk::ImageTiling::eOptimal;
  createInfo.usage |= vk::ImageUsageFlagBits::eTransferDst;
  // eTransferSrc can be set on everything without any negative effects.
  // #TODO_VULKAN eSampled not needed if we only allow buffer readback or is it necessary for depth?
  createInfo.usage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled;

  createInfo.extent.width = m_Description.m_uiWidth;
  createInfo.extent.height = m_Description.m_uiHeight;
  createInfo.extent.depth = m_Description.m_uiDepth;
  createInfo.mipLevels = m_Description.m_uiMipLevelCount;

  createInfo.samples = static_cast<vk::SampleCountFlagBits>(m_Description.m_SampleCount.GetValue());

  // m_bAllowDynamicMipGeneration has to be emulated via a shader so we need to enable shader resource view and render target support.
  if (m_Description.m_bAllowShaderResourceView || m_Description.m_bAllowDynamicMipGeneration)
  {
    // Needed for blit-based generation
    createInfo.usage |= vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
    // Needed for shader-based generation
    createInfo.usage |= vk::ImageUsageFlagBits::eSampled;
    m_stages |= m_pDevice->GetSupportedStages();
    m_access |= vk::AccessFlagBits::eShaderRead;
    m_preferredLayout = ezConversionUtilsVulkan::GetDefaultLayout(createInfo.format);
  }
  // VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT
  if (bIsDepth)
  {
    // This looks wrong but apparently, even if you don't intend to render to a depth texture, you need to set the eDepthStencilAttachment flag.
    // VUID-VkImageMemoryBarrier-oldLayout-01210: https://vulkan.lunarg.com/doc/view/1.3.275.0/windows/1.3-extensions/vkspec.html#VUID-VkImageMemoryBarrier-oldLayout-01210
    // If srcQueueFamilyIndex and dstQueueFamilyIndex define a queue family ownership transfer or oldLayout and newLayout define an image layout transition, and oldLayout or newLayout is VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL then image must have been created with VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    createInfo.usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
  }

  if (m_Description.m_bAllowRenderTargetView || m_Description.m_bAllowDynamicMipGeneration)
  {
    if (bIsDepth)
    {
      m_stages |= vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
      m_access |= vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
      m_preferredLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    }
    else
    {
      createInfo.usage |= vk::ImageUsageFlagBits::eColorAttachment;
      m_stages |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
      m_access |= vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
      m_preferredLayout = vk::ImageLayout::eColorAttachmentOptimal;
    }
  }

  if (m_Description.m_bAllowUAV)
  {
    createInfo.usage |= vk::ImageUsageFlagBits::eStorage;
    m_stages |= m_pDevice->GetSupportedStages();
    m_access |= vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
    m_preferredLayout = vk::ImageLayout::eGeneral;
  }

  switch (m_Description.m_Type)
  {
    case ezGALTextureType::TextureCube:
    case ezGALTextureType::TextureCubeArray:
      createInfo.imageType = vk::ImageType::e2D;
      createInfo.flags |= vk::ImageCreateFlagBits::eCubeCompatible;
      createInfo.arrayLayers = m_Description.m_uiArraySize * 6;
      break;

    case ezGALTextureType::Texture2D:
    case ezGALTextureType::Texture2DArray:
    case ezGALTextureType::Texture2DShared:
    {
      createInfo.imageType = vk::ImageType::e2D;
      createInfo.arrayLayers = m_Description.m_uiArraySize;
    }
    break;

    case ezGALTextureType::Texture3D:
    {
      createInfo.arrayLayers = 1;
      createInfo.imageType = vk::ImageType::e3D;
      if (m_Description.m_bAllowRenderTargetView)
      {
        createInfo.flags |= vk::ImageCreateFlagBits::e2DArrayCompatible;
      }
    }
    break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
}

void ezGALTextureVulkan::ComputeAllocInfo(ezVulkanAllocationCreateInfo& allocInfo)
{
  allocInfo.m_usage = ezVulkanMemoryUsage::Auto;
}

ezUInt32 ezGALTextureVulkan::ComputeSubResourceOffsets(const ezGALDeviceVulkan* pDevice, const ezGALTextureCreationDescription& description, ezDynamicArray<SubResourceOffset>& subResourceSizes)
{
  const ezUInt32 alignment = (ezUInt32)ezGALBufferVulkan::GetAlignment(pDevice, vk::BufferUsageFlagBits::eTransferDst);
  const vk::Format stagingFormat = pDevice->GetFormatLookupTable().GetFormatInfo(description.m_Format).m_readback;
  const ezUInt8 uiBlockSize = vk::blockSize(stagingFormat);
  const auto blockExtent = vk::blockExtent(stagingFormat);
  const ezUInt32 arrayLayers = (description.m_Type == ezGALTextureType::TextureCube || description.m_Type == ezGALTextureType::TextureCubeArray) ? (description.m_uiArraySize * 6) : description.m_uiArraySize;
  const ezUInt32 mipLevels = description.m_uiMipLevelCount;

  subResourceSizes.Reserve(arrayLayers * mipLevels);
  ezUInt32 uiOffset = 0;
  for (ezUInt32 uiLayer = 0; uiLayer < arrayLayers; uiLayer++)
  {
    for (ezUInt32 uiMipLevel = 0; uiMipLevel < mipLevels; uiMipLevel++)
    {
      const ezUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * mipLevels;
      EZ_ASSERT_DEBUG(subResourceSizes.GetCount() == uiSubresourceIndex, "");

      const vk::Extent3D imageExtent = GetMipLevelSize(description, uiMipLevel);
      const VkExtent3D blockCount = {
        (imageExtent.width + blockExtent[0] - 1) / blockExtent[0],
        (imageExtent.height + blockExtent[1] - 1) / blockExtent[1],
        (imageExtent.depth + blockExtent[2] - 1) / blockExtent[2]};

      const ezUInt32 uiTotalSize = uiBlockSize * blockCount.width * blockCount.height * blockCount.depth;
      subResourceSizes.PushBack({uiOffset, uiTotalSize, blockCount.width / blockExtent[0], blockCount.height / blockExtent[1]});
      uiOffset += ezMemoryUtils::AlignSize(uiTotalSize, alignment);
    }
  }
  return uiOffset;
}

ezResult ezGALTextureVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  if (m_image && !m_Description.m_pExisitingNativeObject)
  {
    pVulkanDevice->DeleteLater(m_image, m_alloc);
    m_allocInfo = {};
  }
  m_image = nullptr;

  for (auto it : m_TextureViews)
  {
    pVulkanDevice->DeleteLater(it.Value().imageView);
  }
  m_TextureViews.Clear();

  return EZ_SUCCESS;
}

void ezGALTextureVulkan::SetDebugNamePlatform(const char* szName) const
{
  m_pDevice->SetDebugName(szName, m_image, m_alloc);
}
