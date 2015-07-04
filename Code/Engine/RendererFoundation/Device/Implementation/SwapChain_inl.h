#pragma once

ezGALTextureHandle ezGALSwapChain::GetBackBufferTexture() const
{
  return m_hBackBufferTexture;
}

ezGALRenderTargetViewHandle ezGALSwapChain::GetBackBufferRenderTargetView() const
{
  return m_hBackBufferRTV;
}

ezGALTextureHandle ezGALSwapChain::GetDepthStencilBufferTexture() const
{
  return m_hDepthStencilBufferTexture;
}

ezGALRenderTargetViewHandle ezGALSwapChain::GetDepthStencilTargetView() const
{
  return m_hBackBufferDSV;
}
