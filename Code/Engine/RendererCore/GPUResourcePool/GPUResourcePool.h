#pragma once

#include <RendererCore/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Mutex.h>

/// \brief This class serves as a pool for GPU related resources (e.g. buffers and textures required for rendering).
/// Note that the functions creating and returning render targets are thread safe (by using a mutex).
class EZ_RENDERERCORE_DLL ezGPUResourcePool
{
public:

  ezGPUResourcePool();
  ~ezGPUResourcePool();

  /// \brief Returns a render target handle for the given texture description
  /// Note that you should return the handle to the pool and never destroy it directly with the device.
  ezGALTextureHandle GetRenderTarget(const ezGALTextureCreationDescription& TextureDesc);

  /// \brief Convenience functions which creates a texture description fit for a 2d render target without a mip chains.
  ezGALTextureHandle GetRenderTarget(ezUInt32 uiWidth, ezUInt32 uiHeight, ezGALResourceFormat::Enum eFormat);

  /// \brief Returns a render target to the pool so other consumers can use it.
  /// Note that targets which are returned to the pool are susceptible to destruction due to garbage collection.
  void ReturnRenderTarget(ezGALTextureHandle hRenderTarget);

  /// \brief Tries to free resources which are currently in the pool.
  /// Triggered automatically due to allocation number / size thresholds but can be triggered manually (e.g. after editor window resize)
  void RunGC();


  static ezGPUResourcePool* GetDefaultInstance();
  static void SetDefaultInstance(ezGPUResourcePool* pDefaultInstance);

protected:

  void CheckAndPotentiallyRunGC();
  void UpdateMemoryStats() const;

  ezUInt64 m_uiMemoryThresholdForGC;
  ezUInt64 m_uiCurrentlyAllocatedMemory;
  ezUInt32 m_uiNumAllocationsThresholdForGC;
  ezUInt32 m_uiNumAllocationsSinceLastGC;


  ezDynamicArray<ezGALTextureHandle> m_AvailableTextures;
  ezDynamicArray<ezGALTextureHandle> m_TexturesInUse;

  ezMutex m_Lock;

  ezGALDevice* m_pDevice;

private:

  static ezGPUResourcePool* s_pDefaultInstance;

};