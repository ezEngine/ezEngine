

EZ_ALWAYS_INLINE ID3D11RenderTargetView* ezGALRenderTargetViewVulkan::GetRenderTargetView() const
{
  return m_pRenderTargetView;
}

EZ_ALWAYS_INLINE ID3D11DepthStencilView* ezGALRenderTargetViewVulkan::GetDepthStencilView() const
{
  return m_pDepthStencilView;
}

EZ_ALWAYS_INLINE ID3D11UnorderedAccessView* ezGALRenderTargetViewVulkan::GetUnorderedAccessView() const
{
  return m_pUnorderedAccessView;
}