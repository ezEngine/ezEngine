#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Texture.h>

struct ezGALSharedTextureSwapChainCreationDescription : public ezHashableStruct<ezGALSharedTextureSwapChainCreationDescription>
{
  ezGALTextureCreationDescription m_TextureDesc;
  ezHybridArray<ezGALPlatformSharedHandle, 3> m_Textures;
  ezDelegate<void(ezUInt32 uiTextureIndex, ezUInt64 uiSemaphoreValue)> m_OnPresent;
};

class EZ_RENDERERFOUNDATION_DLL ezGALSharedTextureSwapChain : public ezGALSwapChain
{
public:
  using Functor = ezDelegate<ezGALSwapChainHandle(const ezGALSharedTextureSwapChainCreationDescription&)>;
  static void SetFactoryMethod(Functor factory);

  static ezGALSwapChainHandle Create(const ezGALSharedTextureSwapChainCreationDescription& desc);

public:
  ezGALSharedTextureSwapChain(const ezGALSharedTextureSwapChainCreationDescription& desc);

  void Arm(ezUInt32 uiTextureIndex, ezUInt64 uiCurrentSemaphoreValue);

  virtual void AcquireNextRenderTarget(ezGALDevice* pDevice) override;
  virtual void PresentRenderTarget(ezGALDevice* pDevice) override;
  virtual ezResult UpdateSwapChain(ezGALDevice* pDevice, ezEnum<ezGALPresentMode> newPresentMode) override;

protected:
  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  static Functor s_Factory;

protected:
  ezUInt32 m_uiCurrentTexture = ezMath::MaxValue<ezUInt32>();
  ezUInt64 m_uiCurrentSemaphoreValue = 0;
  ezHybridArray<ezGALTextureHandle, 3> m_hSharedTextures;
  ezHybridArray<const ezGALSharedTexture*, 3> m_pSharedTextures;
  ezHybridArray<ezUInt64, 3> m_CurrentSemaphoreValue;
  ezGALSharedTextureSwapChainCreationDescription m_Desc = {};

  ezGALTextureHandle m_DUMMY;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERFOUNDATION_DLL, ezGALSharedTextureSwapChain);
