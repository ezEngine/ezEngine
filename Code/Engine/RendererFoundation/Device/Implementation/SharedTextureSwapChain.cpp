#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SharedTextureSwapChain.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezGALSharedTextureSwapChain, ezGALSwapChain, 1, ezRTTINoAllocator)
{
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezGALSharedTextureSwapChain::Functor ezGALSharedTextureSwapChain::s_Factory;

void ezGALSharedTextureSwapChain::SetFactoryMethod(Functor factory)
{
  s_Factory = factory;
}

ezGALSwapChainHandle ezGALSharedTextureSwapChain::Create(const ezGALSharedTextureSwapChainCreationDescription& desc)
{
  EZ_ASSERT_DEV(s_Factory.IsValid(), "No factory method assigned for ezGALWindowSwapChain.");
  return s_Factory(desc);
}

ezGALSharedTextureSwapChain::ezGALSharedTextureSwapChain(const ezGALSharedTextureSwapChainCreationDescription& desc)
  : ezGALSwapChain(ezGetStaticRTTI<ezGALSharedTextureSwapChain>())
  , m_Desc(desc)
{
}

void ezGALSharedTextureSwapChain::AcquireNextRenderTarget(ezGALDevice* pDevice)
{

}

void ezGALSharedTextureSwapChain::PresentRenderTarget(ezGALDevice* pDevice)
{

}

ezResult ezGALSharedTextureSwapChain::UpdateSwapChain(ezGALDevice* pDevice, ezEnum<ezGALPresentMode> newPresentMode)
{
  return EZ_SUCCESS;
}

ezResult ezGALSharedTextureSwapChain::InitPlatform(ezGALDevice* pDevice)
{
  // Create textures
  for (ezUInt32 i = 0; i < m_Desc.m_Textures.GetCount(); ++i)
  {
    ezGALPlatformSharedHandle handle = m_Desc.m_Textures[i];
    ezGALTextureHandle tex1 = pDevice->OpenSharedTexture(m_Desc.m_TextureDesc, handle);
    if(tex1.IsInvalidated())
    {
      ezLog::Error("Failed to open shared texture");
      return EZ_FAILURE;
    }
  }

  ezHybridArray<ezGALTextureHandle, 3> m_hSharedTextures;
  ezHybridArray<const ezGALSharedTexture*, 3> m_pSharedTextures;
  ezHybridArray<ezUInt64, 3> m_CurrentSemaphoreValue;

  // get interfaces

  // init semaphores
  return EZ_SUCCESS;
}

ezResult ezGALSharedTextureSwapChain::DeInitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_SharedTextureSwapChain);
