EZ_ALWAYS_INLINE const vk::DescriptorImageInfo& ezGALTextureResourceViewVulkan::GetImageInfo(bool bIsArray) const
{
  return m_resourceImageInfo;
}

EZ_ALWAYS_INLINE vk::ImageSubresourceRange ezGALTextureResourceViewVulkan::GetRange() const
{
  return m_range;
}

EZ_ALWAYS_INLINE const vk::BufferView& ezGALBufferResourceViewVulkan::GetBufferView() const
{
  return m_bufferView;
}
