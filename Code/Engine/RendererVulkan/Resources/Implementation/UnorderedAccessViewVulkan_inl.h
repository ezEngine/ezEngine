EZ_ALWAYS_INLINE const vk::DescriptorImageInfo& ezGALUnorderedAccessViewVulkan::GetImageInfo() const
{
  return m_resourceImageInfo;
}

EZ_ALWAYS_INLINE vk::ImageSubresourceRange ezGALUnorderedAccessViewVulkan::GetRange() const
{
  return m_range;
}
