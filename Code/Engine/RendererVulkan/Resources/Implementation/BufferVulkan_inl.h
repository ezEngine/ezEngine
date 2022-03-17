
vk::Buffer ezGALBufferVulkan::GetVkBuffer() const
{
  return m_buffer;
}

vk::IndexType ezGALBufferVulkan::GetIndexType() const
{
  return m_indexType;
}

ezVulkanAllocation ezGALBufferVulkan::GetAllocation() const
{
  return m_alloc;
}

const ezVulkanAllocationInfo& ezGALBufferVulkan::GetAllocationInfo() const
{
  return m_allocInfo;
}
