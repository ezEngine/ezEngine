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

#define USE_SHARED_TEXTURE

void ezGALSharedTextureSwapChain::Arm(ezUInt32 uiTextureIndex, ezUInt64 uiCurrentSemaphoreValue)
{
  ezLog::Warning("AAA Arm {}, {}", uiTextureIndex, uiCurrentSemaphoreValue);
  if(m_uiCurrentTexture != ezMath::MaxValue<ezUInt32>())
  {
    // We did not use the previous texture index.
    m_Desc.m_OnPresent(m_uiCurrentTexture, m_uiCurrentSemaphoreValue);
    ezLog::Warning("AAA skipping frame {} {}", m_uiCurrentTexture, m_uiCurrentSemaphoreValue);
  }
  m_uiCurrentTexture = uiTextureIndex;
  m_uiCurrentSemaphoreValue = uiCurrentSemaphoreValue;

#ifdef USE_SHARED_TEXTURE
  m_RenderTargets.m_hRTs[0] = m_hSharedTextures[m_uiCurrentTexture];
#else
  m_RenderTargets.m_hRTs[0] = m_DUMMY;
#endif
}

void ezGALSharedTextureSwapChain::AcquireNextRenderTarget(ezGALDevice* pDevice)
{
  ezLog::Warning("AAA AcquireNextRenderTarget {}, {}", m_uiCurrentTexture, m_uiCurrentSemaphoreValue);
  EZ_ASSERT_DEV(m_uiCurrentTexture != ezMath::MaxValue<ezUInt32>(), "Acquire called without calling Arm first.");

#ifdef USE_SHARED_TEXTURE
  m_RenderTargets.m_hRTs[0] = m_hSharedTextures[m_uiCurrentTexture];
  m_pSharedTextures[m_uiCurrentTexture]->WaitSemaphoreGPU(m_uiCurrentSemaphoreValue);
#else
  m_RenderTargets.m_hRTs[0] = m_DUMMY;
#endif
}

void ezGALSharedTextureSwapChain::PresentRenderTarget(ezGALDevice* pDevice)
{
  m_RenderTargets.m_hRTs[0].Invalidate();
  ezLog::Warning("AAA PresentRenderTarget {}, {}", m_uiCurrentTexture, m_uiCurrentSemaphoreValue + 1);

  EZ_ASSERT_DEV(m_uiCurrentTexture != ezMath::MaxValue<ezUInt32>(), "Present called without calling Arm first.");
#ifdef USE_SHARED_TEXTURE
  m_pSharedTextures[m_uiCurrentTexture]->SignalSemaphoreGPU(m_uiCurrentSemaphoreValue + 1);
  m_Desc.m_OnPresent(m_uiCurrentTexture, m_uiCurrentSemaphoreValue + 1);
#else
  m_Desc.m_OnPresent(m_uiCurrentTexture, m_uiCurrentSemaphoreValue + 0);
#endif
  pDevice->Flush();

  m_uiCurrentTexture = ezMath::MaxValue<ezUInt32>();
}

ezResult ezGALSharedTextureSwapChain::UpdateSwapChain(ezGALDevice* pDevice, ezEnum<ezGALPresentMode> newPresentMode)
{
  return EZ_SUCCESS;
}

ezResult ezGALSharedTextureSwapChain::InitPlatform(ezGALDevice* pDevice)
{
  // Create textures
#ifdef USE_SHARED_TEXTURE
  for (ezUInt32 i = 0; i < m_Desc.m_Textures.GetCount(); ++i)
  {
    ezGALPlatformSharedHandle handle = m_Desc.m_Textures[i];
    ezGALTextureHandle hTexture = pDevice->OpenSharedTexture(m_Desc.m_TextureDesc, handle);
    if(hTexture.IsInvalidated())
    {
      ezLog::Error("Failed to open shared texture");
      return EZ_FAILURE;
    }
    m_hSharedTextures.PushBack(hTexture);
    const ezGALSharedTexture* pSharedTexture = pDevice->GetSharedTexture(hTexture);
    if (pSharedTexture == nullptr)
    {
      ezLog::Error("Created texture is not a shared texture");
      return EZ_FAILURE;
    }
    m_pSharedTextures.PushBack(pSharedTexture);
    m_CurrentSemaphoreValue.PushBack(0);
  }
  m_RenderTargets.m_hRTs[0] = m_hSharedTextures[0];
#else
  m_DUMMY = pDevice->CreateTexture(m_Desc.m_TextureDesc);
  m_RenderTargets.m_hRTs[0] = m_DUMMY;
#endif
  m_CurrentSize = {m_Desc.m_TextureDesc.m_uiWidth, m_Desc.m_TextureDesc.m_uiHeight};
  return EZ_SUCCESS;
}

ezResult ezGALSharedTextureSwapChain::DeInitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_SharedTextureSwapChain);
