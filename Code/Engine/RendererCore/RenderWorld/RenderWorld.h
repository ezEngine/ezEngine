#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Declarations.h>

using ezRenderPipelineResourceHandle = ezTypedResourceHandle<class ezRenderPipelineResource>;

struct ezRenderWorldExtractionEvent
{
  enum class Type
  {
    BeginExtraction,
    BeforeViewExtraction,
    AfterViewExtraction,
    EndExtraction
  };

  Type m_Type;
  ezView* m_pView = nullptr;
  ezUInt64 m_uiFrameCounter = 0;
};

struct ezRenderWorldRenderEvent
{
  enum class Type
  {
    BeginRender,
    BeforePipelineExecution,
    AfterPipelineExecution,
    EndRender,
  };

  Type m_Type;
  ezRenderPipeline* m_pPipeline = nullptr;
  const ezRenderViewContext* m_pRenderViewContext = nullptr;
  ezUInt64 m_uiFrameCounter = 0;
};

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

  static void CacheRenderData(const ezView& view, const ezGameObjectHandle& hOwnerObject, const ezComponentHandle& hOwnerComponent, ezUInt16 uiComponentVersion, ezArrayPtr<ezInternal::RenderDataCacheEntry> cacheEntries);

  static void DeleteAllCachedRenderData();
  static void DeleteCachedRenderData(const ezGameObjectHandle& hOwnerObject, const ezComponentHandle& hOwnerComponent);
  static void DeleteCachedRenderDataForObject(const ezGameObject* pOwnerObject);
  static void DeleteCachedRenderDataForObjectRecursive(const ezGameObject* pOwnerObject);
  static void ResetRenderDataCache(ezView& ref_view);
  static ezArrayPtr<const ezInternal::RenderDataCacheEntry> GetCachedRenderData(const ezView& view, const ezGameObjectHandle& hOwner, ezUInt16 uiComponentVersion);

  static void AddViewToRender(const ezViewHandle& hView);

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
