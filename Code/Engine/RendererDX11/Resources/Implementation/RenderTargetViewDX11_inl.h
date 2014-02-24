

  EZ_FORCE_INLINE ID3D11RenderTargetView* ezGALRenderTargetViewDX11::GetRenderTargetView() const
  {
    return m_pRenderTargetView;
  }

  EZ_FORCE_INLINE ID3D11DepthStencilView* ezGALRenderTargetViewDX11::GetDepthStencilView() const
  {
    return m_pDepthStencilView;
  }

  EZ_FORCE_INLINE ID3D11UnorderedAccessView* ezGALRenderTargetViewDX11::GetUnorderedAccessView() const
  {
	  return m_pUnorderedAccessView;
  }