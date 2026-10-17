#include <RendererFoundation/RendererFoundationPCH.h>

#include <Core/System/Window.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/OffscreenSwapChain.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezGALOffscreenSwapChain, ezGALSwapChain, 1, ezRTTINoAllocator)
{
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezGALOffscreenSwapChain::Functor ezGALOffscreenSwapChain::s_Factory;

void ezGALOffscreenSwapChain::SetFactoryMethod(Functor factory)
{
  s_Factory = factory;
}

ezGALSwapChainHandle ezGALOffscreenSwapChain::Create(const ezGALWindowSwapChainCreationDescription& desc)
{
  EZ_ASSERT_DEV(s_Factory.IsValid(), "No factory method assigned for ezGALOffscreenSwapChain.");
  return s_Factory(desc);
}

ezGALOffscreenSwapChain::ezGALOffscreenSwapChain(const ezGALWindowSwapChainCreationDescription& desc)
  : ezGALSwapChain(ezGetStaticRTTI<ezGALOffscreenSwapChain>())
  , m_Desc(desc)
{
}

void ezGALOffscreenSwapChain::AcquireNextRenderTarget(ezGALDevice* pDevice)
{
  EZ_IGNORE_UNUSED(pDevice);
}

void ezGALOffscreenSwapChain::PresentRenderTarget(ezGALDevice* pDevice)
{
  EZ_IGNORE_UNUSED(pDevice);
}

ezResult ezGALOffscreenSwapChain::UpdateSwapChain(ezGALDevice* pDevice, ezEnum<ezGALPresentMode> newPresentMode)
{
  EZ_IGNORE_UNUSED(newPresentMode);

  DestroyBackBuffer(pDevice);
  return CreateBackBuffer(pDevice);
}

ezResult ezGALOffscreenSwapChain::InitPlatform(ezGALDevice* pDevice)
{
  if (m_Desc.m_pWindow == nullptr)
  {
    ezLog::Error("Offscreen swap chain creation requires a window.");
    return EZ_FAILURE;
  }

  EZ_SUCCEED_OR_RETURN(CreateBackBuffer(pDevice));
  m_Desc.m_pWindow->AddReference();
  return EZ_SUCCESS;
}

ezResult ezGALOffscreenSwapChain::DeInitPlatform(ezGALDevice* pDevice)
{
  DestroyBackBuffer(pDevice);
  m_Desc.m_pWindow->RemoveReference();
  return EZ_SUCCESS;
}

ezResult ezGALOffscreenSwapChain::CreateBackBuffer(ezGALDevice* pDevice)
{
  const ezSizeU32 backBufferSize = m_Desc.m_pWindow->GetClientAreaSize();
  if (!backBufferSize.HasNonZeroArea())
  {
    ezLog::Error("Offscreen swap chain creation requires a non-zero window size.");
    return EZ_FAILURE;
  }

  ezGALTextureCreationDescription textureDesc;
  textureDesc.SetAsRenderTarget(backBufferSize.width, backBufferSize.height, m_Desc.m_BackBufferFormat, m_Desc.m_SampleCount);
  textureDesc.m_ResourceAccess.m_bImmutable = false;

  m_RenderTargets.m_hRTs[0] = pDevice->CreateTexture(textureDesc);
  if (m_RenderTargets.m_hRTs[0].IsInvalidated())
    return EZ_FAILURE;

  m_CurrentSize = backBufferSize;
  pDevice->s_SwapChainUpdatedEvent.Broadcast(this);
  return EZ_SUCCESS;
}

void ezGALOffscreenSwapChain::DestroyBackBuffer(ezGALDevice* pDevice)
{
  pDevice->DestroyTexture(m_RenderTargets.m_hRTs[0]);
  m_RenderTargets.m_hRTs[0].Invalidate();
  m_CurrentSize = {};
}

EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_OffscreenSwapChain);
