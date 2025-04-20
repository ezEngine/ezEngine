
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class EZ_RENDERERFOUNDATION_DLL ezGALTexture : public ezGALResource<ezGALTextureCreationDescription>
{
public:
  ezVec3U32 GetMipMapSize(ezUInt32 uiMipLevel) const;

protected:
  friend class ezGALDevice;

  ezGALTexture(const ezGALTextureCreationDescription& Description);
  virtual ~ezGALTexture();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) = 0;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

protected:
  ezGALTextureResourceViewHandle m_hDefaultResourceView;
  ezGALRenderTargetViewHandle m_hDefaultRenderTargetView;

  ezHashTable<ezUInt32, ezGALTextureResourceViewHandle> m_ResourceViews;
  ezHashTable<ezUInt32, ezGALRenderTargetViewHandle> m_RenderTargetViews;
  ezHashTable<ezUInt32, ezGALTextureUnorderedAccessViewHandle> m_UnorderedAccessViews;
};

/// \brief Optional interface for ezGALTexture if it was created via ezGALDevice::CreateSharedTexture.
/// A ezGALTexture can be a shared texture, but doesn't have to be. Access through ezGALDevice::GetSharedTexture.
class EZ_RENDERERFOUNDATION_DLL ezGALSharedTexture
{
public:
  /// \brief Returns the handle that can be used to open this texture on another device / process. Call  ezGALDevice::OpenSharedTexture to do so.
  virtual ezGALPlatformSharedHandle GetSharedHandle() const = 0;
  /// \brief Before the current render pipeline is executed, the GPU will wait for the semaphore to have the given value.
  /// \param iValue Value the semaphore needs to have before the texture can be used.
  virtual void WaitSemaphoreGPU(ezUInt64 uiValue) const = 0;
  /// \brief Once the current render pipeline is done on the GPU, the semaphore will be signaled with the given value.
  /// \param iValue Value the semaphore is set to once we are done using the texture (after the current render pipeline).
  virtual void SignalSemaphoreGPU(ezUInt64 uiValue) const = 0;
};
