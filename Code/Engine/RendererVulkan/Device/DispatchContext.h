#pragma once

#include <vulkan/vulkan.h>

class ezGALDeviceVulkan;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
#  define EZ_DISPATCH_CONTEXT_MEMBER_NAME(Name) m_p##Name
#else
#  define EZ_DISPATCH_CONTEXT_MEMBER_NAME(Name) Name
#endif

// A vulkan hpp compatible dispatch context.
class ezVulkanDispatchContext
{
public:
  void Init(ezGALDeviceVulkan& device);

  ezUInt32 getVkHeaderVersion() const { return VK_HEADER_VERSION; }

  // VK_KHR_external_memory_fd
  PFN_vkGetMemoryFdKHR EZ_DISPATCH_CONTEXT_MEMBER_NAME(vkGetMemoryFdKHR) = nullptr;
  PFN_vkGetMemoryFdPropertiesKHR EZ_DISPATCH_CONTEXT_MEMBER_NAME(vkGetMemoryFdPropertiesKHR) = nullptr;

  // VK_KHR_external_semaphore_fd
  PFN_vkGetSemaphoreFdKHR EZ_DISPATCH_CONTEXT_MEMBER_NAME(vkGetSemaphoreFdKHR) = nullptr;
  PFN_vkImportSemaphoreFdKHR EZ_DISPATCH_CONTEXT_MEMBER_NAME(vkImportSemaphoreFdKHR) = nullptr;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // VK_KHR_external_memory_fd
  VkResult vkGetMemoryFdKHR(VkDevice device, const VkMemoryGetFdInfoKHR* pGetFdInfo, int* pFd) const;
  VkResult vkGetMemoryFdPropertiesKHR(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, int fd, VkMemoryFdPropertiesKHR* pMemoryFdProperties) const;

  // VK_KHR_external_semaphore_fd
  VkResult vkGetSemaphoreFdKHR(VkDevice device, const VkSemaphoreGetFdInfoKHR* pGetFdInfo, int* pFd) const;
  VkResult vkImportSemaphoreFdKHR(VkDevice device, const VkImportSemaphoreFdInfoKHR* pImportSemaphoreFdInfo) const;
#endif

private:
  ezGALDeviceVulkan* m_pDevice = nullptr;
};