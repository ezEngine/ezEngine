#pragma once

class ezGALDeviceVulkan;

// A vulkan hpp compatible dispatch context.
class ezVulkanDispatchContext
{
public:
  void InitInstance(vk::Instance instance, const void* pExtensions);
  void InitDevice(vk::Device device, const void* pExtensions);

  ezUInt32 getVkHeaderVersion() const { return VK_HEADER_VERSION; }

  // VK_EXT_debug_utils (instance functions)
  PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = nullptr;
  PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;

  // VK_EXT_debug_utils (device functions)
  PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = nullptr;
  PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = nullptr;
  PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = nullptr;
  PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT = nullptr;

  // VK_KHR_timeline_semaphore
  PFN_vkSignalSemaphore vkSignalSemaphoreKHR = nullptr;
  PFN_vkSignalSemaphore vkSignalSemaphore = nullptr;                   // Alias for Vulkan-Hpp compatibility
  PFN_vkWaitSemaphores vkWaitSemaphoresKHR = nullptr;
  PFN_vkWaitSemaphores vkWaitSemaphores = nullptr;                     // Alias for Vulkan-Hpp compatibility
  PFN_vkGetSemaphoreCounterValue vkGetSemaphoreCounterValueKHR = nullptr;
  PFN_vkGetSemaphoreCounterValue vkGetSemaphoreCounterValue = nullptr; // Alias for Vulkan-Hpp compatibility

  // VK_KHR_synchronization2
  PFN_vkCmdPipelineBarrier2KHR vkCmdPipelineBarrier2KHR = nullptr;
  PFN_vkCmdPipelineBarrier2KHR vkCmdPipelineBarrier2 = nullptr; // Alias for Vulkan-Hpp compatibility

#if EZ_ENABLED(EZ_PLATFORM_LINUX)
  // VK_KHR_external_memory_fd
  PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR = nullptr;
  PFN_vkGetMemoryFdPropertiesKHR vkGetMemoryFdPropertiesKHR = nullptr;

  // VK_KHR_external_semaphore_fd
  PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR = nullptr;
  PFN_vkImportSemaphoreFdKHR vkImportSemaphoreFdKHR = nullptr;
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  // VK_KHR_external_memory_win32
  PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR = nullptr;
  PFN_vkGetMemoryWin32HandlePropertiesKHR vkGetMemoryWin32HandlePropertiesKHR = nullptr;

  // VK_KHR_external_semaphore_win32
  PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR = nullptr;
  PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR = nullptr;
#endif
};
