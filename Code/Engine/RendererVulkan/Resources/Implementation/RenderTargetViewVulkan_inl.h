

EZ_ALWAYS_INLINE vk::ImageView ezGALRenderTargetViewVulkan::GetImageView() const
{
  return m_imageView;
}

EZ_ALWAYS_INLINE bool ezGALRenderTargetViewVulkan::IsFullRange() const
{
  return m_bfullRange;
}

EZ_ALWAYS_INLINE vk::ImageSubresourceRange ezGALRenderTargetViewVulkan::GetRange() const
{
  return m_range;
}
