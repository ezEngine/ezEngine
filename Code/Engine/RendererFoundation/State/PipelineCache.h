#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/Mutex.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezGALDevice;

/// \brief A cache from pipeline descriptor to handle which holds a reference to each pipeline that is never freed until shutdown. This is just a stopgap solution until the high level interface changes and mostly used by `ezRenderContext` to provide the old interface until further refactoring.
class EZ_RENDERERFOUNDATION_DLL ezGALPipelineCache
{
  EZ_DECLARE_SINGLETON(ezGALPipelineCache);

public:
  /// \brief Creates a pipeline or retrieves it from the cache. Ownership remains with the cache so do not call DestroyGraphicsPipeline on the handle.
  static ezGALGraphicsPipelineHandle GetPipeline(const ezGALGraphicsPipelineCreationDescription& description);
  /// \brief Creates a pipeline or retrieves it from the cache. Ownership remains with the cache so do not call DestroyComputePipeline on the handle.
  static ezGALComputePipelineHandle GetPipeline(const ezGALComputePipelineCreationDescription& description);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererFoundation, PipelineCache);
  friend class ezMemoryUtils;

  struct GraphicsPipelineCacheKey
  {
    EZ_DECLARE_POD_TYPE();
    ezUInt32 m_uiHash = 0;
    ezGALGraphicsPipelineCreationDescription m_Desc;
  };

  struct ComputePipelineCacheKey
  {
    EZ_DECLARE_POD_TYPE();
    ezUInt32 m_uiHash = 0;
    ezGALComputePipelineCreationDescription m_Desc;
  };

  struct CacheKeyHasher
  {
    static ezUInt32 Hash(const GraphicsPipelineCacheKey& a);
    static bool Equal(const GraphicsPipelineCacheKey& a, const GraphicsPipelineCacheKey& b);

    static ezUInt32 Hash(const ComputePipelineCacheKey& a);
    static bool Equal(const ComputePipelineCacheKey& a, const ComputePipelineCacheKey& b);
  };

private:
  ezGALPipelineCache();
  ~ezGALPipelineCache();
  void GALDeviceEventHandler(const ezGALDeviceEvent& e);
  void Clear();

  template <typename HandleType, typename DescType, typename KeyType>
  EZ_ALWAYS_INLINE HandleType TryGetPipeline(const DescType& description, ezHashTable<KeyType, HandleType, CacheKeyHasher>& table);

  template <typename HandleType, typename DescType, typename KeyType>
  EZ_ALWAYS_INLINE ezResult TryInsertPipeline(const DescType& description, HandleType hNewPipeline, ezHashTable<KeyType, HandleType, CacheKeyHasher>& table);

private:
  ezMutex m_Mutex;
  ezGALDevice* m_pDevice = nullptr;
  ezHashTable<GraphicsPipelineCacheKey, ezGALGraphicsPipelineHandle, CacheKeyHasher> m_GraphicsPipelines;
  ezHashTable<ComputePipelineCacheKey, ezGALComputePipelineHandle, CacheKeyHasher> m_ComputePipelines;
};

#include <RendererFoundation/State/Implementation/PipelineCache_inl.h>
