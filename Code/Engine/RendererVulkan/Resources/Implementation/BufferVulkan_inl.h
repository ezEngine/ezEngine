
vk::Buffer ezGALBufferVulkan::GetVkBuffer() const
{
  return m_Buffer;
}

vk::IndexType ezGALBufferVulkan::GetIndexType() const
{
  return m_IndexType;
}

ezVulkanAllocation ezGALBufferVulkan::GetAllocation() const
{
  return m_pAlloc;
}

const ezVulkanAllocationInfo& ezGALBufferVulkan::GetAllocationInfo() const
{
  return m_AllocInfo;
}

vk::PipelineStageFlags ezGALBufferVulkan::GetUsedByPipelineStage() const
{
  return m_Stages;
}

vk::AccessFlags ezGALBufferVulkan::GetAccessMask() const
{
  return m_Access;
}
