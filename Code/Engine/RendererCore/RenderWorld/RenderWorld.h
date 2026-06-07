#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Declarations.h>

using ezRenderPipelineResourceHandle = ezTypedResourceHandle<class ezRenderPipelineResource>;

/// Event data for render data extraction phase.
struct ezRenderWorldExtractionEvent
{
  enum class Type
  {
    BeginExtraction,      ///< Fired before any view extraction begins.
    BeforeViewExtraction, ///< Fired before extracting data for a specific view.
    AfterViewExtraction,  ///< Fired after extracting data for a specific view.
    EndExtraction         ///< Fired after all views have been extracted.
  };

  Type m_Type;
  ezView* m_pView = nullptr;
  ezUInt64 m_uiFrameCounter = 0;
};

/// Event data for render execution phase.
struct ezRenderWorldRenderEvent
{
  enum class Type
  {
    BeginRender,             ///< Fired before rendering begins.
    BeforePipelineExecution, ///< Fired before executing a render pipeline.
    AfterPipelineExecution,  ///< Fired after executing a render pipeline.
    EndRender,               ///< Fired after rendering is complete.
  };

  Type m_Type;
  const ezRenderViewContext* m_pRenderViewContext = nullptr;
  ezUInt64 m_uiFrameCounter = 0;
};

/// Central hub for rendering operations and view management.
///
/// Manages views, render data extraction, and rendering execution. Handles double-buffering
/// of render data when multithreaded rendering is enabled. Provides events for hooking into
/// various stages of the rendering pipeline.
class EZ_RENDERERCORE_DLL ezRenderWorld
{
public:
  static ezViewHandle CreateView(const char* szName, ezView*& out_pView);
  static void DeleteView(const ezViewHandle& hView);

  static bool TryGetView(const ezViewHandle& hView, ezView*& out_pView);

  /// \brief Searches for an ezView with the desired usage hint or alternative usage hint.
  static ezView* GetViewByUsageHint(ezCameraUsageHint::Enum usageHint, ezCameraUsageHint::Enum alternativeUsageHint = ezCameraUsageHint::None, const ezWorld* pWorld = nullptr);

  static void AddMainView(const ezViewHandle& hView);
  static void RemoveMainView(const ezViewHandle& hView);
  static void ClearMainViews();
  static ezArrayPtr<ezViewHandle> GetMainViews();
  static bool IsRenderingScheduled();

  /// Caches render data for an object to avoid re-extraction if unchanged.
  ///
  /// Cached render data needs to be deleted/invalidated manually if any data changes. The dependency arrays carry per-category render-graph barrier dependencies that were recorded during extraction and are cached alongside the render data.
  static void CacheRenderData(const ezView& view, const ezGameObjectHandle& hOwnerObject, const ezComponentHandle& hOwnerComponent, ezUInt16 uiComponentVersion, ezArrayPtr<ezInternal::RenderDataCacheEntry> cacheEntries, ezArrayPtr<const ezTextureDependency> textureDependencies = {}, ezArrayPtr<const ezBufferDependency> bufferDependencies = {});

  /// Deletes all cached render data globally.
  static void DeleteAllCachedRenderData();

  /// Deletes cached render data for a specific component.
  static void DeleteCachedRenderData(const ezGameObjectHandle& hOwnerObject, const ezComponentHandle& hOwnerComponent);

  /// Deletes cached render data for a game object.
  static void DeleteCachedRenderDataForObject(const ezGameObject* pOwnerObject);

  /// Recursively deletes cached render data for a game object and all its children.
  static void DeleteCachedRenderDataForObjectRecursive(const ezGameObject* pOwnerObject);

  /// Resets the render data cache for a specific view.
  static void ResetRenderDataCache(ezView& ref_view);

  /// Retrieves cached render data if available and still valid.
  static ezArrayPtr<const ezInternal::RenderDataCacheEntry> GetCachedRenderData(const ezView& view, const ezGameObjectHandle& hOwner, ezUInt16 uiComponentVersion, ezArrayPtr<const ezTextureDependency>& out_textureDependencies, ezArrayPtr<const ezBufferDependency>& out_bufferDependencies);

  static void AddViewToRender(const ezViewHandle& hView);

  /// Declares that the given view needs the specified texture in the given resource state when its render graph executes. Must be called during extraction.
  /// Usually this function is called alongside ezRenderWorld::AddViewToRender to declare the required state of the output the view produced.
  static void AddViewDependency(const ezView& consumerView, ezGALTextureHandle hTexture, ezBitflags<ezGALResourceState> requiredState, ezBitflags<ezGALShaderStageFlags> stage = ezGALShaderStageFlags::Auto);

  /// Declares that the given view needs the specified buffer in the given resource state when its render graph executes. Must be called during extraction.
  /// Usually this function is called alongside ezRenderWorld::AddViewToRender to declare the required state of the output the view produced.
  static void AddViewDependency(const ezView& consumerView, ezGALBufferHandle hBuffer, ezBitflags<ezGALResourceState> requiredState, ezBitflags<ezGALShaderStageFlags> stage = ezGALShaderStageFlags::Auto);

  static void ExtractMainViews();

  static void Render(ezRenderContext* pRenderContext);

  static void BeginFrame();
  static void EndFrame();

  static ezEvent<ezView*, ezMutex> s_ViewCreatedEvent;
  static ezEvent<ezView*, ezMutex> s_ViewDeletedEvent;

  static const ezEvent<const ezRenderWorldExtractionEvent&, ezMutex>& GetExtractionEvent() { return s_ExtractionEvent; }
  static const ezEvent<const ezRenderWorldRenderEvent&, ezMutex>& GetRenderEvent() { return s_RenderEvent; }

  static bool GetUseMultithreadedRendering();

  /// \brief Resets the frame counter to zero. Only for test purposes !
  EZ_ALWAYS_INLINE static void ResetFrameCounter() { s_uiFrameCounter = 0; }

  EZ_ALWAYS_INLINE static ezUInt64 GetFrameCounter() { return s_uiFrameCounter; }

  EZ_FORCE_INLINE static ezUInt32 GetDataIndexForExtraction() { return GetUseMultithreadedRendering() ? (s_uiFrameCounter & 1) : 0; }

  EZ_FORCE_INLINE static ezUInt32 GetDataIndexForRendering() { return GetUseMultithreadedRendering() ? ((s_uiFrameCounter + 1) & 1) : 0; }

  static bool IsRenderingThread();

  /// \name Render To Texture
  /// @{
public:
  struct CameraConfig
  {
    ezRenderPipelineResourceHandle m_hRenderPipeline;
  };

  static void BeginModifyCameraConfigs();
  static void EndModifyCameraConfigs();
  static void ClearCameraConfigs();
  static void SetCameraConfig(const char* szName, const CameraConfig& config);
  static const CameraConfig* FindCameraConfig(const char* szName);

  static ezEvent<void*> s_CameraConfigsModifiedEvent;

private:
  static bool s_bModifyingCameraConfigs;
  static ezMap<ezString, CameraConfig> s_CameraConfigs;

  /// @}

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, RenderWorld);
  friend class ezView;
  friend class ezRenderPipeline;

  static void DeleteCachedRenderDataInternal(const ezGameObjectHandle& hOwnerObject);
  static void ClearRenderDataCache();
  static void UpdateRenderDataCache();

  static void AddRenderPipelineToRebuild(ezRenderPipeline* pRenderPipeline, const ezViewHandle& hView);
  static void RebuildPipelines();

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static ezEvent<const ezRenderWorldExtractionEvent&, ezMutex> s_ExtractionEvent;
  static ezEvent<const ezRenderWorldRenderEvent&, ezMutex> s_RenderEvent;
  static ezUInt64 s_uiFrameCounter;
};
