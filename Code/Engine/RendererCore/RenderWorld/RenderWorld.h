#pragma once

#include <RendererCore/Pipeline/Declarations.h>
#include <Core/ResourceManager/ResourceHandle.h>

typedef ezTypedResourceHandle<class ezRenderPipelineResource> ezRenderPipelineResourceHandle;

class EZ_RENDERERCORE_DLL ezRenderWorld
{
public:
  static ezViewHandle CreateView(const char* szName, ezView*& out_pView);
  static void DeleteView(const ezViewHandle& hView);

  static bool TryGetView(const ezViewHandle& hView, ezView*& out_pView);
  static ezView* GetViewByUsageHint(ezCameraUsageHint::Enum usageHint, ezCameraUsageHint::Enum alternativeUsageHint = ezCameraUsageHint::None);

  static void AddMainView(const ezViewHandle& hView);
  static void RemoveMainView(const ezViewHandle& hView);
  static void ClearMainViews();
  static ezArrayPtr<ezViewHandle> GetMainViews();

  static void CacheRenderData(const ezView& view, const ezGameObjectHandle& hOwnerObject, const ezComponentHandle& hOwnerComponent,
                              ezArrayPtr<ezInternal::RenderDataCacheEntry> cacheEntries);

  static void DeleteAllCachedRenderData();
  static void DeleteCachedRenderData(const ezGameObjectHandle& hOwnerObject, const ezComponentHandle& hOwnerComponent);
  static void DeleteCachedRenderDataRecursive(const ezGameObject* pOwnerObject);
  static void DeleteCachedRenderData(ezView& view);
  static ezArrayPtr<ezInternal::RenderDataCacheEntry> GetCachedRenderData(const ezView& view, const ezGameObjectHandle& hOwner);

  static void AddViewToRender(const ezViewHandle& hView);

  static void ExtractMainViews();

  static void Render(ezRenderContext* pRenderContext);

  static void BeginFrame();
  static void EndFrame();

  static ezEvent<ezView*> s_ViewCreatedEvent;
  static ezEvent<ezView*> s_ViewDeletedEvent;

  static ezEvent<ezUInt64> s_BeginExtractionEvent;
  static ezEvent<ezUInt64> s_EndExtractionEvent;

  static ezEvent<ezUInt64> s_BeginRenderEvent;
  static ezEvent<ezUInt64> s_EndRenderEvent;

  static bool GetUseMultithreadedRendering();

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

  static void ClearRenderDataCache();
  static void UpdateRenderDataCache();

  static void AddRenderPipelineToRebuild(ezRenderPipeline* pRenderPipeline, const ezViewHandle& hView);
  static void RebuildPipelines();

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static ezUInt64 s_uiFrameCounter;
};

