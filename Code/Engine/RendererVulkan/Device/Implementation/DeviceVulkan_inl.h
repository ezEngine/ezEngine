#include "..\DeviceVulkan.h"

EZ_ALWAYS_INLINE vk::Device ezGALDeviceVulkan::GetVulkanDevice() const
{
  return m_device;
}

EZ_ALWAYS_INLINE vk::Instance ezGALDeviceVulkan::GetVulkanInstance() const
{
  return m_instance;
}

EZ_ALWAYS_INLINE vk::CommandBuffer& ezGALDeviceVulkan::GetPrimaryCommandBuffer()
{
  return m_commandBuffers[m_uiCurrentCmdBufferIndex];
}

EZ_ALWAYS_INLINE ezArrayPtr<const ezUInt32> ezGALDeviceVulkan::GetQueueFamilyIndices() const
{
  return m_queueFamilyIndices.GetArrayPtr();
}

inline vk::Queue ezGALDeviceVulkan::GetQueue()
{
  return m_queue;
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
