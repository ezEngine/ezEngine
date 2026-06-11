EZ_ALWAYS_INLINE vk::Device ezGALDeviceVulkan::GetVulkanDevice() const
{
  return m_Device;
}

EZ_ALWAYS_INLINE const ezGALDeviceVulkan::Queue& ezGALDeviceVulkan::GetGraphicsQueue() const
{
  return m_GraphicsQueue;
}

EZ_ALWAYS_INLINE const ezGALDeviceVulkan::Queue& ezGALDeviceVulkan::GetTransferQueue() const
{
  return m_TransferQueue;
}

EZ_ALWAYS_INLINE vk::PhysicalDevice ezGALDeviceVulkan::GetVulkanPhysicalDevice() const
{
  return m_PhysicalDevice;
}

EZ_ALWAYS_INLINE vk::Instance ezGALDeviceVulkan::GetVulkanInstance() const
{
  return m_Instance;
}

EZ_ALWAYS_INLINE const ezGALFormatLookupTableVulkan& ezGALDeviceVulkan::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}
