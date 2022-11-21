#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/DispatchContext.h>

void ezVulkanDispatchContext::Init(ezGALDeviceVulkan& device)
{
  m_pDevice = &device;

  VkDevice nativeDevice = (VkDevice)device.GetVulkanDevice();
  const ezGALDeviceVulkan::Extensions& extensions = device.GetExtensions();

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
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
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
#endif