#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/DispatchContext.h>

void ezVulkanDispatchContext::InitInstance(vk::Instance instance, const void* pExtensions)
{
  const ezGALDeviceVulkan::Extensions& extensions = *static_cast<const ezGALDeviceVulkan::Extensions*>(pExtensions);
  // VK_EXT_debug_utils (instance functions)
  if (extensions.m_bDebugUtils)
  {
    vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)instance.getProcAddr("vkCreateDebugUtilsMessengerEXT");
    vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT");
  }
}

void ezVulkanDispatchContext::InitDevice(vk::Device device, const void* pExtensions)
{
  const ezGALDeviceVulkan::Extensions& extensions = *static_cast<const ezGALDeviceVulkan::Extensions*>(pExtensions);
  // VK_EXT_debug_utils
  if (extensions.m_bDebugUtils)
  {
    vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)device.getProcAddr("vkSetDebugUtilsObjectNameEXT");
    vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)device.getProcAddr("vkCmdBeginDebugUtilsLabelEXT");
    vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)device.getProcAddr("vkCmdEndDebugUtilsLabelEXT");
    vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)device.getProcAddr("vkCmdInsertDebugUtilsLabelEXT");
  }

  // VK_KHR_timeline_semaphore
  if (extensions.m_bTimelineSemaphore)
  {
    vkSignalSemaphoreKHR = (PFN_vkSignalSemaphore)device.getProcAddr("vkSignalSemaphoreKHR");
    vkSignalSemaphore = vkSignalSemaphoreKHR;                   // Alias
    vkWaitSemaphoresKHR = (PFN_vkWaitSemaphores)device.getProcAddr("vkWaitSemaphoresKHR");
    vkWaitSemaphores = vkWaitSemaphoresKHR;                     // Alias
    vkGetSemaphoreCounterValueKHR = (PFN_vkGetSemaphoreCounterValue)device.getProcAddr("vkGetSemaphoreCounterValueKHR");
    vkGetSemaphoreCounterValue = vkGetSemaphoreCounterValueKHR; // Alias
  }

  // VK_KHR_synchronization2
  if (extensions.m_bSynchronization2)
  {
    vkCmdPipelineBarrier2KHR = (PFN_vkCmdPipelineBarrier2KHR)device.getProcAddr("vkCmdPipelineBarrier2KHR");
    vkCmdPipelineBarrier2 = vkCmdPipelineBarrier2KHR; // Alias
  }

#if EZ_ENABLED(EZ_PLATFORM_LINUX)
  if (extensions.m_bExternalMemoryFd)
  {
    vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)device.getProcAddr("vkGetMemoryFdKHR");
    vkGetMemoryFdPropertiesKHR = (PFN_vkGetMemoryFdPropertiesKHR)device.getProcAddr("vkGetMemoryFdPropertiesKHR");
  }

  if (extensions.m_bExternalSemaphoreFd)
  {
    vkGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)device.getProcAddr("vkGetSemaphoreFdKHR");
    vkImportSemaphoreFdKHR = (PFN_vkImportSemaphoreFdKHR)device.getProcAddr("vkImportSemaphoreFdKHR");
  }
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (extensions.m_bExternalMemoryWin32)
  {
    vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)device.getProcAddr("vkGetMemoryWin32HandleKHR");
    vkGetMemoryWin32HandlePropertiesKHR = (PFN_vkGetMemoryWin32HandlePropertiesKHR)device.getProcAddr("vkGetMemoryWin32HandlePropertiesKHR");
  }

  if (extensions.m_bExternalSemaphoreWin32)
  {
    vkGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)device.getProcAddr("vkGetSemaphoreWin32HandleKHR");
    vkImportSemaphoreWin32HandleKHR = (PFN_vkImportSemaphoreWin32HandleKHR)device.getProcAddr("vkImportSemaphoreWin32HandleKHR");
  }
#endif
}
