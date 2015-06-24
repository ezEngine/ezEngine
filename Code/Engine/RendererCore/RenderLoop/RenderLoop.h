#pragma once

#include <RendererCore/Declarations.h>
#include <Foundation/Threading/TaskSystem.h>

class ezView;
class ezRenderContext;

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

  static void AddViewToRender(ezView* pView);

  static void ExtractMainViews();

  static void Render(ezRenderContext* pRenderContext);

  static bool GetUseMultithreadedRendering();

  static void FinishFrame();
  EZ_FORCE_INLINE static ezUInt32 GetFrameCounter()
  {
    return s_uiFrameCounter;
  }


private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Graphics, RendererLoop);

  static void OnEngineShutdown();

  static ezUInt32 s_uiFrameCounter;

  static ezDynamicArray<ezView*> s_MainViews;
};

