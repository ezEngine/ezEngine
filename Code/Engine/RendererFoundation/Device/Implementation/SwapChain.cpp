#include <RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>

ezGALSwapChain::ezGALSwapChain(const ezGALSwapChainCreationDescription& Description)
    : ezGALObject(Description)
{
}

ezGALSwapChain::~ezGALSwapChain() {}


ezResult ezGALSwapChain::DeInitPlatform(ezGALDevice* pDevice)
{
  if (!m_hBackBufferTexture.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hBackBufferTexture);
    m_hBackBufferTexture.Invalidate();
  }

  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_SwapChain);

