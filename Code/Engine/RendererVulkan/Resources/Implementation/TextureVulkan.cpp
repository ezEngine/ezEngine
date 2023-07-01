#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/System/Process.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <errno.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#endif

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
  range.aspectMask = GetAspectMask();
  range.baseArrayLayer = 0;
  range.baseMipLevel = 0;
  range.layerCount = m_Description.m_Type == ezGALTextureType::TextureCube ? m_Description.m_uiArraySize * 6 : m_Description.m_uiArraySize;
  range.levelCount = m_Description.m_uiMipLevelCount;
  return range;
}

vk::ImageAspectFlags ezGALTextureVulkan::GetAspectMask() const
{
  vk::ImageAspectFlags mask = ezConversionUtilsVulkan::IsDepthFormat(m_imageFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (ezConversionUtilsVulkan::IsStencilFormat(m_imageFormat))
    mask |= vk::ImageAspectFlagBits::eStencil;
  return mask;
}

ezGALTextureVulkan::ezGALTextureVulkan(const ezGALTextureCreationDescription& Description, ezGALSharedTextureType sharedType, ezGALPlatformSharedHandle hSharedHandle)
  : ezGALTexture(Description)
  , m_image(nullptr)
  , m_pExisitingNativeObject(Description.m_pExisitingNativeObject)
  , m_sharedType(sharedType)
  , m_sharedHandle(hSharedHandle)
{
}

ezGALTextureVulkan::ezGALTextureVulkan(const ezGALTextureCreationDescription& Description, vk::Format OverrideFormat, bool bLinearCPU)
  : ezGALTexture(Description)
  , m_image(nullptr)
  , m_pExisitingNativeObject(Description.m_pExisitingNativeObject)
  , m_imageFormat(OverrideFormat)
  , m_formatOverride(true)
  , m_bLinearCPU(bLinearCPU)
{
}

ezGALTextureVulkan::~ezGALTextureVulkan() {}

ezResult ezGALTextureVulkan::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  m_pDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  vk::ImageFormatListCreateInfo imageFormats;
  vk::ImageCreateInfo createInfo = {};

  if (m_imageFormat == vk::Format::eUndefined)
  {
    const ezGALFormatLookupEntryVulkan& format = m_pDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format);
    m_imageFormat = format.m_format;

    createInfo.flags |= vk::ImageCreateFlagBits::eMutableFormat;
    if (m_pDevice->GetExtensions().m_bImageFormatList && !format.m_mutableFormats.IsEmpty())
    {
      createInfo.pNext = &imageFormats;

      imageFormats.viewFormatCount = format.m_mutableFormats.GetCount();
      imageFormats.pViewFormats = format.m_mutableFormats.GetData();
    }
  }
  else
  {
    // #TODO_VULKAN if m_imageFormat is set, than means that we are replacing the texture format with another because the swapchain surface does not support the target format. This creates a bit of a conundrum: If we want to support mutable formats in this case, we would need to swap the texture view's format as well and this hack would continue ripple through the code base. Probably best to revisit the GetAlternativeFormat logic in ezGALSwapChainVulkan.
    // For now, don't support mutable formats until we run into problems.
  }

  if ((m_imageFormat == vk::Format::eR8G8B8A8Srgb || m_imageFormat == vk::Format::eB8G8R8A8Unorm) && m_Description.m_bCreateRenderTarget)
  {
    // printf("");
  }
  createInfo.format = m_imageFormat;
  if (createInfo.format == vk::Format::eUndefined)
  {
    ezLog::Error("No storage format available for given format: {0}", m_Description.m_Format);
    return EZ_FAILURE;
  }
  const bool bIsDepth = ezConversionUtilsVulkan::IsDepthFormat(m_imageFormat);

  m_stages = vk::PipelineStageFlagBits::eTransfer;
  m_access = vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite;
  m_preferredLayout = vk::ImageLayout::eGeneral;

  createInfo.initialLayout = vk::ImageLayout::eUndefined;
  createInfo.sharingMode = vk::SharingMode::eExclusive;
  createInfo.pQueueFamilyIndices = nullptr;
  createInfo.queueFamilyIndexCount = 0;
  createInfo.tiling = vk::ImageTiling::eOptimal;
  createInfo.usage |= vk::ImageUsageFlagBits::eTransferDst;
  if (m_Description.m_ResourceAccess.m_bReadBack)
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
    m_preferredLayout = bIsDepth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
  }
  // VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT
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
    m_stages |= m_pDevice->GetSupportedStages();
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
    if (m_sharedType == ezGALSharedTextureType::None || m_sharedType == ezGALSharedTextureType::Exported)
    {

      ezVulkanAllocationCreateInfo allocInfo;
      allocInfo.m_usage = ezVulkanMemoryUsage::Auto;
      if (m_bLinearCPU)
      {
        createInfo.tiling = vk::ImageTiling::eLinear;
        createInfo.usage |= vk::ImageUsageFlagBits::eTransferSrc;
        m_stages |= vk::PipelineStageFlagBits::eHost;
        m_access |= vk::AccessFlagBits::eHostRead;
        createInfo.flags = {}; // Clear all flags as we don't need them and they usually are not supported on NVidia in linear mode.

        allocInfo.m_flags = ezVulkanAllocationCreateFlags::HostAccessRandom;
      }

      if (m_sharedType == ezGALSharedTextureType::Exported)
      {
        allocInfo.m_bExportSharedAllocation = true;
      }

      vk::ImageFormatProperties props2;
      VK_ASSERT_DEBUG(m_pDevice->GetVulkanPhysicalDevice().getImageFormatProperties(createInfo.format, createInfo.imageType, createInfo.tiling, createInfo.usage, createInfo.flags, &props2));
      vk::ExternalMemoryImageCreateInfo extMemoryCreateInfo;
      if (m_sharedType == ezGALSharedTextureType::Exported)
      {
#if EZ_ENABLED(EZ_PLATFORM_LINUX)
        extMemoryCreateInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)
        extMemoryCreateInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
#endif
        createInfo.pNext = &extMemoryCreateInfo;
      }
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(ezMemoryAllocatorVulkan::CreateImage(createInfo, allocInfo, m_image, m_alloc, &m_allocInfo));

      if (m_sharedType == ezGALSharedTextureType::Exported)
      {
        if (!m_pDevice->GetExtensions().m_bTimelineSemaphore)
        {
          ezLog::Error("Can not create shared textures because timeline semaphores are not supported");
          return EZ_FAILURE;
        }

#if EZ_ENABLED(EZ_PLATFORM_LINUX)
        if (!m_pDevice->GetExtensions().m_bExternalMemoryFd)
        {
          ezLog::Error("Can not create shared textures because external memory fd is not supported");
          return EZ_FAILURE;
        }

        if (!m_pDevice->GetExtensions().m_bExternalSemaphoreFd)
        {
          ezLog::Error("Can not create shared textures because external semaphore fd is not supported");
          return EZ_FAILURE;
        }

        vk::MemoryGetFdInfoKHR getWin32HandleInfo{m_allocInfo.m_deviceMemory, vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd};
        int fd = -1;
        vk::Device device = m_pDevice->GetVulkanDevice();
        VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.getMemoryFdKHR(&getWin32HandleInfo, &fd, m_pDevice->GetDispatchContext()));
        m_sharedHandle.m_uiProcessId = ezProcess::GetCurrentProcessID();
        m_sharedHandle.a = (size_t)fd;
        m_sharedHandle.m_uiMemoryTypeIndex = m_allocInfo.m_memoryType;
        m_sharedHandle.m_uiSize = m_allocInfo.m_size;

        vk::ExportSemaphoreCreateInfoKHR exportInfo{vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd};
        vk::SemaphoreTypeCreateInfoKHR semTypeCreateInfo{vk::SemaphoreType::eTimeline, 0, &exportInfo};
        vk::SemaphoreCreateInfo semCreateInfo{{}, &semTypeCreateInfo};
        m_sharedSemaphore = device.createSemaphore(semCreateInfo);

        int semaphoreFd = -1;
        vk::SemaphoreGetFdInfoKHR getSemaphoreWin32Info{m_sharedSemaphore, vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd};
        VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.getSemaphoreFdKHR(&getSemaphoreWin32Info, &semaphoreFd, m_pDevice->GetDispatchContext()));
        m_sharedHandle.b = (size_t)semaphoreFd;
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)
        if (!m_pDevice->GetExtensions().m_bExternalMemoryWin32)
        {
          ezLog::Error("Can not create shared textures because external memory win32 is not supported");
          return EZ_FAILURE;
        }

        if (!m_pDevice->GetExtensions().m_bExternalSemaphoreWin32)
        {
          ezLog::Error("Can not create shared textures because external semaphore win32 is not supported");
          return EZ_FAILURE;
        }

        vk::Device device = m_pDevice->GetVulkanDevice();
        vk::MemoryGetWin32HandleInfoKHR getWin32HandleInfo{m_allocInfo.m_deviceMemory, vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32};
        HANDLE handle = 0;
        VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.getMemoryWin32HandleKHR(&getWin32HandleInfo, &handle, m_pDevice->GetDispatchContext()));
        m_sharedHandle.m_uiProcessId = ezProcess::GetCurrentProcessID();
        m_sharedHandle.a = (size_t)handle;
        m_sharedHandle.m_uiMemoryTypeIndex = m_allocInfo.m_memoryType;
        m_sharedHandle.m_uiSize = m_allocInfo.m_size;

        vk::ExportSemaphoreCreateInfoKHR exportInfo{vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32};
        vk::SemaphoreTypeCreateInfoKHR semTypeCreateInfo{vk::SemaphoreType::eTimeline, 0, &exportInfo};
        vk::SemaphoreCreateInfo semCreateInfo{{}, &semTypeCreateInfo};
        m_sharedSemaphore = device.createSemaphore(semCreateInfo);

        HANDLE semaphoreHandle = 0;
        vk::SemaphoreGetWin32HandleInfoKHR getSemaphoreWin32Info{m_sharedSemaphore, vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32};
        VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.getSemaphoreWin32HandleKHR(&getSemaphoreWin32Info, &semaphoreHandle, m_pDevice->GetDispatchContext()));
        m_sharedHandle.b = (size_t)semaphoreHandle;
#else
        EZ_ASSERT_NOT_IMPLEMENTED
#endif
      }
    }
    else if (m_sharedType == ezGALSharedTextureType::Imported)
    {
      if (!m_pDevice->GetExtensions().m_bTimelineSemaphore)
      {
        ezLog::Error("Can not open shared texture: timeline semaphores not supported");
        return EZ_FAILURE;
      }

#if EZ_ENABLED(EZ_PLATFORM_LINUX)
      if (m_sharedHandle.a == 0 || m_sharedHandle.b == 0)
      {
        ezLog::Error("Can not open shared texture: invalid handle given");
        return EZ_FAILURE;
      }


      if (!m_pDevice->GetExtensions().m_bExternalMemoryFd)
      {
        ezLog::Error("Can not open shared texture: external memory fd not supported");
        return EZ_FAILURE;
      }

      if (!m_pDevice->GetExtensions().m_bExternalSemaphoreFd)
      {
        ezLog::Error("Can not open shared texture: external semaphore fd not supported");
        return EZ_FAILURE;
      }

      bool bNeedToImportForeignProcessFileDescriptors = m_sharedHandle.m_uiProcessId != ezProcess::GetCurrentProcessID();
      if (bNeedToImportForeignProcessFileDescriptors)
      {
        int processFd = syscall(SYS_pidfd_open, m_sharedHandle.m_uiProcessId, 0);
        if (processFd == -1)
        {
          ezLog::Error("SYS_pidfd_open failed with errno: {}", ezArgErrno(errno));
          m_sharedHandle.a = 0;
          m_sharedHandle.b = 0;
          return EZ_FAILURE;
        }

        m_sharedHandle.a = syscall(SYS_pidfd_getfd, processFd, m_sharedHandle.a, 0);
        if (m_sharedHandle.a == -1)
        {
          ezLog::Error("SYS_pidfd_getfd for texture failed with errno: {}", ezArgErrno(errno));
          m_sharedHandle.a = 0;
          m_sharedHandle.b = 0;
          return EZ_FAILURE;
        }

        m_sharedHandle.b = syscall(SYS_pidfd_getfd, processFd, m_sharedHandle.b, 0);
        if (m_sharedHandle.b == -1)
        {
          ezLog::Error("SYS_pidfd_getfd for semaphore failed with errno: {}", ezArgErrno(errno));
          m_sharedHandle.b = 0;
          return EZ_FAILURE;
        }
      }

      vk::Device device = m_pDevice->GetVulkanDevice();

      // Import semaphore
      vk::SemaphoreTypeCreateInfoKHR semTypeCreateInfo{vk::SemaphoreType::eTimeline, 0};
      vk::SemaphoreCreateInfo semCreateInfo{{}, &semTypeCreateInfo};
      m_sharedSemaphore = device.createSemaphore(semCreateInfo);

      vk::ImportSemaphoreFdInfoKHR importSemaphoreInfo{m_sharedSemaphore, {}, vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd, static_cast<int>(m_sharedHandle.b)};
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.importSemaphoreFdKHR(&importSemaphoreInfo, m_pDevice->GetDispatchContext()));

      // Create Image
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.createImage(&createInfo, nullptr, &m_image));

      vk::ImageMemoryRequirementsInfo2 imageRequirementsInfo{m_image};
      vk::MemoryRequirements2 imageMemoryRequirements;
      device.getImageMemoryRequirements2(&imageRequirementsInfo, &imageMemoryRequirements);

      // Import memory
      // vk::MemoryFdPropertiesKHR importInfo;
      // VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.getMemoryFdPropertiesKHR(vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd, m_sharedHandle.a, &importInfo, m_pDevice->GetDispatchContext()));

      // EZ_ASSERT_DEBUG(ezMath::CountBits(importInfo.memoryTypeBits) == 1, "There should only be one valid memory type");
      // ezUInt32 memoryTypeIndex = ezMath::FirstBitHigh(importInfo.memoryTypeBits);
      // EZ_ASSERT_DEBUG(importInfo.memoryTypeBits == imageMemoryRequirements.memoryRequirements.memoryTypeBits, "Required and imported memory type bits do not match");
      EZ_ASSERT_DEBUG(imageMemoryRequirements.memoryRequirements.size == m_sharedHandle.m_uiSize, "");

      vk::ImportMemoryFdInfoKHR fdInfo{vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd, static_cast<int>(m_sharedHandle.a)};
      vk::MemoryAllocateInfo allocateInfo{imageMemoryRequirements.memoryRequirements.size, m_sharedHandle.m_uiMemoryTypeIndex, &fdInfo};

      m_allocInfo = {};
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.allocateMemory(&allocateInfo, nullptr, &m_allocInfo.m_deviceMemory));
      m_allocInfo.m_offset = 0;
      m_allocInfo.m_size = imageMemoryRequirements.memoryRequirements.size;
      m_allocInfo.m_memoryType = m_sharedHandle.m_uiMemoryTypeIndex;

      device.bindImageMemory(m_image, m_allocInfo.m_deviceMemory, 0);
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)
      if (m_sharedHandle.a == 0 || m_sharedHandle.b == 0)
      {
        ezLog::Error("Can not open shared texture: invalid handle given");
        return EZ_FAILURE;
      }


      if (!m_pDevice->GetExtensions().m_bExternalMemoryWin32)
      {
        ezLog::Error("Can not open shared texture: external memory win32 not supported");
        return EZ_FAILURE;
      }

      if (!m_pDevice->GetExtensions().m_bExternalSemaphoreWin32)
      {
        ezLog::Error("Can not open shared texture: external semaphore win32 not supported");
        return EZ_FAILURE;
      }

      bool bNeedToImportForeignProcessFileDescriptors = m_sharedHandle.m_uiProcessId != ezProcess::GetCurrentProcessID();
      if (bNeedToImportForeignProcessFileDescriptors)
      {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, m_sharedHandle.m_uiProcessId);
        if (hProcess == 0)
        {
          ezLog::Error("OpenProcess failed with error: {}", ezArgErrorCode(GetLastError()));
          m_sharedHandle.a = 0;
          m_sharedHandle.b = 0;
          return EZ_FAILURE;
        }

        HANDLE duplicateA = 0;
        BOOL res = DuplicateHandle(hProcess, reinterpret_cast<HANDLE>(m_sharedHandle.a), GetCurrentProcess(), &duplicateA, DUPLICATE_SAME_ACCESS, FALSE, 0);
        m_sharedHandle.a = reinterpret_cast<ezUInt64>(duplicateA);
        if (res == FALSE)
        {
          ezLog::Error("DuplicateHandle failed with error: {}", ezArgErrorCode(GetLastError()));
          m_sharedHandle.a = 0;
          m_sharedHandle.b = 0;
          return EZ_FAILURE;
        }

        HANDLE duplicateB = 0;
        res = DuplicateHandle(hProcess, reinterpret_cast<HANDLE>(m_sharedHandle.b), GetCurrentProcess(), &duplicateB, DUPLICATE_SAME_ACCESS, FALSE, 0);
        m_sharedHandle.b = reinterpret_cast<ezUInt64>(duplicateB);
        if (res == FALSE)
        {
          ezLog::Error("DuplicateHandle failed with error: {}", ezArgErrorCode(GetLastError()));
          m_sharedHandle.b = 0;
          return EZ_FAILURE;
        }
      }

      vk::Device device = m_pDevice->GetVulkanDevice();

      // Import semaphore
      vk::SemaphoreTypeCreateInfoKHR semTypeCreateInfo{vk::SemaphoreType::eTimeline, 0};
      vk::SemaphoreCreateInfo semCreateInfo{{}, &semTypeCreateInfo};
      m_sharedSemaphore = device.createSemaphore(semCreateInfo);

      vk::ImportSemaphoreWin32HandleInfoKHR importSemaphoreInfo{m_sharedSemaphore, {}, vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32, reinterpret_cast<HANDLE>(m_sharedHandle.b)};
      vk::Result res = device.importSemaphoreWin32HandleKHR(&importSemaphoreInfo, m_pDevice->GetDispatchContext());
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(res);

      // Create Image
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.createImage(&createInfo, nullptr, &m_image));

      vk::ImageMemoryRequirementsInfo2 imageRequirementsInfo{m_image};
      vk::MemoryRequirements2 imageMemoryRequirements;
      device.getImageMemoryRequirements2(&imageRequirementsInfo, &imageMemoryRequirements);

      // Import memory
      // vk::MemoryFdPropertiesKHR importInfo;
      // VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.getMemoryFdPropertiesKHR(vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd, m_sharedHandle.a, &importInfo, m_pDevice->GetDispatchContext()));

      // EZ_ASSERT_DEBUG(ezMath::CountBits(importInfo.memoryTypeBits) == 1, "There should only be one valid memory type");
      // ezUInt32 memoryTypeIndex = ezMath::FirstBitHigh(importInfo.memoryTypeBits);
      // EZ_ASSERT_DEBUG(importInfo.memoryTypeBits == imageMemoryRequirements.memoryRequirements.memoryTypeBits, "Required and imported memory type bits do not match");
      EZ_ASSERT_DEBUG(imageMemoryRequirements.memoryRequirements.size == m_sharedHandle.m_uiSize, "");

      vk::ImportMemoryWin32HandleInfoKHR fdInfo{vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32, reinterpret_cast<HANDLE>(m_sharedHandle.a)};
      vk::MemoryAllocateInfo allocateInfo{imageMemoryRequirements.memoryRequirements.size, m_sharedHandle.m_uiMemoryTypeIndex, &fdInfo};

      m_allocInfo = {};
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.allocateMemory(&allocateInfo, nullptr, &m_allocInfo.m_deviceMemory));
      m_allocInfo.m_offset = 0;
      m_allocInfo.m_size = imageMemoryRequirements.memoryRequirements.size;
      m_allocInfo.m_memoryType = m_sharedHandle.m_uiMemoryTypeIndex;

      device.bindImageMemory(m_image, m_allocInfo.m_deviceMemory, 0);
#else
      EZ_ASSERT_NOT_IMPLEMENTED
#endif
    }
  }
  else
  {
    m_image = static_cast<VkImage>(m_pExisitingNativeObject);
  }
  m_pDevice->GetInitContext().InitTexture(this, createInfo, pInitialData);

  if (m_Description.m_ResourceAccess.m_bReadBack)
  {
    return CreateStagingBuffer(createInfo);
  }

  return EZ_SUCCESS;
}

ezGALTextureVulkan::StagingMode ezGALTextureVulkan::ComputeStagingMode(const vk::ImageCreateInfo& createInfo) const
{
  if (!m_Description.m_ResourceAccess.m_bReadBack)
    return StagingMode::None;

  // We want the staging texture to always have the intended format and not the override format given by the parent texture.
  vk::Format stagingFormat = m_pDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_readback;

  EZ_ASSERT_DEV(!ezConversionUtilsVulkan::IsStencilFormat(m_imageFormat), "Stencil read-back not implemented.");
  EZ_ASSERT_DEV(!ezConversionUtilsVulkan::IsDepthFormat(stagingFormat), "Depth read-back should use a color format for CPU staging.");

  const vk::FormatProperties srcFormatProps = m_pDevice->GetVulkanPhysicalDevice().getFormatProperties(m_imageFormat);
  const vk::FormatProperties dstFormatProps = m_pDevice->GetVulkanPhysicalDevice().getFormatProperties(stagingFormat);

  const bool bFormatsEqual = m_imageFormat == stagingFormat && createInfo.samples == vk::SampleCountFlagBits::e1;
  const bool bSupportsCopy = bFormatsEqual && (srcFormatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eTransferSrc) && (dstFormatProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eTransferDst);
  if (bFormatsEqual)
  {
    EZ_ASSERT_DEV(srcFormatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eTransferSrc, "Source format can't be read, readback impossible.");
    return StagingMode::Buffer;
  }
  else
  {
    vk::ImageFormatProperties props;
    vk::Result res = m_pDevice->GetVulkanPhysicalDevice().getImageFormatProperties(stagingFormat, createInfo.imageType, vk::ImageTiling::eLinear, vk::ImageUsageFlagBits::eColorAttachment, {}, &props);

    // #TODO_VULKAN Note that on Nvidia driver 531.68 the above call succeeds even though linear rendering is not supported by the driver. Thus, we need to check explicitly here that vk::FormatFeatureFlagBits::eColorAttachment is supported again via the vk::FormatProperties.
    const bool bCanUseDirectTexture = (dstFormatProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment) && (res == vk::Result::eSuccess) && createInfo.arrayLayers <= props.maxArrayLayers && createInfo.mipLevels <= props.maxMipLevels && createInfo.extent.depth <= props.maxExtent.depth && createInfo.extent.width <= props.maxExtent.width && createInfo.extent.height <= props.maxExtent.height && (createInfo.samples & props.sampleCounts);
    return bCanUseDirectTexture ? StagingMode::Texture : StagingMode::TextureAndBuffer;
  }
}

ezUInt32 ezGALTextureVulkan::ComputeSubResourceOffsets(ezDynamicArray<SubResourceOffset>& subResourceSizes) const
{
  const ezUInt32 alignment = (ezUInt32)ezGALBufferVulkan::GetAlignment(m_pDevice, vk::BufferUsageFlagBits::eTransferDst);
  const vk::Format stagingFormat = m_pDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_readback;
  const ezUInt8 uiBlockSize = vk::blockSize(stagingFormat);
  const auto blockExtent = vk::blockExtent(stagingFormat);
  const ezUInt32 arrayLayers = (m_Description.m_Type == ezGALTextureType::TextureCube ? (m_Description.m_uiArraySize * 6) : m_Description.m_uiArraySize);
  const ezUInt32 mipLevels = m_Description.m_uiMipLevelCount;

  subResourceSizes.Reserve(arrayLayers * mipLevels);
  ezUInt32 uiOffset = 0;
  for (ezUInt32 uiLayer = 0; uiLayer < arrayLayers; uiLayer++)
  {
    for (ezUInt32 uiMipLevel = 0; uiMipLevel < mipLevels; uiMipLevel++)
    {
      const ezUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * mipLevels;
      EZ_ASSERT_DEBUG(subResourceSizes.GetCount() == uiSubresourceIndex, "");

      const vk::Extent3D imageExtent = GetMipLevelSize(uiMipLevel);
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

ezResult ezGALTextureVulkan::CreateStagingBuffer(const vk::ImageCreateInfo& createInfo)
{
  m_stagingMode = ezGALTextureVulkan::ComputeStagingMode(createInfo);
  if (m_stagingMode == StagingMode::Texture || m_stagingMode == StagingMode::TextureAndBuffer)
  {
    ezGALTextureCreationDescription stagingDesc = m_Description;
    stagingDesc.m_SampleCount = ezGALMSAASampleCount::None;
    stagingDesc.m_bAllowShaderResourceView = false;
    stagingDesc.m_bAllowUAV = false;
    stagingDesc.m_bCreateRenderTarget = true;
    stagingDesc.m_bAllowDynamicMipGeneration = false;
    stagingDesc.m_ResourceAccess.m_bReadBack = false;
    stagingDesc.m_ResourceAccess.m_bImmutable = false;
    stagingDesc.m_pExisitingNativeObject = nullptr;

    const bool bLinearCPU = m_stagingMode == StagingMode::Texture;
    const vk::Format stagingFormat = m_pDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_readback;

    m_hStagingTexture = m_pDevice->CreateTextureInternal(stagingDesc, {}, stagingFormat, bLinearCPU);
    if (m_hStagingTexture.IsInvalidated())
    {
      ezLog::Error("Failed to create staging texture for read-back");
      return EZ_FAILURE;
    }
  }
  if (m_stagingMode == StagingMode::Buffer || m_stagingMode == StagingMode::TextureAndBuffer)
  {
    ezGALBufferCreationDescription stagingBuffer;
    stagingBuffer.m_BufferType = ezGALBufferType::Generic;

    ezHybridArray<SubResourceOffset, 8> subResourceSizes;
    stagingBuffer.m_uiTotalSize = ComputeSubResourceOffsets(subResourceSizes);
    stagingBuffer.m_uiStructSize = 1;
    stagingBuffer.m_bAllowRawViews = true;
    stagingBuffer.m_ResourceAccess.m_bImmutable = false;

    m_hStagingBuffer = m_pDevice->CreateBufferInternal(stagingBuffer, {}, true);
    if (m_hStagingBuffer.IsInvalidated())
    {
      ezLog::Error("Failed to create staging buffer for read-back");
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}
ezResult ezGALTextureVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  if (m_sharedType == ezGALSharedTextureType::Imported)
  {
    pVulkanDevice->DeleteLater(m_image, m_allocInfo.m_deviceMemory);
    pVulkanDevice->DeleteLater(m_sharedSemaphore);
  }
  else
  {
    if (m_image && !m_pExisitingNativeObject)
    {
      pVulkanDevice->DeleteLater(m_image, m_alloc);
    }
  }
  m_image = nullptr;

  if (!m_hStagingTexture.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hStagingTexture);
    m_hStagingTexture.Invalidate();
  }
  if (!m_hStagingBuffer.IsInvalidated())
  {
    pDevice->DestroyBuffer(m_hStagingBuffer);
    m_hStagingBuffer.Invalidate();
  }

#if EZ_ENABLED(EZ_PLATFORM_LINUX)
  if (m_sharedHandle.a != 0)
  {
    pVulkanDevice->DeleteLaterImpl({vk::ObjectType::eUnknown, {ezGALDeviceVulkan::PendingDeletionFlags::IsFileDescriptor}, (void*)static_cast<size_t>(m_sharedHandle.a), nullptr});
    m_sharedHandle.a = 0;
  }
  if (m_sharedHandle.b != 0)
  {
    pVulkanDevice->DeleteLaterImpl({vk::ObjectType::eUnknown, {ezGALDeviceVulkan::PendingDeletionFlags::IsFileDescriptor}, (void*)static_cast<size_t>(m_sharedHandle.b), nullptr});
    m_sharedHandle.b = 0;
  }
#endif
  return EZ_SUCCESS;
}

void ezGALTextureVulkan::SetDebugNamePlatform(const char* szName) const
{
  m_pDevice->SetDebugName(szName, m_image, m_alloc);
  if (!m_hStagingTexture.IsInvalidated())
  {
    auto pStagingTexture = static_cast<const ezGALTextureVulkan*>(m_pDevice->GetTexture(m_hStagingTexture));
    pStagingTexture->SetDebugName(szName);
  }
  if (!m_hStagingBuffer.IsInvalidated())
  {
    auto pStagingBuffer = static_cast<const ezGALBufferVulkan*>(m_pDevice->GetBuffer(m_hStagingBuffer));
    pStagingBuffer->SetDebugName(szName);
  }
}

ezGALPlatformSharedHandle ezGALTextureVulkan::GetSharedHandle() const
{
  return m_sharedHandle;
}

void ezGALTextureVulkan::WaitSemaphore(ezUInt64 uiValue) const
{
  m_pDevice->AddWaitSemaphore({m_sharedSemaphore, vk::SemaphoreType::eTimeline, uiValue});
}

void ezGALTextureVulkan::SignalSemaphore(ezUInt64 uiValue) const
{
  m_pDevice->AddSignalSemaphore({m_sharedSemaphore, vk::SemaphoreType::eTimeline, uiValue});
}

EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_TextureVulkan);
