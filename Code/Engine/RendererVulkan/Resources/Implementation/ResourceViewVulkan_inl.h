EZ_ALWAYS_INLINE const vk::DescriptorImageInfo& ezGALTextureResourceViewVulkan::GetImageInfo(bool bIsArray) const
{
  EZ_ASSERT_DEBUG((bIsArray ? m_resourceImageInfoArray : m_resourceImageInfo).imageView, "View does not support bIsArray: {}", bIsArray);
  return bIsArray ? m_resourceImageInfoArray : m_resourceImageInfo;
}

EZ_ALWAYS_INLINE vk::ImageSubresourceRange ezGALTextureResourceViewVulkan::GetRange() const
{
  return m_range;
}

EZ_ALWAYS_INLINE const vk::BufferView& ezGALBufferResourceViewVulkan::GetBufferView() const
{
  return m_bufferView;
}
