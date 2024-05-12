

EZ_ALWAYS_INLINE const vk::DescriptorImageInfo& ezGALTextureUnorderedAccessViewVulkan::GetImageInfo() const
{
  return m_resourceImageInfo;
}

EZ_ALWAYS_INLINE vk::ImageSubresourceRange ezGALTextureUnorderedAccessViewVulkan::GetRange() const
{
  return m_range;
}

EZ_ALWAYS_INLINE const vk::BufferView& ezGALBufferUnorderedAccessViewVulkan::GetBufferView() const
{
  return m_bufferView;
}
