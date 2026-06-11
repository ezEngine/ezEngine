vk::Image ezGALTextureVulkan::GetImage() const
{
  return m_Image;
}

ezVulkanAllocation ezGALTextureVulkan::GetAllocation() const
{
  return m_pAlloc;
}

const ezVulkanAllocationInfo& ezGALTextureVulkan::GetAllocationInfo() const
{
  return m_AllocInfo;
}
