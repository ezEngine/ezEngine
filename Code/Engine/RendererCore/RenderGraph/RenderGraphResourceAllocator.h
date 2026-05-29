#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/RenderGraph/RenderGraphResourcePool.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Utils/ResourceStateTracker.h> // provides ezHashHelper<ezGALTextureHandle/ezGALBufferHandle>

/// Per-graph, single-threaded resource allocator that sits in front of the
/// global ezRenderGraphResourcePool.
///
/// Each render graph owns one allocator. The allocator provides a simple
/// AcquireTexture / ReleaseTexture API. Internally it tracks how many resources
/// of each description are currently in use so that the same GPU resource is never
/// handed out twice within a graph, while still sharing resources between graphs
/// that happen to need the same description.
///
/// Released resources stay alive in the allocator for intra-graph recycling.
/// FreeResources (or the destructor) drops all shared pointers, decrementing
/// the ref counts back toward the pool.
class EZ_RENDERERCORE_DLL ezRenderGraphResourceAllocator
{
public:
  explicit ezRenderGraphResourceAllocator(ezRenderGraphResourcePool* pPool);
  ~ezRenderGraphResourceAllocator();

  ezRenderGraphResourceAllocator(const ezRenderGraphResourceAllocator&) = delete;
  ezRenderGraphResourceAllocator& operator=(const ezRenderGraphResourceAllocator&) = delete;
  ezRenderGraphResourceAllocator(ezRenderGraphResourceAllocator&& rhs) noexcept;
  ezRenderGraphResourceAllocator& operator=(ezRenderGraphResourceAllocator&& rhs) noexcept;

  /// Acquire a texture matching the given description. If a previously released
  /// texture with the same description is available, it is recycled. Otherwise, a
  /// new slot is requested from the pool.
  ezGALTextureHandle AcquireTexture(const ezGALTextureCreationDescription& desc);

  /// Return a texture for intra-graph recycling. The underlying pooled resource
  /// stays alive in the allocator; a future AcquireTexture with a matching
  /// description may return this same resource.
  void ReleaseTexture(ezGALTextureHandle hTexture);

  /// Acquire a buffer matching the given description.
  ezGALBufferHandle AcquireBuffer(const ezGALBufferCreationDescription& desc);

  /// Return a buffer for intra-graph recycling.
  void ReleaseBuffer(ezGALBufferHandle hBuffer);

  /// Drop all shared pointers, decrementing ref counts back toward the pool.
  void FreeResources();

private:
  ezRenderGraphResourcePool* m_pPool = nullptr;

  struct TextureGroup
  {
    ezUInt32 m_uiNextPoolIndex = 0;
    ezHybridArray<ezSharedPtr<ezPooledRenderTexture>, 1> m_All;
    ezHybridArray<ezSharedPtr<ezPooledRenderTexture>, 1> m_Available;
  };
  ezHashTable<ezUInt64, TextureGroup> m_TextureGroups; // keyed by desc hash
  ezHashTable<ezGALTextureHandle, ezSharedPtr<ezPooledRenderTexture>> m_HandleToTexture;

  struct BufferGroup
  {
    ezUInt32 m_uiNextPoolIndex = 0;
    ezHybridArray<ezSharedPtr<ezPooledRenderBuffer>, 1> m_All;
    ezHybridArray<ezSharedPtr<ezPooledRenderBuffer>, 1> m_Available;
  };
  ezHashTable<ezUInt64, BufferGroup> m_BufferGroups;
  ezHashTable<ezGALBufferHandle, ezSharedPtr<ezPooledRenderBuffer>> m_HandleToBuffer;

  static ezUInt64 ComputeDescHash(const ezGALTextureCreationDescription& desc);
  static ezUInt64 ComputeDescHash(const ezGALBufferCreationDescription& desc);
};
