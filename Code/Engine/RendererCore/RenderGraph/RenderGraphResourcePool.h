#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezGALDevice;

/// Ref-counted wrapper around a pooled GPU texture.
class EZ_RENDERERCORE_DLL ezPooledRenderTexture : public ezRefCounted
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezPooledRenderTexture);
  friend class ezRenderGraphResourcePool;

public:
  ezGALTextureHandle GetHandle() const { return m_hTexture; }
  const ezGALTextureCreationDescription& GetDescription() const { return m_Desc; }
  ~ezPooledRenderTexture() = default;

private:
  ezPooledRenderTexture() = default;

  ezGALTextureHandle m_hTexture;
  ezGALTextureCreationDescription m_Desc;
  ezUInt64 m_uiUsedCounter = 0;
};

/// Ref-counted wrapper around a pooled GPU buffer.
class EZ_RENDERERCORE_DLL ezPooledRenderBuffer : public ezRefCounted
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezPooledRenderBuffer);
  friend class ezRenderGraphResourcePool;

public:
  ezGALBufferHandle GetHandle() const { return m_hBuffer; }
  const ezGALBufferCreationDescription& GetDescription() const { return m_Desc; }
  ~ezPooledRenderBuffer() = default;

private:
  ezPooledRenderBuffer() = default;

  ezGALBufferHandle m_hBuffer;
  ezGALBufferCreationDescription m_Desc;
  ezUInt64 m_uiUsedCounter = 0;
};

/// Global, thread-safe transient resource pool for render graphs.
///
/// Resources are indexed by (descHash, index) pairs. The pool owns a shared pointer
/// to every resource it creates. Allocators (ezRenderGraphResourceAllocator) also hold
/// shared pointers, keeping the resources alive as long as any graph references them.
///
/// GC destroys GPU resources whose ref count is 1 (only the pool holds them)
/// and that haven't been used for a configurable number of frames.
///
/// Users don't call into the pool directly. Instead, they create an
/// ezRenderGraphResourceAllocator which manages acquire/release semantics.
class EZ_RENDERERCORE_DLL ezRenderGraphResourcePool
{
  friend class ezRenderGraphResourceAllocator;

public:
  ezRenderGraphResourcePool(ezGALDevice* pDevice);
  ~ezRenderGraphResourcePool();

  /// End-of-frame housekeeping: increments frame counter and runs periodic GC.
  void EndFrame();

  /// Run GC: destroy GPU resources whose ref count is 1 (only pool holds them)
  /// and that haven't been used for uiMinimumAge frames.
  void RunGC(ezUInt32 uiMinimumAge);

  /// Destroy all pooled GPU resources immediately.
  void Clear();

private:
  /// Thread-safe. Returns the texture at (descHash, uiIndex), creating the GPU
  /// resource on first access. Bumps m_uiLastUsedFrame.
  ezSharedPtr<ezPooledRenderTexture> AcquireTexture(const ezGALTextureCreationDescription& desc, ezUInt32 uiIndex);

  /// Thread-safe. Returns the buffer at (descHash, uiIndex), creating the GPU
  /// resource on first access. Bumps m_uiLastUsedFrame.
  ezSharedPtr<ezPooledRenderBuffer> AcquireBuffer(const ezGALBufferCreationDescription& desc, ezUInt32 uiIndex);

  void UpdateMemoryStats() const;

  ezGALDevice* m_pDevice = nullptr;
  ezMutex m_Mutex;
  ezUInt64 m_uiFrameCounter = 0;

  // descHash -> array of pooled resources. Array index = the uiIndex parameter.
  ezHashTable<ezUInt64, ezDynamicArray<ezSharedPtr<ezPooledRenderTexture>>> m_Textures;
  ezHashTable<ezUInt64, ezDynamicArray<ezSharedPtr<ezPooledRenderBuffer>>> m_Buffers;

  ezUInt64 m_uiCurrentlyAllocatedMemory = 0;
  ezUInt16 m_uiFramesSinceLastGC = 0;
  static constexpr ezUInt16 s_uiFramesThresholdForGC = 60;
  static constexpr ezUInt32 s_uiMinimumAgeForGC = 10;
};
