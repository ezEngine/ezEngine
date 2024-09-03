#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/DispatchContext.h>

void ezVulkanDispatchContext::Init(ezGALDeviceVulkan& device)
{
  m_pDevice = &device;

  VkDevice nativeDevice = (VkDevice)device.GetVulkanDevice();
  const ezGALDeviceVulkan::Extensions& extensions = device.GetExtensions();
#if EZ_ENABLED(EZ_PLATFORM_LINUX)
  if (extensions.m_bExternalMemoryFd)
  {
    EZ_DISPATCH_CONTEXT_MEMBER_NAME(vkGetMemoryFdKHR) = (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(nativeDevice, "vkGetMemoryFdKHR");
    EZ_DISPATCH_CONTEXT_MEMBER_NAME(vkGetMemoryFdPropertiesKHR) = (PFN_vkGetMemoryFdPropertiesKHR)vkGetDeviceProcAddr(nativeDevice, "vkGetMemoryFdPropertiesKHR");
  }

  if (extensions.m_bExternalSemaphoreFd)
  {
    EZ_DISPATCH_CONTEXT_MEMBER_NAME(vkGetSemaphoreFdKHR) = (PFN_vkGetSemaphoreFdKHR)vkGetDeviceProcAddr(nativeDevice, "vkGetSemaphoreFdKHR");
    EZ_DISPATCH_CONTEXT_MEMBER_NAME(vkImportSemaphoreFdKHR) = (PFN_vkImportSemaphoreFdKHR)vkGetDeviceProcAddr(nativeDevice, "vkImportSemaphoreFdKHR");
  }
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (extensions.m_bExternalMemoryWin32)
  {
    EZ_DISPATCH_CONTEXT_MEMBER_NAME(vkGetMemoryWin32HandleKHR) = (PFN_vkGetMemoryWin32HandleKHR)vkGetDeviceProcAddr(nativeDevice, "vkGetMemoryWin32HandleKHR");
    EZ_DISPATCH_CONTEXT_MEMBER_NAME(vkGetMemoryWin32HandlePropertiesKHR) = (PFN_vkGetMemoryWin32HandlePropertiesKHR)vkGetDeviceProcAddr(nativeDevice, "vkGetMemoryWin32HandlePropertiesKHR");
  }

  if (extensions.m_bExternalSemaphoreWin32)
  {
    EZ_DISPATCH_CONTEXT_MEMBER_NAME(vkGetSemaphoreWin32HandleKHR) = (PFN_vkGetSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(nativeDevice, "vkGetSemaphoreWin32HandleKHR");
    EZ_DISPATCH_CONTEXT_MEMBER_NAME(vkImportSemaphoreWin32HandleKHR) = (PFN_vkImportSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(nativeDevice, "vkImportSemaphoreWin32HandleKHR");
  }
#endif
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
#  if EZ_ENABLED(EZ_PLATFORM_LINUX)

VkResult ezVulkanDispatchContext::vkGetMemoryFdKHR(VkDevice device, const VkMemoryGetFdInfoKHR* pGetFdInfo, int* pFd) const
{
  EZ_ASSERT_DEBUG(m_pvkGetMemoryFdKHR != nullptr, "vkGetMemoryFdKHR not supported");
  return m_pvkGetMemoryFdKHR(device, pGetFdInfo, pFd);
}

VkResult ezVulkanDispatchContext::vkGetMemoryFdPropertiesKHR(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, int fd, VkMemoryFdPropertiesKHR* pMemoryFdProperties) const
{
  EZ_ASSERT_DEBUG(m_pvkGetMemoryFdPropertiesKHR != nullptr, "vkGetMemoryFdPropertiesKHR not supported");
  return m_pvkGetMemoryFdPropertiesKHR(device, handleType, fd, pMemoryFdProperties);
}

VkResult ezVulkanDispatchContext::vkGetSemaphoreFdKHR(VkDevice device, const VkSemaphoreGetFdInfoKHR* pGetFdInfo, int* pFd) const
{
  EZ_ASSERT_DEBUG(m_pvkGetSemaphoreFdKHR != nullptr, "vkGetSemaphoreFdKHR not supported");
  return m_pvkGetSemaphoreFdKHR(device, pGetFdInfo, pFd);
}

VkResult ezVulkanDispatchContext::vkImportSemaphoreFdKHR(VkDevice device, const VkImportSemaphoreFdInfoKHR* pImportSemaphoreFdInfo) const
{
  EZ_ASSERT_DEBUG(m_pvkImportSemaphoreFdKHR != nullptr, "vkImportSemaphoreFdKHR not supported");
  return m_pvkImportSemaphoreFdKHR(device, pImportSemaphoreFdInfo);
}

#  elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)

VkResult ezVulkanDispatchContext::vkGetMemoryWin32HandleKHR(VkDevice device, const VkMemoryGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pWin32Handle) const
{
  EZ_ASSERT_DEBUG(m_pvkGetMemoryWin32HandleKHR != nullptr, "vkGetMemoryWin32HandleKHR not supported");
  return m_pvkGetMemoryWin32HandleKHR(device, pGetWin32HandleInfo, pWin32Handle);
}

VkResult ezVulkanDispatchContext::vkGetMemoryWin32HandlePropertiesKHR(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, HANDLE Win32Handle, VkMemoryWin32HandlePropertiesKHR* pMemoryWin32HandleProperties) const
{
  EZ_ASSERT_DEBUG(m_pvkGetMemoryWin32HandlePropertiesKHR != nullptr, "vkGetMemoryWin32HandlePropertiesKHR not supported");
  return m_pvkGetMemoryWin32HandlePropertiesKHR(device, handleType, Win32Handle, pMemoryWin32HandleProperties);
}

VkResult ezVulkanDispatchContext::vkGetSemaphoreWin32HandleKHR(VkDevice device, const VkSemaphoreGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pWin32Handle) const
{
  EZ_ASSERT_DEBUG(m_pvkGetSemaphoreWin32HandleKHR != nullptr, "vkGetSemaphoreWin32HandleKHR not supported");
  return m_pvkGetSemaphoreWin32HandleKHR(device, pGetWin32HandleInfo, pWin32Handle);
}

VkResult ezVulkanDispatchContext::vkImportSemaphoreWin32HandleKHR(VkDevice device, const VkImportSemaphoreWin32HandleInfoKHR* pImportSemaphoreWin32HandleInfo) const
{
  EZ_ASSERT_DEBUG(m_pvkImportSemaphoreWin32HandleKHR != nullptr, "vkImportSemaphoreWin32HandleKHR not supported");
  return m_pvkImportSemaphoreWin32HandleKHR(device, pImportSemaphoreWin32HandleInfo);
}

#  endif
#endif


