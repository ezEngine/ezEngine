vk::Image ezGALTextureVulkan::GetImage() const
{
  return m_image;
}

vk::ImageLayout ezGALTextureVulkan::GetCurrentLayout() const
{
  return m_currentLayout;
}

vk::ImageLayout ezGALTextureVulkan::GetPreferredLayout() const
{
  return m_preferredLayout;
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
