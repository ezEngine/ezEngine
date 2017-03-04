#pragma once

#include <RendererCore/Pipeline/Declarations.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/UniquePtr.h>

class ezView;
class ezRenderContext;
class ezRenderPipeline;

class EZ_RENDERERCORE_DLL ezRenderLoop
{
public:
  static ezView* CreateView(const char* szName);
  static void DeleteView(ezView* pView);
  static void AddMainView(ezView* pView);
  static void AddMainViews(const ezArrayPtr<ezView*>& views);
  static void RemoveMainView(ezView* pView);
  static void ClearMainViews();

  EZ_FORCE_INLINE static ezArrayPtr<ezView*> GetMainViews()
  {
    return s_MainViews;
  }

  EZ_FORCE_INLINE static ezArrayPtr<ezView*> GetAllViews()
  {
    return s_Views;
  }

  static ezView* GetViewByUsageHint(ezCameraUsageHint::Enum usageHint);

  static void AddViewToRender(ezView* pView);

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

  static void AddRenderPipelineToRebuild(ezRenderPipeline* pRenderPipeline, ezView* pView);

  static void OnEngineShutdown();

  static ezUInt64 s_uiFrameCounter;

  static ezDynamicArray<ezView*> s_Views;
  static ezDynamicArray<ezView*> s_MainViews;
};

