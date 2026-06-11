

EZ_ALWAYS_INLINE vk::ImageView ezGALRenderTargetViewVulkan::GetImageView() const
{
  return m_ImageView;
}

EZ_ALWAYS_INLINE bool ezGALRenderTargetViewVulkan::IsFullRange() const
{
  return m_bBfullRange;
}

EZ_ALWAYS_INLINE vk::ImageSubresourceRange ezGALRenderTargetViewVulkan::GetRange() const
{
  return m_Range;
}
