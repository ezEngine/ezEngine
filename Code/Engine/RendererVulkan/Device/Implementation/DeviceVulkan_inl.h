#include "../DeviceVulkan.h"

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

/*
inline ID3D11Query* ezGALDeviceVulkan::GetTimestamp(ezGALTimestampHandle hTimestamp)
{
  if (hTimestamp.m_uiIndex < m_Timestamps.GetCount())
  {
    return m_Timestamps[static_cast<ezUInt32>(hTimestamp.m_uiIndex)];
  }

  return nullptr;
}

*/
