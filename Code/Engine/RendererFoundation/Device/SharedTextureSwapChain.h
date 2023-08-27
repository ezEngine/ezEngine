#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Texture.h>

/// \brief Description of a shared texture swap chain. Use ezGALSharedTextureSwapChain::Create to create instance.
struct ezGALSharedTextureSwapChainCreationDescription : public ezHashableStruct<ezGALSharedTextureSwapChainCreationDescription>
{
  ezGALTextureCreationDescription m_TextureDesc;
  ezHybridArray<ezGALPlatformSharedHandle, 3> m_Textures;
  /// \brief Called when rendering to a swap chain texture has been submitted to the GPU queue. Use this to get the new semaphore value of the previously armed texture.
  ezDelegate<void(ezUInt32 uiTextureIndex, ezUInt64 uiSemaphoreValue)> m_OnPresent;
};

/// \brief Use to render to a set of shared textures.
/// To use it, it needs to be armed with the next shared texture index and its current semaphore value.
class EZ_RENDERERFOUNDATION_DLL ezGALSharedTextureSwapChain : public ezGALSwapChain
{
  friend class ezGALDevice;
public:
  using Functor = ezDelegate<ezGALSwapChainHandle(const ezGALSharedTextureSwapChainCreationDescription&)>;
  static void SetFactoryMethod(Functor factory);

  /// \brief Creates an instance of a ezGALSharedTextureSwapChain.
  static ezGALSwapChainHandle Create(const ezGALSharedTextureSwapChainCreationDescription& desc);

public:
  /// \brief Call this before rendering.
  /// \param uiTextureIndex Texture to render into.
  /// \param uiCurrentSemaphoreValue Current semaphore value of the texture.
  void Arm(ezUInt32 uiTextureIndex, ezUInt64 uiCurrentSemaphoreValue);

protected:
  ezGALSharedTextureSwapChain(const ezGALSharedTextureSwapChainCreationDescription& desc);
  virtual void AcquireNextRenderTarget(ezGALDevice* pDevice) override;
  virtual void PresentRenderTarget(ezGALDevice* pDevice) override;
  virtual ezResult UpdateSwapChain(ezGALDevice* pDevice, ezEnum<ezGALPresentMode> newPresentMode) override;
  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

protected:
  static Functor s_Factory;

protected:
  ezUInt32 m_uiCurrentTexture = ezMath::MaxValue<ezUInt32>();
  ezUInt64 m_uiCurrentSemaphoreValue = 0;
  ezHybridArray<ezGALTextureHandle, 3> m_hSharedTextures;
  ezHybridArray<const ezGALSharedTexture*, 3> m_pSharedTextures;
  ezHybridArray<ezUInt64, 3> m_CurrentSemaphoreValue;
  ezGALSharedTextureSwapChainCreationDescription m_Desc = {};
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERFOUNDATION_DLL, ezGALSharedTextureSwapChain);
