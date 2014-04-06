#pragma once

inline ezGALRenderTargetConfigHandle ezGALSwapChain::GetRenderTargetViewConfig() const
{
  return m_hRenderTargetConfig;
}

inline ezGALTextureHandle ezGALSwapChain::GetBackBufferTexture() const
{
  return m_hBackBufferTexture;
}

inline ezGALTextureHandle ezGALSwapChain::GetDepthStencilBufferTexture() const
{
  return m_hDepthStencilBufferTexture;
}
