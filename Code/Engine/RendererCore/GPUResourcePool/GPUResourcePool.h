#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Threading/Mutex.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

struct ezGALDeviceEvent;

/// \brief This class serves as a pool for GPU related resources (e.g. buffers and textures required for rendering).
/// Note that the functions creating and returning render targets are thread safe (by using a mutex).
class EZ_RENDERERCORE_DLL ezGPUResourcePool
{
public:
  ezGPUResourcePool();
  ~ezGPUResourcePool();

  /// \brief Returns a render target handle for the given texture description
  /// Note that you should return the handle to the pool and never destroy it directly with the device.
  ezGALTextureHandle GetRenderTarget(const ezGALTextureCreationDescription& textureDesc);

  /// \brief Convenience functions which creates a texture description fit for a 2d render target without a mip chains.
  ezGALTextureHandle GetRenderTarget(ezUInt32 uiWidth, ezUInt32 uiHeight, ezGALResourceFormat::Enum format,
    ezGALMSAASampleCount::Enum sampleCount = ezGALMSAASampleCount::None, ezUInt32 uiSliceColunt = 1, ezGALTextureType::Enum textureType = ezGALTextureType::Texture2D);

  /// \brief Returns a render target to the pool so other consumers can use it.
  /// Note that targets which are returned to the pool are susceptible to destruction due to garbage collection.
  void ReturnRenderTarget(ezGALTextureHandle hRenderTarget);


  /// \brief Returns a buffer handle for the given buffer description
  ezGALBufferHandle GetBuffer(const ezGALBufferCreationDescription& bufferDesc);

  /// \brief Returns a buffer to the pool so other consumers can use it.
  void ReturnBuffer(ezGALBufferHandle hBuffer);


  /// \brief Tries to free resources which are currently in the pool.
  /// Triggered automatically due to allocation number / size thresholds but can be triggered manually (e.g. after editor window resize)
  ///
  /// \param uiMinimumAge How many frames at least the resource needs to have been unused before it will be GCed.
  void RunGC(ezUInt32 uiMinimumAge);


  static ezGPUResourcePool* GetDefaultInstance();
  static void SetDefaultInstance(ezGPUResourcePool* pDefaultInstance);

protected:
  void CheckAndPotentiallyRunGC();
  void UpdateMemoryStats() const;
  void GALDeviceEventHandler(const ezGALDeviceEvent& e);

  struct TextureHandleWithAge
  {
    ezGALTextureHandle m_hTexture;
    ezUInt64 m_uiLastUsed = 0;
  };

  struct BufferHandleWithAge
  {
    ezGALBufferHandle m_hBuffer;
    ezUInt64 m_uiLastUsed = 0;
  };

  ezEventSubscriptionID m_GALDeviceEventSubscriptionID = 0;
  ezUInt64 m_uiMemoryThresholdForGC = 256 * 1024 * 1024;
  ezUInt64 m_uiCurrentlyAllocatedMemory = 0;
  ezUInt16 m_uiNumAllocationsThresholdForGC = 128;
  ezUInt16 m_uiNumAllocationsSinceLastGC = 0;
  ezUInt16 m_uiFramesThresholdSinceLastGC = 60; ///< Every 60 frames resources unused for more than 10 frames in a row are GCed.
  ezUInt16 m_uiFramesSinceLastGC = 0;

  ezMap<ezUInt32, ezDynamicArray<TextureHandleWithAge>> m_AvailableTextures;
  ezSet<ezGALTextureHandle> m_TexturesInUse;

  ezMap<ezUInt32, ezDynamicArray<BufferHandleWithAge>> m_AvailableBuffers;
  ezSet<ezGALBufferHandle> m_BuffersInUse;
  ezDynamicArray<ezGALBufferHandle> m_BuffersToBeReused;

  ezMutex m_Lock;

  ezGALDevice* m_pDevice;

private:
  static ezGPUResourcePool* s_pDefaultInstance;
};
