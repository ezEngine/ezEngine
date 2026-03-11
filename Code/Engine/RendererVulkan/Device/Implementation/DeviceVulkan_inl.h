EZ_ALWAYS_INLINE vk::Device ezGALDeviceVulkan::GetVulkanDevice() const
{
  return m_device;
}

EZ_ALWAYS_INLINE const ezGALDeviceVulkan::Queue& ezGALDeviceVulkan::GetGraphicsQueue() const
{
  return m_graphicsQueue;
}

EZ_ALWAYS_INLINE const ezGALDeviceVulkan::Queue& ezGALDeviceVulkan::GetTransferQueue() const
{
  return m_transferQueue;
}

EZ_ALWAYS_INLINE vk::PhysicalDevice ezGALDeviceVulkan::GetVulkanPhysicalDevice() const
{
  return m_physicalDevice;
}

EZ_ALWAYS_INLINE vk::Instance ezGALDeviceVulkan::GetVulkanInstance() const
{
  return m_instance;
}

EZ_ALWAYS_INLINE const ezGALFormatLookupTableVulkan& ezGALDeviceVulkan::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}
