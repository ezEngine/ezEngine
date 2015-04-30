#include <RendererCore/PCH.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Pipeline/View.h>

#include <Foundation/Configuration/CVar.h>

ezCVarBool CVarMultithreadedRendering("Renderer.Multithreading", true, ezCVarFlags::Default, "Enables multithreaded update and rendering");

ezUInt32 ezRenderLoop::s_uiFrameCounter;

bool ezRenderLoop::s_bInExtract;
ezTaskGroupID ezRenderLoop::s_ExtractionFinishedTaskID;

ezDynamicArray<ezView*> ezRenderLoop::s_MainViews;

ezMutex ezRenderLoop::s_ViewsToRenderMutex;
ezDynamicArray<ezView*> ezRenderLoop::s_ViewsToRender;

ezDynamicArray<ezView*> ezRenderLoop::s_FilteredViewsToRender[2];

ezView* ezRenderLoop::CreateView(const char* szName)
{
  ezView* pView = static_cast<ezView*>(ezFoundation::GetDefaultAllocator()->Allocate(sizeof(ezView), EZ_ALIGNMENT_OF(ezView)));
  ezMemoryUtils::Construct(pView, 1);

  pView->SetName(szName);
  return pView;
}

void ezRenderLoop::DeleteView(ezView* pView)
{
  
}

void ezRenderLoop::AddMainView(ezView* pView)
{
  EZ_ASSERT_DEV(!s_bInExtract, "Cannot add main view during extraction");

  s_MainViews.PushBack(pView);
}

void ezRenderLoop::AddMainViews(const ezArrayPtr<ezView*>& views)
{
  EZ_ASSERT_DEV(!s_bInExtract, "Cannot add main views during extraction");

  s_MainViews.PushBackRange(views);
}

void ezRenderLoop::RemoveMainView(ezView* pView)
{
  EZ_ASSERT_DEV(!s_bInExtract, "Cannot remove main view during extraction");

  s_MainViews.Remove(pView);
}

void ezRenderLoop::ClearMainViews()
{
  EZ_ASSERT_DEV(!s_bInExtract, "Cannot clear main views during extraction");

  s_MainViews.Clear();
}

void ezRenderLoop::AddViewToRender(ezView* pView)
{
  ezLock<ezMutex> lock(s_ViewsToRenderMutex);
  EZ_ASSERT_DEV(s_bInExtract, "Render views need to be collected during extraction");

  s_ViewsToRender.PushBack(pView);

  if (CVarMultithreadedRendering)
  {
    ezTaskGroupID extractTaskID = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);
    ezTaskSystem::AddTaskGroupDependency(s_ExtractionFinishedTaskID, extractTaskID);

    ezTaskSystem::AddTaskToGroup(extractTaskID, pView->GetExtractTask());

    ezTaskSystem::StartTaskGroup(extractTaskID);
  }
  else
  {
    pView->ExtractData();
  }
}

void ezRenderLoop::ExtractMainViews()
{
  EZ_ASSERT_DEV(!s_bInExtract, "ExtractMainViews must not be called from multiple threads.");

  s_bInExtract = true;

  if (CVarMultithreadedRendering)
  {
    s_ExtractionFinishedTaskID = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);

    ezTaskGroupID extractTaskID = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);
    ezTaskSystem::AddTaskGroupDependency(s_ExtractionFinishedTaskID, extractTaskID);

    {
      ezLock<ezMutex> lock(s_ViewsToRenderMutex);
      for (ezUInt32 i = 0; i < s_MainViews.GetCount(); ++i)
      {
        ezView* pView = s_MainViews[i];

        s_ViewsToRender.PushBack(pView);
        ezTaskSystem::AddTaskToGroup(extractTaskID, pView->GetExtractTask());
      }
    }    

    ezTaskSystem::StartTaskGroup(s_ExtractionFinishedTaskID);
    ezTaskSystem::StartTaskGroup(extractTaskID);

    ezTaskSystem::WaitForGroup(s_ExtractionFinishedTaskID);
  }
  else
  {
    for (ezUInt32 i = 0; i < s_MainViews.GetCount(); ++i)
    {
      s_ViewsToRender.PushBack(s_MainViews[i]);
      s_MainViews[i]->ExtractData();
    }
  }

  s_bInExtract = false;

  // filter out duplicates and reverse order so that dependent views are rendered first
  {
    ezDynamicArray<ezView*>& filteredViews = s_FilteredViewsToRender[s_uiFrameCounter & 1];
    filteredViews.Clear();

    for (ezUInt32 i = s_ViewsToRender.GetCount(); i-- > 0;)
    {
      ezView* pView = s_ViewsToRender[i];
      if (!filteredViews.Contains(pView))
      {
        filteredViews.PushBack(pView);
      }
    }

    s_ViewsToRender.Clear();
  }  
}

void ezRenderLoop::Render(ezRenderContext* pRenderContext)
{
  const ezUInt32 uiFrameCounter = s_uiFrameCounter + (CVarMultithreadedRendering ? 1 : 0);
  ezDynamicArray<ezView*>& filteredViews = s_FilteredViewsToRender[uiFrameCounter & 1];

  for (ezUInt32 i = 0; i < filteredViews.GetCount(); ++i)
  {
    filteredViews[i]->Render(pRenderContext);
  }
}

bool ezRenderLoop::GetUseMultithreadedRendering()
{
  return CVarMultithreadedRendering;
}

void ezRenderLoop::FinishFrame()
{
  ++s_uiFrameCounter;
}
