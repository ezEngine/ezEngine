#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/Status.h>
#include <RendererCore/RenderGraph/Declarations.h>
#include <RendererCore/RenderGraph/RenderGraphPassBuilder.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class ezGALCommandEncoder;
class ezGALDevice;
class ezRenderGraphPassObserver;
class ezRenderGraphResourcePool;
class ezRenderGraphResourceAllocator;
class ezGALResourceStateTracker;
struct ezRenderGraphInspectionInfo;

/// Render graph usable for a single frame. Contains passes that declare transient resource dependencies. After all passes are added, the graph is compiled to cull unused passes, allocate and alias transient resources, and compute barrier placement, then executed. Passes are always executed in declaration order.
///
/// The graph object is designed to persist across the lifetime of the program. Every frame you want to use the graph, call Reset() before re-declaring passes. Note that only one ezRenderGraphPassBuilder can exist at any time so calls to `Add*Pass` must be scoped individually. Finally, call ezRenderGraphManager::EnqueueRenderGraph to submit the graph for execution this frame.
class EZ_RENDERERCORE_DLL ezRenderGraph : public ezRefCounted
{
public:
  ~ezRenderGraph();

  ezGALDevice* GetDevice() const { return m_pDevice; }

  /// Returns the immutable name assigned at creation.
  const ezString& GetGraphName() const { return m_sGraphName; }

  /// Sets the user name that can be changed at any time.
  void SetUserName(ezStringView sName) { m_sUserName = sName; }
  const ezString& GetUserName() const { return m_sUserName; }

  /// User data is accessible during execution callbacks via `ctx.GetUserData<T>()`.
  void SetUserData(ezReflectedClass* pUserData) { m_pUserData = pUserData; }
  ezReflectedClass* GetUserData() const { return m_pUserData; }

  /// \name Transient Resource Creation
  ///@{

  /// Create a transient texture. The GPU resource is allocated or aliased from a pool during compilation. There is no need to set any flags on the texture. Whenever a pass operates on a texture its flags are extended to support that operation.
  ezRenderGraphTextureHandle CreateTexture(const ezGALTextureCreationDescription& desc);

  /// Create a transient buffer. The GPU resource is allocated or aliased from a pool during compilation. There is no need to set any flags on the buffer. Whenever a pass operates on a buffer its flags are extended to support that operation.
  ezRenderGraphBufferHandle CreateBuffer(const ezGALBufferCreationDescription& desc);

  ///@}
  /// \name Resource Queries
  ///@{

  /// Returns the creation description of a transient or imported texture. Note that this reference is not stable and can become invalid when new textures are added to the graph so it is best to store a copy of the returned value.
  const ezGALTextureCreationDescription& GetTextureDesc(ezRenderGraphTextureHandle hTexture) const;

  /// Returns the creation description of a transient or imported buffer. Note that this reference is not stable and can become invalid when new buffers are added to the graph so it is best to store a copy of the returned value.
  const ezGALBufferCreationDescription& GetBufferDesc(ezRenderGraphBufferHandle hBuffer) const;

  ///@}
  /// \name Import External Resources
  ///@{

  /// Import an existing GPU texture into the graph, returning a graph handle so it can be used inside the graph. You can optionally set `access` and `stage` to force a barrier on the imported resource at the start of the graph.
  ezRenderGraphTextureHandle ImportTexture(ezGALTextureHandle hTexture, ezBitflags<ezGALResourceState> access = ezGALResourceState::Unknown,
    ezBitflags<ezGALShaderStageFlags> stage = ezGALShaderStageFlags::Auto);

  /// Import an existing GPU buffer into the graph, returning a graph handle so it can be used inside the graph. You can optionally set `access` and `stage` to force a barrier on the imported resource at the start of the graph.
  ezRenderGraphBufferHandle ImportBuffer(ezGALBufferHandle hBuffer, ezBitflags<ezGALResourceState> access = ezGALResourceState::Unknown,
    ezBitflags<ezGALShaderStageFlags> stage = ezGALShaderStageFlags::Auto);

  /// Use to replace a graph texture with an actual GPU resource. The `hNewTexture` must not be already imported into the graph. Mainly used when graphs are built greedily and the target swachchain image is not clear at which point it fints into the pipeline. Thus, the graph can be built entirely with transient graph textures and only at the end the replacement can be done. If this call failed the given texture does not fulfil the requirements made to the graph texture.
  ezStatus ReplaceImportedTexture(ezRenderGraphTextureHandle hGraphTexture, ezGALTextureHandle hNewTexture);

  ///@}
  /// \name Pass Creation
  ///@{

  /// Add a graphics pass. Returns a builder through which resource accesses and the execution callback must be declared before compilation.
  ezRenderGraphPassBuilder AddGraphicsPass(ezStringView sName);

  /// Add a compute pass.
  ezRenderGraphPassBuilder AddComputePass(ezStringView sName);

  /// Add a transfer pass.
  ezRenderGraphPassBuilder AddTransferPass(ezStringView sName);

  /// Insert a debug marker that will be pushed during execution at the/ current recording position (before the next pass).
  void PushMarker(ezStringView sMarker);

  /// Insert a pop-marker at the current recording position.
  void PopMarker();

  ///@}
  /// \name Compilation & Execution
  ///@{

  /// Clear all declared passes and resources so the graph can be rebuilt. This is done automatically for enqueued graphs at the end of each frame, but should be called regardless every time you intent to record into the graph.
  void Reset();

  /// Checks that all imported textures and buffers still exist on the device. Call before registering the graph each frame to detect stale resources.
  ezResult ValidateImportedResources() const;

  /// Compile the graph: cull unused passes, allocate and alias transient resources. After this call, no new passes can be added until the graph is reset.
  ezResult Compile();

  /// Compute texture and buffer barriers for each pass based on resource state tracking.
  void ComputeBarriers(ezGALResourceStateTracker& ref_tracker, ezArrayPtr<ezRenderGraphPassObserver*> observers = {});

  /// Execute all passes in compiled order.
  void Execute(ezRenderGraphContext& ref_ctx, ezArrayPtr<ezRenderGraphPassObserver*> observers = {});

private:
  friend class ezRenderGraphManager;
  friend class ezRenderGraphPassBuilder;

  ezRenderGraph(ezGALDevice* pDevice, ezStringView sName, ezEnum<ezRenderGraphPhase> phase);

  void StartPassBuilder(ezEnum<ezGALQueueType> queueType, ezStringView sName);
  void EndPassBuilder();

  ezResult GetInspectionInfo(ezRenderGraphInspectionInfo& out_info) const;

  /// Find a pass by name and return its original index. Returns s_Unused if not found.
  ezUInt16 FindPassByName(ezStringView sName) const;

  /// Find the texture handle used in the Nth texture access of the given pass.
  ezRenderGraphTextureHandle FindTextureAccessInPass(ezUInt16 uiOriginalPassIndex, ezUInt16 uiAccessIndex) const;

private:
  static constexpr ezUInt16 s_Unused = 0xFFFF;

  enum class RenderGraphState
  {
    Recording,       // New passes can be added, initial state
    Enqueued,        // No new passes can be added.
    Compiled,        // BuildDependencyGraph, CullDeadPasses, BuildSortedPassList, ComputeResourceLifetimes, AllocateTransientResources, BuildRenderingSetups
    BarriersCreated, // ComputeBarriers
  };

  struct TextureInfo
  {
    ezRenderGraphTextureHandle m_hTexture;
    ezGALTextureRange m_range;
    ezBitflags<ezGALShaderStageFlags> m_stage;
    ezBitflags<ezGALResourceState> m_access;
  };

  struct BufferInfo
  {
    ezRenderGraphBufferHandle m_hBuffer;
    ezBitflags<ezGALShaderStageFlags> m_stage;
    ezBitflags<ezGALResourceState> m_access;
  };

  struct ColorTargetInfo
  {
    ezRenderGraphTextureHandle m_hTexture;
    ezGALRenderTargetRange m_range;
    ezEnum<ezGALRenderTargetLoadOp> m_loadOp;
    ezEnum<ezGALRenderTargetStoreOp> m_storeOp;
    ezEnum<ezGALResourceFormat> m_overrideViewFormat;
    ezEnum<ezGALTextureType> m_overrideViewType;
  };

  struct DepthStencilTargetInfo
  {
    ezRenderGraphTextureHandle m_hTexture;
    ezGALRenderTargetRange m_range;
    ezEnum<ezGALRenderTargetLoadOp> m_depthLoadOp;
    ezEnum<ezGALRenderTargetStoreOp> m_depthStoreOp;
    ezEnum<ezGALRenderTargetLoadOp> m_stencilLoadOp;
    ezEnum<ezGALRenderTargetStoreOp> m_stencilStoreOp;
    bool m_bReadOnly = false;
  };

  struct DepthStencilTargetClearInfo
  {
    float fDepthClear = 1.0f;
    ezUInt8 uiStencilClear = 0;
  };

  struct ImportedTexture
  {
    ezGALTextureHandle m_hTextureHandle;
    ezBitflags<ezGALResourceState> m_access;
    ezBitflags<ezGALShaderStageFlags> m_stage;
  };
  struct ImportedBuffer
  {
    ezGALBufferHandle m_hBufferHandle;
    ezBitflags<ezGALResourceState> m_access;
    ezBitflags<ezGALShaderStageFlags> m_stage;
  };

  struct Pass
  {
    EZ_ALWAYS_INLINE ezArrayPtr<const TextureInfo> GetReadTextures(const ezRenderGraph* pGraph) const
    {
      return ezMakeArrayPtr(pGraph->m_ReadTextures.GetData() + m_uiReadTextureIndex, m_uiReadTextureCount);
    }
    EZ_ALWAYS_INLINE ezArrayPtr<const TextureInfo> GetWriteTextures(const ezRenderGraph* pGraph) const
    {
      return ezMakeArrayPtr(pGraph->m_WriteTextures.GetData() + m_uiWriteTextureIndex, m_uiWriteTextureCount);
    }
    EZ_ALWAYS_INLINE ezArrayPtr<const BufferInfo> GetReadBuffers(const ezRenderGraph* pGraph) const
    {
      return ezMakeArrayPtr(pGraph->m_ReadBuffers.GetData() + m_uiReadBufferIndex, m_uiReadBufferCount);
    }
    EZ_ALWAYS_INLINE ezArrayPtr<const BufferInfo> GetWriteBuffers(const ezRenderGraph* pGraph) const
    {
      return ezMakeArrayPtr(pGraph->m_WriteBuffers.GetData() + m_uiWriteBufferIndex, m_uiWriteBufferCount);
    }
    EZ_ALWAYS_INLINE ezArrayPtr<const ColorTargetInfo> GetColorTargets(const ezRenderGraph* pGraph) const
    {
      return ezMakeArrayPtr(pGraph->m_ColorTargets.GetData() + m_uiColorTargetIndex, m_uiColorTargetCount);
    }
    EZ_ALWAYS_INLINE ezArrayPtr<const DepthStencilTargetInfo> GetDepthStencilTargets(const ezRenderGraph* pGraph) const
    {
      return ezMakeArrayPtr(pGraph->m_DepthStencilTargets.GetData() + m_uiDepthStencilTargetIndex, m_uiDepthStencilTargetCount);
    }
    EZ_ALWAYS_INLINE ezArrayPtr<const ezColor> GetClearColors(const ezRenderGraph* pGraph) const
    {
      return ezMakeArrayPtr(pGraph->m_ClearColors.GetData() + m_uiClearColorIndex, m_uiClearColorCount);
    }
    EZ_ALWAYS_INLINE ezArrayPtr<const DepthStencilTargetClearInfo> GetClearDepthStencils(const ezRenderGraph* pGraph) const
    {
      return ezMakeArrayPtr(pGraph->m_ClearDepthStencils.GetData() + m_uiDepthStencilClearIndex, m_uiDepthStencilClearCount);
    }

    /// Returns the adjacency slice for this pass from the flat storage. Populated during BuildDependencyGraph.
    EZ_ALWAYS_INLINE ezArrayPtr<const ezUInt16> GetAdjacency(const ezRenderGraph* pGraph) const
    {
      return ezMakeArrayPtr(pGraph->m_AdjacencyStorage.GetData() + m_uiAdjacencyIndex, m_uiAdjacencyCount);
    }

    ezUInt16 m_uiReadTextureIndex = 0;
    ezUInt16 m_uiWriteTextureIndex = 0;
    ezUInt16 m_uiReadBufferIndex = 0;
    ezUInt16 m_uiWriteBufferIndex = 0;
    ezUInt16 m_uiColorTargetIndex = 0;
    ezUInt16 m_uiDepthStencilTargetIndex = 0;
    ezUInt16 m_uiClearColorIndex = 0;
    ezUInt16 m_uiDepthStencilClearIndex = 0;
    ezUInt16 m_uiAdjacencyIndex = 0;

    ezUInt8 m_uiReadTextureCount = 0;
    ezUInt8 m_uiWriteTextureCount = 0;
    ezUInt8 m_uiReadBufferCount = 0;
    ezUInt8 m_uiWriteBufferCount = 0;
    ezUInt8 m_uiColorTargetCount = 0;
    ezUInt8 m_uiDepthStencilTargetCount = 0;
    ezUInt8 m_uiClearColorCount = 0;
    ezUInt8 m_uiDepthStencilClearCount = 0;
    ezUInt8 m_uiAdjacencyCount = 0;
    ezRenderGraphExecuteFunction m_ExecuteFunction;
    bool m_bHasSideEffects = false;
    bool m_bStereoscopic = false;
    ezEnum<ezGALQueueType> m_QueueType;
  };

  struct IntermediatePass
  {
    EZ_ALWAYS_INLINE ezArrayPtr<const ezUInt16> GetAcquireTextures(const ezRenderGraph* pGraph) const
    {
      return ezMakeArrayPtr(pGraph->m_AcquireTextures.GetData() + m_uiAcquireTextureIndex, m_uiAcquireTextureCount);
    }
    EZ_ALWAYS_INLINE ezArrayPtr<const ezUInt16> GetReleaseTextures(const ezRenderGraph* pGraph) const
    {
      return ezMakeArrayPtr(pGraph->m_ReleaseTextures.GetData() + m_uiReleaseTextureIndex, m_uiReleaseTextureCount);
    }
    EZ_ALWAYS_INLINE ezArrayPtr<const ezUInt16> GetAcquireBuffers(const ezRenderGraph* pGraph) const
    {
      return ezMakeArrayPtr(pGraph->m_AcquireBuffers.GetData() + m_uiAcquireBufferIndex, m_uiAcquireBufferCount);
    }
    EZ_ALWAYS_INLINE ezArrayPtr<const ezUInt16> GetReleaseBuffers(const ezRenderGraph* pGraph) const
    {
      return ezMakeArrayPtr(pGraph->m_ReleaseBuffers.GetData() + m_uiReleaseBufferIndex, m_uiReleaseBufferCount);
    }

    ezUInt16 uiOriginalPassIndex = 0;
    ezUInt8 m_uiAcquireTextureCount = 0;
    ezUInt8 m_uiReleaseTextureCount = 0;
    ezUInt8 m_uiAcquireBufferCount = 0;
    ezUInt8 m_uiReleaseBufferCount = 0;

    ezUInt16 m_uiAcquireTextureIndex = 0;
    ezUInt16 m_uiReleaseTextureIndex = 0;
    ezUInt16 m_uiAcquireBufferIndex = 0;
    ezUInt16 m_uiReleaseBufferIndex = 0;
  };

  struct CompiledPass
  {
    EZ_ALWAYS_INLINE ezArrayPtr<const ezGALTextureBarrier> GetTextureBarriers(const ezRenderGraph* pGraph) const
    {
      return ezMakeArrayPtr(pGraph->m_CompiledTextureBarriers.GetData() + m_uiTextureBarrierIndex, m_uiTextureBarrierCount);
    }
    EZ_ALWAYS_INLINE ezArrayPtr<const ezGALBufferBarrier> GetBufferBarriers(const ezRenderGraph* pGraph) const
    {
      return ezMakeArrayPtr(pGraph->m_CompiledBufferBarriers.GetData() + m_uiBufferBarrierIndex, m_uiBufferBarrierCount);
    }

    ezUInt16 m_uiOriginalPassIndex = 0;
    ezUInt32 m_uiTextureBarrierIndex = 0;
    ezUInt32 m_uiBufferBarrierIndex = 0;
    ezUInt16 m_uiTextureBarrierCount = 0;
    ezUInt16 m_uiBufferBarrierCount = 0;
    ezGALRenderingSetup m_RenderingSetup;
  };

private:
  void ResetInternal(RenderGraphState renderGraphState);

  // --- Compilation ---

  /// Validate that all passes have execute-callbacks and that all referenced handles are valid.
  ezResult ValidateGraph();

  /// Build per-pass dependency adjacency lists from resource read/write patterns.
  void BuildDependencyGraph();

  /// Remove passes that do not contribute to any side-effecting output.
  void CullDeadPasses();

  /// Build the sorted pass list from alive passes in declaration order.
  void BuildSortedPassList();

  /// Determine the first and last use (in sorted order) of each transient resource.
  void ComputeResourceLifetimes();

  /// Allocate concrete GPU resources for transient handles using the resource pool.
  void AllocateTransientResources();

  /// Build ezGALRenderingSetup for each graphics pass from its declared color
  /// and depth-stencil targets.
  void BuildRenderingSetups();

private:
  ezGALDevice* m_pDevice = nullptr;
  ezUniquePtr<ezRenderGraphResourceAllocator> m_pAllocator;
  RenderGraphState m_RenderGraphState = RenderGraphState::Recording;
  ezString m_sUserName;
  ezString m_sGraphName;
  ezEnum<ezRenderGraphPhase> m_Phase;
  ezReflectedClass* m_pUserData = nullptr;

  /// \name Recording State
  ///@{

  Pass* m_pCurrentPass = nullptr;
  ezDynamicArray<ezString> m_PassNames;
  ezHashTable<ezString, ezUInt32> m_UniquePassNames;
  ezDynamicArray<Pass> m_Passes;
  // Storage heaps for pass data
  ezDynamicArray<TextureInfo> m_ReadTextures;
  ezDynamicArray<TextureInfo> m_WriteTextures;
  ezDynamicArray<BufferInfo> m_ReadBuffers;
  ezDynamicArray<BufferInfo> m_WriteBuffers;
  ezDynamicArray<ColorTargetInfo> m_ColorTargets;
  ezDynamicArray<DepthStencilTargetInfo> m_DepthStencilTargets;
  ezDynamicArray<ezColor> m_ClearColors;
  ezDynamicArray<DepthStencilTargetClearInfo> m_ClearDepthStencils;

  struct MarkerEvent
  {
    ezUInt16 m_uiPassIndex; ///< Emitted before this pass index.
    bool m_bPush;           ///< true = PushMarker, false = PopMarker.
    ezString m_sName;       ///< Only used for push.
  };
  ezDynamicArray<MarkerEvent> m_MarkerEvents;

  // Create / Import Texture / Buffer result
  ezDynamicArray<ezGALTextureCreationDescription> m_TextureCreationDescriptions;
  ezDynamicArray<ezGALBufferCreationDescription> m_BufferCreationDescriptions;

  // Imported textures / Buffers
  ezMap<ezRenderGraphTextureHandle, ImportedTexture> m_HandleToImportTexture;
  ezMap<ezGALTextureHandle, ezRenderGraphTextureHandle> m_ImportTextureToHandle;
  ezMap<ezRenderGraphBufferHandle, ImportedBuffer> m_HandleToImportBuffer;
  ezMap<ezGALBufferHandle, ezRenderGraphBufferHandle> m_ImportBufferToHandle;

  ///@}
  /// \name Intermediate State
  ///@{

  // BuildDependencyGraph (pass index to...)
  ezDynamicArray<ezUInt16> m_AdjacencyStorage;
  // CullDeadPasses (pass index to...)
  ezDynamicArray<bool> m_Alive;

  // BuildSortedPassList (list of pass indices)
  ezDynamicArray<IntermediatePass> m_AlivePasses;

  // ComputeResourceLifetimes (texture / buffer index to sorted pass index)
  ezDynamicArray<ezUInt16> m_TextureFirstUse;
  ezDynamicArray<ezUInt16> m_TextureLastUse;
  ezDynamicArray<ezUInt16> m_BufferFirstUse;
  ezDynamicArray<ezUInt16> m_BufferLastUse;
  // Heap storage for acquire / release calls
  ezDynamicArray<ezUInt16> m_AcquireTextures;
  ezDynamicArray<ezUInt16> m_ReleaseTextures;
  ezDynamicArray<ezUInt16> m_AcquireBuffers;
  ezDynamicArray<ezUInt16> m_ReleaseBuffers;

  // AllocateTransientResources (texture / buffer index to resolved index)
  ezDynamicArray<ezUInt16> m_TextureToResolvedTexture;
  ezDynamicArray<ezUInt16> m_BufferToResolvedBuffer;
  /// List of resolved resources. Barriers will happen on this array. Size will be less or equal to handle count (aliasing of resources).
  ezDynamicArray<ezGALTextureHandle> m_ResolvedTextures;
  ezDynamicArray<ezGALBufferHandle> m_ResolvedBuffers;

  ///@}
  /// \name Compiled state
  ///@{

  // BuildRenderingSetups
  ezDynamicArray<CompiledPass> m_CompiledPasses;

  ///@}
  /// \name BarriersCreated state
  ///@{

  // ComputeBarriers
  ezDynamicArray<ezGALTextureBarrier> m_CompiledTextureBarriers;
  ezDynamicArray<ezGALBufferBarrier> m_CompiledBufferBarriers;

  ///@}
};
