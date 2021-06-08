vk::Image ezGALTextureVulkan::GetImage() const
{
  return m_image;
}

const ezGALBufferVulkan* ezGALTextureVulkan::GetStagingBuffer() const
{
  return m_pStagingBuffer;
}

vk::DeviceMemory ezGALTextureVulkan::GetMemory() const
{
  return m_memory;
}

vk::DeviceSize ezGALTextureVulkan::GetMemoryOffset() const
{
  return m_memoryOffset;
}

vk::DeviceSize ezGALTextureVulkan::GetMemorySize() const
{
  return m_memorySize;
}
