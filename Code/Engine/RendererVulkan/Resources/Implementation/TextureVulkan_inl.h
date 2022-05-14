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

vk::ImageLayout ezGALTextureVulkan::GetPreferredLayout(vk::ImageLayout targetLayout) const
{
  return m_preferredLayout == vk::ImageLayout::eGeneral ? vk::ImageLayout::eGeneral : targetLayout;
}

vk::PipelineStageFlags ezGALTextureVulkan::GetUsedByPipelineStage() const
{
  return m_stages;
}

vk::AccessFlags ezGALTextureVulkan::GetAccessMask() const
{
  return m_access;
}

ezVulkanAllocation ezGALTextureVulkan::GetAllocation() const
{
  return m_alloc;
}

const ezVulkanAllocationInfo& ezGALTextureVulkan::GetAllocationInfo() const
{
  return m_allocInfo;
}

const vk::Image ezGALTextureVulkan::GetStagingTexture() const
{
  return m_stagingImage;
}

ezVulkanAllocation ezGALTextureVulkan::GetStagingAllocation() const
{
  return m_stagingAlloc;
}

const ezVulkanAllocationInfo& ezGALTextureVulkan::GetStagingAllocationInfo() const
{
  return m_stagingAllocInfo;
}
