EZ_ALWAYS_INLINE const vk::DescriptorImageInfo& ezGALResourceViewVulkan::GetImageInfo() const
{
  return m_resourceImageInfo;
}

EZ_ALWAYS_INLINE vk::ImageSubresourceRange ezGALResourceViewVulkan::GetRange() const
{
  return m_range;
}
