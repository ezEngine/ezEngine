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

void ezGALSharedTextureSwapChain::Arm(ezUInt32 uiTextureIndex, ezUInt64 uiCurrentSemaphoreValue)
{
  if (m_uiCurrentTexture != ezMath::MaxValue<ezUInt32>())
  {
    // We did not use the previous texture index.
    m_Desc.m_OnPresent(m_uiCurrentTexture, m_uiCurrentSemaphoreValue);
  }
  m_uiCurrentTexture = uiTextureIndex;
  m_uiCurrentSemaphoreValue = uiCurrentSemaphoreValue;

  m_RenderTargets.m_hRTs[0] = m_SharedTextureHandles[m_uiCurrentTexture];
}

void ezGALSharedTextureSwapChain::AcquireNextRenderTarget(ezGALDevice* pDevice)
{
  EZ_IGNORE_UNUSED(pDevice);

  EZ_ASSERT_DEV(m_uiCurrentTexture != ezMath::MaxValue<ezUInt32>(), "Acquire called without calling Arm first.");

  m_RenderTargets.m_hRTs[0] = m_SharedTextureHandles[m_uiCurrentTexture];
  m_SharedTextureInterfaces[m_uiCurrentTexture]->WaitSemaphoreGPU(m_uiCurrentSemaphoreValue);
}

void ezGALSharedTextureSwapChain::PresentRenderTarget(ezGALDevice* pDevice)
{
  m_RenderTargets.m_hRTs[0].Invalidate();

  EZ_ASSERT_DEV(m_uiCurrentTexture != ezMath::MaxValue<ezUInt32>(), "Present called without calling Arm first.");

  m_SharedTextureInterfaces[m_uiCurrentTexture]->SignalSemaphoreGPU(m_uiCurrentSemaphoreValue + 1);
  m_Desc.m_OnPresent(m_uiCurrentTexture, m_uiCurrentSemaphoreValue + 1);

  pDevice->Flush();

  m_uiCurrentTexture = ezMath::MaxValue<ezUInt32>();
}

ezResult ezGALSharedTextureSwapChain::UpdateSwapChain(ezGALDevice* pDevice, ezEnum<ezGALPresentMode> newPresentMode)
{
  EZ_IGNORE_UNUSED(pDevice);
  EZ_IGNORE_UNUSED(newPresentMode);

  return EZ_SUCCESS;
}

ezResult ezGALSharedTextureSwapChain::InitPlatform(ezGALDevice* pDevice)
{
  // Create textures
  for (ezUInt32 i = 0; i < m_Desc.m_Textures.GetCount(); ++i)
  {
    ezGALPlatformSharedHandle handle = m_Desc.m_Textures[i];
    ezGALTextureHandle hTexture = pDevice->OpenSharedTexture(m_Desc.m_TextureDesc, handle);
    if (hTexture.IsInvalidated())
    {
      ezLog::Error("Failed to open shared texture");
      return EZ_FAILURE;
    }
    m_SharedTextureHandles.PushBack(hTexture);
    const ezGALSharedTexture* pSharedTexture = pDevice->GetSharedTexture(hTexture);
    if (pSharedTexture == nullptr)
    {
      ezLog::Error("Created texture is not a shared texture");
      return EZ_FAILURE;
    }
    m_SharedTextureInterfaces.PushBack(pSharedTexture);
    m_CurrentSemaphoreValue.PushBack(0);
  }
  m_RenderTargets.m_hRTs[0] = m_SharedTextureHandles[0];
  m_CurrentSize = {m_Desc.m_TextureDesc.m_uiWidth, m_Desc.m_TextureDesc.m_uiHeight};
  return EZ_SUCCESS;
}

ezResult ezGALSharedTextureSwapChain::DeInitPlatform(ezGALDevice* pDevice)
{
  for (ezUInt32 i = 0; i < m_SharedTextureHandles.GetCount(); ++i)
  {
    pDevice->DestroySharedTexture(m_SharedTextureHandles[i]);
  }
  m_uiCurrentTexture = ezMath::MaxValue<ezUInt32>();
  m_uiCurrentSemaphoreValue = 0;
  m_SharedTextureHandles.Clear();
  m_SharedTextureInterfaces.Clear();
  m_CurrentSemaphoreValue.Clear();

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_SharedTextureSwapChain);
