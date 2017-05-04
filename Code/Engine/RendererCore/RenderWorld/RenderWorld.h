#pragma once

#include <RendererCore/Pipeline/Declarations.h>

class EZ_RENDERERCORE_DLL ezRenderWorld
{
public:
  static ezViewHandle CreateView(const char* szName, ezView*& out_pView);
  static void DeleteView(const ezViewHandle& hView);

  static bool TryGetView(const ezViewHandle& hView, ezView*& out_pView);
  static ezView* GetViewByUsageHint(ezCameraUsageHint::Enum usageHint);

  static void AddMainView(const ezViewHandle& hView);
  static void RemoveMainView(const ezViewHandle& hView);
  static void ClearMainViews();
  static ezArrayPtr<ezViewHandle> GetMainViews();

  static void AddViewToRender(const ezViewHandle& hView);

  static void ExtractMainViews();

  static void Render(ezRenderContext* pRenderContext);

  static void BeginFrame();
  static void EndFrame();

  static ezEvent<ezView*> s_ViewCreatedEvent;
  static ezEvent<ezView*> s_ViewDeletedEvent;
  static ezEvent<ezUInt64> s_BeginFrameEvent; ///< Triggered at the end of BeginFrame.
  static ezEvent<ezUInt64> s_EndFrameEvent; ///< Triggered at the beginning of EndFrame before the frame counter is incremented.

  static bool GetUseMultithreadedRendering();

  EZ_FORCE_INLINE static ezUInt64 GetFrameCounter()
  {
    return s_uiFrameCounter;
  }

  EZ_FORCE_INLINE static ezUInt32 GetDataIndexForExtraction()
  {
    return GetUseMultithreadedRendering() ? (s_uiFrameCounter & 1) : 0;
  }

  EZ_FORCE_INLINE static ezUInt32 GetDataIndexForRendering()
  {
    return GetUseMultithreadedRendering() ? ((s_uiFrameCounter + 1) & 1) : 0;
  }

  static bool IsRenderingThread();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, RendererLoop);
  friend class ezView;

  static void AddRenderPipelineToRebuild(ezRenderPipeline* pRenderPipeline, const ezViewHandle& hView);

  static void OnEngineShutdown();

  static ezUInt64 s_uiFrameCounter;
};

