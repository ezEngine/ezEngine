#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

vk::Extent3D ezGALTextureVulkan::GetMipLevelSize(ezUInt32 uiMipLevel) const
{
  vk::Extent3D size = {m_Description.m_uiWidth, m_Description.m_uiHeight, m_Description.m_uiDepth};
  size.width = ezMath::Max(1u, size.width >> uiMipLevel);
  size.height = ezMath::Max(1u, size.height >> uiMipLevel);
  size.depth = ezMath::Max(1u, size.depth >> uiMipLevel);
  return size;
}

vk::ImageSubresourceRange ezGALTextureVulkan::GetFullRange() const
{
  vk::ImageSubresourceRange range;
  range.aspectMask = ezGALResourceFormat::IsDepthFormat(m_Description.m_Format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (ezGALResourceFormat::IsStencilFormat(m_Description.m_Format))
    range.aspectMask |= vk::ImageAspectFlagBits::eStencil;
  range.baseArrayLayer = 0;
  range.baseMipLevel = 0;
  range.layerCount = m_Description.m_Type == ezGALTextureType::TextureCube ? m_Description.m_uiArraySize * 6 : m_Description.m_uiArraySize;
  range.levelCount = m_Description.m_uiMipLevelCount;
  return range;
}

vk::ImageAspectFlags ezGALTextureVulkan::GetStagingAspectMask() const
{
  vk::ImageAspectFlags mask = ezGALResourceFormat::IsDepthFormat(m_Description.m_Format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (ezGALResourceFormat::IsStencilFormat(m_Description.m_Format))
    mask |= vk::ImageAspectFlagBits::eStencil;
  if (ezGALResourceFormat::IsDepthFormat(m_Description.m_Format))
    mask = vk::ImageAspectFlagBits::eColor;
  return mask;
}

ezGALTextureVulkan::ezGALTextureVulkan(const ezGALTextureCreationDescription& Description)
  : ezGALTexture(Description)
  , m_image(nullptr)
  , m_pExisitingNativeObject(Description.m_pExisitingNativeObject)
{
}

ezGALTextureVulkan::ezGALTextureVulkan(const ezGALTextureCreationDescription& Description, vk::Format OverrideFormat)
  : ezGALTexture(Description)
  , m_image(nullptr)
  , m_pExisitingNativeObject(Description.m_pExisitingNativeObject)
  , m_imageFormat(OverrideFormat)
  , m_formatOverride(true)
{
}

ezGALTextureVulkan::~ezGALTextureVulkan() {}

ezResult ezGALTextureVulkan::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  ezGALDeviceVulkan* m_pDeviceVulkan = static_cast<ezGALDeviceVulkan*>(pDevice);
  m_pDevice = m_pDeviceVulkan;
  m_device = m_pDeviceVulkan->GetVulkanDevice();

  vk::ImageCreateInfo createInfo = {};
  //#TODO_VULKAN VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT / VkImageFormatListCreateInfoKHR to allow changing the format in a view is slow.
  createInfo.flags |= vk::ImageCreateFlagBits::eMutableFormat;
  if (m_imageFormat == vk::Format::eUndefined)
  {
    m_imageFormat = m_pDeviceVulkan->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_eStorage;
  }
  createInfo.format = m_imageFormat;
  if (createInfo.format == vk::Format::eUndefined)
  {
    ezLog::Error("No storage format available for given format: {0}", m_Description.m_Format);
    return EZ_FAILURE;
  }
  const bool bIsDepth = ezGALResourceFormat::IsDepthFormat(m_Description.m_Format);

  m_stages = vk::PipelineStageFlagBits::eTransfer;
  m_access = vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite;
  m_currentLayout = vk::ImageLayout::eUndefined;
  m_preferredLayout = vk::ImageLayout::eGeneral;

  createInfo.initialLayout = vk::ImageLayout::eUndefined;
  createInfo.sharingMode = vk::SharingMode::eExclusive;
  createInfo.pQueueFamilyIndices = nullptr;
  createInfo.queueFamilyIndexCount = 0;
  createInfo.tiling = vk::ImageTiling::eOptimal;
  createInfo.usage |= vk::ImageUsageFlagBits::eTransferDst;
  if (m_Description.m_ResourceAccess.m_bReadBack)
    createInfo.usage |= vk::ImageUsageFlagBits::eTransferSrc;

  // TODO are these correctly populated or do they contain meaningless values depending on
  // the texture type indicated?
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
    m_stages |= m_pDeviceVulkan->GetSupportedStages();
    m_access |= vk::AccessFlagBits::eShaderRead;
    m_preferredLayout = bIsDepth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
  }

  if (m_Description.m_bCreateRenderTarget || m_Description.m_bAllowDynamicMipGeneration)
  {
    if (bIsDepth)
    {
      createInfo.usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
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
    m_stages |= m_pDeviceVulkan->GetSupportedStages();
    m_access |= vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
    m_preferredLayout = vk::ImageLayout::eGeneral;
  }

  switch (m_Description.m_Type)
  {
    case ezGALTextureType::Texture2D:
    case ezGALTextureType::TextureCube:
    {
      createInfo.imageType = vk::ImageType::e2D;
      createInfo.arrayLayers = (m_Description.m_Type == ezGALTextureType::Texture2D ? m_Description.m_uiArraySize : (m_Description.m_uiArraySize * 6));

      if (m_Description.m_Type == ezGALTextureType::TextureCube)
        createInfo.flags |= vk::ImageCreateFlagBits::eCubeCompatible;
    }
    break;

    case ezGALTextureType::Texture3D:
    {
      createInfo.arrayLayers = 1;
      createInfo.imageType = vk::ImageType::e3D;
      if (m_Description.m_bCreateRenderTarget)
      {
        createInfo.flags |= vk::ImageCreateFlagBits::e2DArrayCompatible;
      }
    }
    break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return EZ_FAILURE;
  }


  if (m_pExisitingNativeObject == nullptr)
  {
    ezVulkanAllocationCreateInfo allocInfo;
    allocInfo.m_usage = ezVulkanMemoryUsage::Auto;
    VK_SUCCEED_OR_RETURN_EZ_FAILURE(ezMemoryAllocatorVulkan::CreateImage(createInfo, allocInfo, m_image, m_alloc, &m_allocInfo));
  }
  else
  {
    m_image = static_cast<VkImage>(m_pExisitingNativeObject);
  }
  m_pDeviceVulkan->GetInitContext().InitTexture(this, createInfo, pInitialData);

  if (m_Description.m_ResourceAccess.m_bReadBack)
  {
    // We want the staging texture to always have the intended format
    // and not the override format given by the parent texture
    createInfo.format = m_pDeviceVulkan->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_eStorage;
    return CreateStagingBuffer(m_pDeviceVulkan, createInfo);
  }

  return EZ_SUCCESS;
}

ezResult ezGALTextureVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  if (m_image && !m_pExisitingNativeObject)
  {
    pVulkanDevice->DeleteLater(m_image, m_alloc);
  }
  m_image = nullptr;

  pVulkanDevice->DeleteLater(m_stagingImage, m_stagingAlloc);

  return EZ_SUCCESS;
}

void ezGALTextureVulkan::SetDebugNamePlatform(const char* szName) const
{
  m_pDevice->SetDebugName(szName, m_image, m_alloc);
  if (m_stagingImage)
  {
    m_pDevice->SetDebugName(szName, m_stagingImage, m_stagingAlloc);
  }
}

ezResult ezGALTextureVulkan::CreateStagingBuffer(ezGALDeviceVulkan* pDevice, const vk::ImageCreateInfo& imageCreateInfo)
{
  vk::ImageCreateInfo stagingImageCreateInfo = imageCreateInfo;

  stagingImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
  stagingImageCreateInfo.tiling = vk::ImageTiling::eLinear;
  stagingImageCreateInfo.usage = vk::ImageUsageFlagBits::eTransferDst;
  stagingImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

  ezVulkanAllocationCreateInfo allocInfo;
  allocInfo.m_usage = ezVulkanMemoryUsage::Auto;
  allocInfo.m_flags = ezVulkanAllocationCreateFlags::HostAccessRandom;
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(ezMemoryAllocatorVulkan::CreateImage(stagingImageCreateInfo, allocInfo, m_stagingImage, m_stagingAlloc, &m_stagingAllocInfo));

  m_stagingImageFormat = imageCreateInfo.format;

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_TextureVulkan);
