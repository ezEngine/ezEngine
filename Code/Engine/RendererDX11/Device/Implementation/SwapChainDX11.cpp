
#include <RendererDX11/PCH.h>
#include <RendererDX11/Device/SwapChainDX11.h>
#include <Foundation/Logging/Log.h>
#include <System/Window/Window.h>

#include <d3d11.h>

ezGALSwapChainDX11::ezGALSwapChainDX11(const ezGALSwapChainCreationDescription& Description, ezGALTextureHandle hBackBufferTexture, ezGALRenderTargetViewHandle hBackBufferRenderTargetView, IDXGISwapChain* pDXSwapChain)
  : ezGALSwapChain(Description, hBackBufferTexture, hBackBufferRenderTargetView), m_pDXSwapChain(pDXSwapChain)
{

  EZ_ASSERT(pDXSwapChain != NULL, "Creating DX11 SwapChain with invalid pointer!");
}

ezGALSwapChainDX11::~ezGALSwapChainDX11()
{
  EZ_GAL_DX11_RELEASE(m_pDXSwapChain);
}
