#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/System/Process.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Resources/SharedTextureVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <errno.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#endif

ezGALSharedTextureVulkan::ezGALSharedTextureVulkan(const ezGALTextureCreationDescription& Description, ezEnum<ezGALSharedTextureType> sharedType, ezGALPlatformSharedHandle hSharedHandle)
  : ezGALTextureVulkan(Description)
  , m_SharedType(sharedType)
  , m_hSharedHandle(hSharedHandle)
{
}

ezGALSharedTextureVulkan::~ezGALSharedTextureVulkan()
{
}

ezResult ezGALSharedTextureVulkan::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  m_pDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  vk::ImageFormatListCreateInfo imageFormats;
  vk::ImageCreateInfo createInfo = {};

  m_imageFormat = ComputeImageFormat(m_pDevice, m_Description.m_Format, createInfo, imageFormats);

  ComputeCreateInfo(m_pDevice, m_Description, createInfo, m_stages, m_access, m_preferredLayout);

  if (m_Description.m_pExisitingNativeObject == nullptr)
  {
    vk::ExternalMemoryImageCreateInfo extMemoryCreateInfo;
    if (m_SharedType == ezGALSharedTextureType::Exported || m_SharedType == ezGALSharedTextureType::Imported)
    {
#if EZ_ENABLED(EZ_PLATFORM_LINUX)
      extMemoryCreateInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)
      extMemoryCreateInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
#endif
      extMemoryCreateInfo.pNext = createInfo.pNext;
      createInfo.pNext = &extMemoryCreateInfo;

      m_preferredLayout = vk::ImageLayout::eGeneral;
    }

    if (m_SharedType == ezGALSharedTextureType::None || m_SharedType == ezGALSharedTextureType::Exported)
    {

      ezVulkanAllocationCreateInfo allocInfo;
      ComputeAllocInfo(allocInfo);

      if (m_SharedType == ezGALSharedTextureType::Exported)
      {
        allocInfo.m_bExportSharedAllocation = true;
      }

      vk::ImageFormatProperties props2;
      VK_ASSERT_DEBUG(m_pDevice->GetVulkanPhysicalDevice().getImageFormatProperties(createInfo.format, createInfo.imageType, createInfo.tiling, createInfo.usage, createInfo.flags, &props2));
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(ezMemoryAllocatorVulkan::CreateImage(createInfo, allocInfo, m_image, m_alloc, &m_allocInfo));

      if (m_SharedType == ezGALSharedTextureType::Exported)
      {
        if (!m_pDevice->GetExtensions().m_bTimelineSemaphore)
        {
          ezLog::Error("Can not create shared textures because timeline semaphores are not supported");
          return EZ_FAILURE;
        }

#if EZ_ENABLED(EZ_PLATFORM_LINUX) && defined(SYS_pidfd_getfd)
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
        m_hSharedHandle.m_uiProcessId = ezProcess::GetCurrentProcessID();
        m_hSharedHandle.m_hSharedTexture = (size_t)fd;
        m_hSharedHandle.m_uiMemoryTypeIndex = m_allocInfo.m_memoryType;
        m_hSharedHandle.m_uiSize = m_allocInfo.m_size;

        vk::ExportSemaphoreCreateInfoKHR exportInfo{vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd};
        vk::SemaphoreTypeCreateInfoKHR semTypeCreateInfo{vk::SemaphoreType::eTimeline, 0, &exportInfo};
        vk::SemaphoreCreateInfo semCreateInfo{{}, &semTypeCreateInfo};
        m_SharedSemaphore = device.createSemaphore(semCreateInfo);

        int semaphoreFd = -1;
        vk::SemaphoreGetFdInfoKHR getSemaphoreWin32Info{m_SharedSemaphore, vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd};
        VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.getSemaphoreFdKHR(&getSemaphoreWin32Info, &semaphoreFd, m_pDevice->GetDispatchContext()));
        m_hSharedHandle.m_hSemaphore = (size_t)semaphoreFd;
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
        m_hSharedHandle.m_uiProcessId = ezProcess::GetCurrentProcessID();
        m_hSharedHandle.m_hSharedTexture = (size_t)handle;
        m_hSharedHandle.m_uiMemoryTypeIndex = m_allocInfo.m_memoryType;
        m_hSharedHandle.m_uiSize = m_allocInfo.m_size;

        vk::ExportSemaphoreWin32HandleInfoKHR exportInfoWin32;
        exportInfoWin32.dwAccess = GENERIC_ALL;
        vk::ExportSemaphoreCreateInfoKHR exportInfo{vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32, &exportInfoWin32};
        vk::SemaphoreTypeCreateInfoKHR semTypeCreateInfo{vk::SemaphoreType::eTimeline, 0, &exportInfo};
        vk::SemaphoreCreateInfo semCreateInfo{{}, &semTypeCreateInfo};
        VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.createSemaphore(&semCreateInfo, nullptr, &m_SharedSemaphore));

        HANDLE semaphoreHandle = 0;
        vk::SemaphoreGetWin32HandleInfoKHR getSemaphoreWin32Info{m_SharedSemaphore, vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32};
        VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.getSemaphoreWin32HandleKHR(&getSemaphoreWin32Info, &semaphoreHandle, m_pDevice->GetDispatchContext()));
        m_hSharedHandle.m_hSemaphore = (size_t)semaphoreHandle;
#else
        EZ_ASSERT_NOT_IMPLEMENTED
#endif
      }
    }
    else if (m_SharedType == ezGALSharedTextureType::Imported)
    {
      if (!m_pDevice->GetExtensions().m_bTimelineSemaphore)
      {
        ezLog::Error("Can not open shared texture: timeline semaphores not supported");
        return EZ_FAILURE;
      }

#if EZ_ENABLED(EZ_PLATFORM_LINUX) && defined(SYS_pidfd_getfd)
      if (m_hSharedHandle.m_hSharedTexture == 0 || m_hSharedHandle.m_hSemaphore == 0)
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

      bool bNeedToImportForeignProcessFileDescriptors = m_hSharedHandle.m_uiProcessId != ezProcess::GetCurrentProcessID();
      if (bNeedToImportForeignProcessFileDescriptors)
      {
        int processFd = syscall(SYS_pidfd_open, m_hSharedHandle.m_uiProcessId, 0);
        if (processFd == -1)
        {
          ezLog::Error("SYS_pidfd_open failed with errno: {}", ezArgErrno(errno));
          m_hSharedHandle.m_hSharedTexture = 0;
          m_hSharedHandle.m_hSemaphore = 0;
          return EZ_FAILURE;
        }
        EZ_SCOPE_EXIT(close(processFd));

        m_hSharedHandle.m_hSharedTexture = syscall(SYS_pidfd_getfd, processFd, m_hSharedHandle.m_hSharedTexture, 0);
        if (m_hSharedHandle.m_hSharedTexture == -1)
        {
          ezLog::Error("SYS_pidfd_getfd for texture failed with errno: {}", ezArgErrno(errno));
          m_hSharedHandle.m_hSharedTexture = 0;
          m_hSharedHandle.m_hSemaphore = 0;
          return EZ_FAILURE;
        }

        m_hSharedHandle.m_hSemaphore = syscall(SYS_pidfd_getfd, processFd, m_hSharedHandle.m_hSemaphore, 0);
        if (m_hSharedHandle.m_hSemaphore == -1)
        {
          ezLog::Error("SYS_pidfd_getfd for semaphore failed with errno: {}", ezArgErrno(errno));
          m_hSharedHandle.m_hSemaphore = 0;
          return EZ_FAILURE;
        }
      }

      vk::Device device = m_pDevice->GetVulkanDevice();

      // Import semaphore
      vk::SemaphoreTypeCreateInfoKHR semTypeCreateInfo{vk::SemaphoreType::eTimeline, 0};
      vk::SemaphoreCreateInfo semCreateInfo{{}, &semTypeCreateInfo};
      m_SharedSemaphore = device.createSemaphore(semCreateInfo);

      vk::ImportSemaphoreFdInfoKHR importSemaphoreInfo{m_SharedSemaphore, {}, vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd, static_cast<int>(m_hSharedHandle.m_hSemaphore)};
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.importSemaphoreFdKHR(&importSemaphoreInfo, m_pDevice->GetDispatchContext()));
      // Spec: "Importing a semaphore payload from a file descriptor transfers ownership of the file descriptor from the application to the Vulkan implementation. The application must not perform any operations on the file descriptor after a successful import."
      m_hSharedHandle.m_hSemaphore = 0;

      // Create Image
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.createImage(&createInfo, nullptr, &m_image));

      vk::ImageMemoryRequirementsInfo2 imageRequirementsInfo{m_image};
      vk::MemoryRequirements2 imageMemoryRequirements;
      device.getImageMemoryRequirements2(&imageRequirementsInfo, &imageMemoryRequirements);

      // Import memory
      EZ_ASSERT_DEBUG(imageMemoryRequirements.memoryRequirements.size == m_hSharedHandle.m_uiSize, "");

      vk::ImportMemoryFdInfoKHR fdInfo{vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd, static_cast<int>(m_hSharedHandle.m_hSharedTexture)};
      vk::MemoryAllocateInfo allocateInfo{imageMemoryRequirements.memoryRequirements.size, m_hSharedHandle.m_uiMemoryTypeIndex, &fdInfo};

      m_allocInfo = {};
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.allocateMemory(&allocateInfo, nullptr, &m_allocInfo.m_deviceMemory));
      m_allocInfo.m_offset = 0;
      m_allocInfo.m_size = imageMemoryRequirements.memoryRequirements.size;
      m_allocInfo.m_memoryType = m_hSharedHandle.m_uiMemoryTypeIndex;
      // Spec: "Importing memory from a file descriptor transfers ownership of the file descriptor from the application to the Vulkan implementation. The application must not perform any operations on the file descriptor after a successful import. "
      m_hSharedHandle.m_hSharedTexture = 0;

      device.bindImageMemory(m_image, m_allocInfo.m_deviceMemory, 0);

#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)
      if (m_hSharedHandle.m_hSharedTexture == 0 || m_hSharedHandle.m_hSemaphore == 0)
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

      bool bNeedToImportForeignProcessFileDescriptors = m_hSharedHandle.m_uiProcessId != ezProcess::GetCurrentProcessID();
      if (bNeedToImportForeignProcessFileDescriptors)
      {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, m_hSharedHandle.m_uiProcessId);
        if (hProcess == 0)
        {
          ezLog::Error("OpenProcess failed with error: {}", ezArgErrorCode(GetLastError()));
          m_hSharedHandle.m_hSharedTexture = 0;
          m_hSharedHandle.m_hSemaphore = 0;
          return EZ_FAILURE;
        }

        HANDLE duplicateA = 0;
        BOOL res = DuplicateHandle(hProcess, reinterpret_cast<HANDLE>(m_hSharedHandle.m_hSharedTexture), GetCurrentProcess(), &duplicateA, 0, FALSE, DUPLICATE_SAME_ACCESS);
        m_hSharedHandle.m_hSharedTexture = reinterpret_cast<ezUInt64>(duplicateA);
        if (res == FALSE)
        {
          ezLog::Error("DuplicateHandle failed with error: {}", ezArgErrorCode(GetLastError()));
          m_hSharedHandle.m_hSharedTexture = 0;
          m_hSharedHandle.m_hSemaphore = 0;
          return EZ_FAILURE;
        }

        HANDLE duplicateB = 0;
        res = DuplicateHandle(hProcess, reinterpret_cast<HANDLE>(m_hSharedHandle.m_hSemaphore), GetCurrentProcess(), &duplicateB, 0, FALSE, DUPLICATE_SAME_ACCESS);
        m_hSharedHandle.m_hSemaphore = reinterpret_cast<ezUInt64>(duplicateB);
        if (res == FALSE)
        {
          ezLog::Error("DuplicateHandle failed with error: {}", ezArgErrorCode(GetLastError()));
          m_hSharedHandle.m_hSemaphore = 0;
          return EZ_FAILURE;
        }
      }

      vk::Device device = m_pDevice->GetVulkanDevice();

      // Import semaphore
      vk::SemaphoreTypeCreateInfoKHR semTypeCreateInfo{vk::SemaphoreType::eTimeline, 0};
      vk::SemaphoreCreateInfo semCreateInfo{{}, &semTypeCreateInfo};
      m_SharedSemaphore = device.createSemaphore(semCreateInfo);

      vk::ImportSemaphoreWin32HandleInfoKHR importSemaphoreInfo{m_SharedSemaphore, {}, vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32, reinterpret_cast<HANDLE>(m_hSharedHandle.m_hSemaphore)};
      vk::Result res = device.importSemaphoreWin32HandleKHR(&importSemaphoreInfo, m_pDevice->GetDispatchContext());
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(res);

      // Create Image
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.createImage(&createInfo, nullptr, &m_image));

      vk::ImageMemoryRequirementsInfo2 imageRequirementsInfo{m_image};
      vk::MemoryRequirements2 imageMemoryRequirements;
      device.getImageMemoryRequirements2(&imageRequirementsInfo, &imageMemoryRequirements);

      // Import memory
      EZ_ASSERT_DEBUG(imageMemoryRequirements.memoryRequirements.size == m_hSharedHandle.m_uiSize, "");

      vk::ImportMemoryWin32HandleInfoKHR fdInfo{vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32, reinterpret_cast<HANDLE>(m_hSharedHandle.m_hSharedTexture)};
      vk::MemoryAllocateInfo allocateInfo{imageMemoryRequirements.memoryRequirements.size, m_hSharedHandle.m_uiMemoryTypeIndex, &fdInfo};

      // vk::MemoryWin32HandlePropertiesKHR handleProperties;
      //  device.getMemoryWin32HandlePropertiesKHR(vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32, reinterpret_cast<HANDLE>(m_hSharedHandle.m_hSharedTexture), m_pDevice->GetDispatchContext());

      m_allocInfo = {};
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(device.allocateMemory(&allocateInfo, nullptr, &m_allocInfo.m_deviceMemory));
      m_allocInfo.m_offset = 0;
      m_allocInfo.m_size = imageMemoryRequirements.memoryRequirements.size;
      m_allocInfo.m_memoryType = m_hSharedHandle.m_uiMemoryTypeIndex;

      device.bindImageMemory(m_image, m_allocInfo.m_deviceMemory, 0);
#else
      EZ_ASSERT_NOT_IMPLEMENTED
#endif
    }
  }
  else
  {
    m_image = static_cast<VkImage>(m_Description.m_pExisitingNativeObject);
  }
  m_pDevice->GetInitContext().InitTexture(this, createInfo, pInitialData);

  return EZ_SUCCESS;
}


ezResult ezGALSharedTextureVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  if (m_SharedType == ezGALSharedTextureType::Imported)
  {
    pVulkanDevice->DeleteLater(m_image, m_allocInfo.m_deviceMemory);
    pVulkanDevice->DeleteLater(m_SharedSemaphore);
  }
  else if (m_SharedType == ezGALSharedTextureType::Exported)
  {
    pVulkanDevice->DeleteLater(m_image, m_alloc);
    pVulkanDevice->DeleteLater(m_SharedSemaphore);
  }

  auto res = SUPER::DeInitPlatform(pDevice);

#if EZ_ENABLED(EZ_PLATFORM_LINUX) && defined(SYS_pidfd_getfd)
  // These are only needed if init failed before importing the semaphore, which would have transferred ownership.
  if (m_hSharedHandle.m_hSharedTexture != 0)
  {
    pVulkanDevice->DeleteLaterImpl({vk::ObjectType::eUnknown, {ezGALDeviceVulkan::PendingDeletionFlags::IsFileDescriptor}, (void*)static_cast<size_t>(m_hSharedHandle.m_hSharedTexture), nullptr});
    m_hSharedHandle.m_hSharedTexture = 0;
  }
  if (m_hSharedHandle.m_hSemaphore != 0)
  {

    pVulkanDevice->DeleteLaterImpl({vk::ObjectType::eUnknown, {ezGALDeviceVulkan::PendingDeletionFlags::IsFileDescriptor}, (void*)static_cast<size_t>(m_hSharedHandle.m_hSemaphore), nullptr});
    m_hSharedHandle.m_hSemaphore = 0;
  }
#endif
  return res;
}

ezGALPlatformSharedHandle ezGALSharedTextureVulkan::GetSharedHandle() const
{
  return m_hSharedHandle;
}

void ezGALSharedTextureVulkan::WaitSemaphoreGPU(ezUInt64 uiValue) const
{
  m_pDevice->AddWaitSemaphore(ezGALDeviceVulkan::SemaphoreInfo::MakeWaitSemaphore(m_SharedSemaphore, vk::PipelineStageFlagBits::eAllCommands, vk::SemaphoreType::eTimeline, uiValue));
}

void ezGALSharedTextureVulkan::SignalSemaphoreGPU(ezUInt64 uiValue) const
{
  m_pDevice->AddSignalSemaphore(ezGALDeviceVulkan::SemaphoreInfo::MakeSignalSemaphore(m_SharedSemaphore, vk::SemaphoreType::eTimeline, uiValue));
  // TODO, transition texture into GENERAL layout
  m_pDevice->GetCurrentPipelineBarrier().EnsureImageLayout(this, GetPreferredLayout(), GetUsedByPipelineStage(), GetAccessMask());
}
