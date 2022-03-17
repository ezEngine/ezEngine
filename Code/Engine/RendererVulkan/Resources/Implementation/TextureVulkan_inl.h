vk::Image ezGALTextureVulkan::GetImage() const
{
  return m_image;
}

ezVulkanAllocation ezGALTextureVulkan::GetAllocation() const
{
  return m_alloc;
}

const ezVulkanAllocationInfo& ezGALTextureVulkan::GetAllocationInfo() const
{
  return m_allocInfo;
}

const ezGALBufferVulkan* ezGALTextureVulkan::GetStagingBuffer() const
{
  return m_pStagingBuffer;
}
