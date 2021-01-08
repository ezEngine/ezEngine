
vk::Buffer ezGALBufferVulkan::GetVkBuffer() const
{
  return m_buffer;
}

vk::IndexType ezGALBufferVulkan::GetIndexType() const
{
  return m_indexType;
}

vk::DeviceMemory ezGALBufferVulkan::GetMemory() const
{
  return m_memory;
}

vk::DeviceSize ezGALBufferVulkan::GetMemoryOffset() const
{
  return m_memoryOffset;
}
