
vk::Buffer ezGALBufferVulkan::GetVkBuffer() const
{
  m_currentBuffer.m_currentFrame = m_pDeviceVulkan->GetCurrentFrame();
  return m_currentBuffer.m_buffer;
}

vk::IndexType ezGALBufferVulkan::GetIndexType() const
{
  return m_indexType;
}

ezVulkanAllocation ezGALBufferVulkan::GetAllocation() const
{
  return m_currentBuffer.m_alloc;
}

const ezVulkanAllocationInfo& ezGALBufferVulkan::GetAllocationInfo() const
{
  return m_allocInfo;
}

vk::PipelineStageFlags ezGALBufferVulkan::GetUsedByPipelineStage() const
{
  return m_stages;
}

vk::AccessFlags ezGALBufferVulkan::GetAccessMask() const
{
  return m_access;
}
