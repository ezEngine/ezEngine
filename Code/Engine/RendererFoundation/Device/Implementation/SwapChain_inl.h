#pragma once

ezGALTextureHandle ezGALSwapChain::GetBackBufferTexture() const
{
  return m_hBackBufferTexture;
}

ezGALTextureHandle ezGALSwapChain::GetDepthStencilBufferTexture() const
{
  return m_hDepthStencilBufferTexture;
}
