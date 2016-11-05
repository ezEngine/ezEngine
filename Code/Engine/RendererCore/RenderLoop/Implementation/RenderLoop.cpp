#include <RendererCore/PCH.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>

#include <Foundation/Configuration/CVar.h>

ezCVarBool CVarMultithreadedRendering("r_Multithreading", true, ezCVarFlags::Default, "Enables multi-threaded update and rendering");

ezEvent<ezUInt64> ezRenderLoop::s_BeginFrameEvent;
ezEvent<ezUInt64> ezRenderLoop::s_EndFrameEvent;

ezUInt64 ezRenderLoop::s_uiFrameCounter;
ezDynamicArray<ezView*> ezRenderLoop::s_Views;
ezDynamicArray<ezView*> ezRenderLoop::s_MainViews;

namespace
{
  static bool s_bInExtract;
  static ezTaskGroupID s_ExtractionFinishedTaskID;
  static ezThreadID s_RenderingThreadID;

  static ezMutex s_ViewsToRenderMutex;
  static ezDynamicArray<ezView*> s_ViewsToRender;

  static ezDynamicArray<ezSharedPtr<ezRenderPipeline>> s_FilteredRenderPipelines[2];

  struct PipelineToRebuild
  {
    EZ_DECLARE_POD_TYPE();

    ezRenderPipeline* m_pPipeline;
    ezView* m_pView;
  };
  
  static ezMutex s_PipelinesToRebuildMutex;
  static ezDynamicArray<PipelineToRebuild> s_PipelinesToRebuild;
}

EZ_BEGIN_SUBSYSTEM_DECLARATION(Graphics, RendererLoop)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_ENGINE_SHUTDOWN
  {
    ezRenderLoop::OnEngineShutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION

ezView* ezRenderLoop::CreateView(const char* szName)
{
  // can't use EZ_DEFAULT_NEW because it wants to create a destructor function which is not possible since the view destructor is private
  ezView* pView = new (ezFoundation::GetDefaultAllocator()->Allocate(sizeof(ezView), EZ_ALIGNMENT_OF(ezView))) ezView();
  
  pView->SetName(szName);
  pView->InitializePins();

  s_Views.PushBack(pView);

  return pView;
}

void ezRenderLoop::DeleteView(ezView* pView)
{
  {
    EZ_LOCK(s_PipelinesToRebuildMutex);

    for (ezUInt32 i = s_PipelinesToRebuild.GetCount(); i-- > 0;)
    {
      if (s_PipelinesToRebuild[i].m_pView == pView)
      {
        s_PipelinesToRebuild.RemoveAt(i);
      }
    }
  }

  RemoveMainView(pView);
  s_Views.Remove(pView);
  
  pView->~ezView();
  ezFoundation::GetDefaultAllocator()->Deallocate(pView);
}

void ezRenderLoop::AddMainView(ezView* pView)
{
  EZ_ASSERT_DEV(!s_bInExtract, "Cannot add main view during extraction");

  if (!s_MainViews.Contains(pView))
    s_MainViews.PushBack(pView);
}

void ezRenderLoop::AddMainViews(const ezArrayPtr<ezView*>& views)
{
  EZ_ASSERT_DEV(!s_bInExtract, "Cannot add main views during extraction");

  for (auto pView : views)
  {
    if (!s_MainViews.Contains(pView))
      s_MainViews.PushBack(pView);
  }
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

ezView* ezRenderLoop::GetViewByUsageHint(ezCameraUsageHint::Enum usageHint)
{
  for (auto pView : s_Views)
  {
    if (pView->GetCameraUsageHint() == usageHint)
    {
      return pView;
    }
  }

  return nullptr;
}

void ezRenderLoop::AddViewToRender(ezView* pView)
{
  EZ_LOCK(s_ViewsToRenderMutex);
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
    auto& filteredRenderPipelines = s_FilteredRenderPipelines[GetDataIndexForExtraction()];
    filteredRenderPipelines.Clear();

    for (ezUInt32 i = s_ViewsToRender.GetCount(); i-- > 0;)
    {
      auto& pRenderPipeline = s_ViewsToRender[i]->m_pRenderPipeline;
      if (!filteredRenderPipelines.Contains(pRenderPipeline))
      {
        filteredRenderPipelines.PushBack(pRenderPipeline);
      }
    }

    s_ViewsToRender.Clear();
  }  
}

void ezRenderLoop::Render(ezRenderContext* pRenderContext)
{
  auto& filteredRenderPipelines = s_FilteredRenderPipelines[GetDataIndexForRendering()];

  for (auto& pRenderPipeline : filteredRenderPipelines)
  {
    // If we are the only one holding a reference to the pipeline skip rendering. The pipeline is not needed anymore and will be deleted soon.
    if (pRenderPipeline->GetRefCount() > 1)
    {
      pRenderPipeline->Render(pRenderContext);
    }
    pRenderPipeline = nullptr;
  }

  filteredRenderPipelines.Clear();
}

void ezRenderLoop::BeginFrame()
{
  s_RenderingThreadID = ezThreadUtils::GetCurrentThreadID();

  for (auto& view : s_MainViews)
  {
    view->EnsureUpToDate();
  }

  for (auto& pipelineToRebuild : s_PipelinesToRebuild)
  {
    pipelineToRebuild.m_pPipeline->Rebuild(*pipelineToRebuild.m_pView);
  }

  s_PipelinesToRebuild.Clear();

  s_BeginFrameEvent.Broadcast(s_uiFrameCounter);
}

void ezRenderLoop::EndFrame()
{
  s_EndFrameEvent.Broadcast(s_uiFrameCounter);

  ++s_uiFrameCounter;

  for (auto& view : s_MainViews)
  {
    view->ReadBackPassProperties();
  }

  s_RenderingThreadID = (ezThreadID)0;
}

bool ezRenderLoop::GetUseMultithreadedRendering()
{
  return CVarMultithreadedRendering;
}


bool ezRenderLoop::IsRenderingThread()
{
  return s_RenderingThreadID == ezThreadUtils::GetCurrentThreadID();
}

void ezRenderLoop::AddRenderPipelineToRebuild(ezRenderPipeline* pRenderPipeline, ezView* pView)
{
  EZ_LOCK(s_PipelinesToRebuildMutex);

  for (auto& pipelineToRebuild : s_PipelinesToRebuild)
  {
    if (pipelineToRebuild.m_pView == pView)
    {
      pipelineToRebuild.m_pPipeline = pRenderPipeline;
      return;
    }
  }

  auto& pipelineToRebuild = s_PipelinesToRebuild.ExpandAndGetRef();
  pipelineToRebuild.m_pPipeline = pRenderPipeline;
  pipelineToRebuild.m_pView = pView;
}

void ezRenderLoop::OnEngineShutdown()
{
  s_FilteredRenderPipelines[0].Clear();
  s_FilteredRenderPipelines[1].Clear();

  for (auto pView : s_MainViews)
  {
    pView->~ezView();
    ezFoundation::GetDefaultAllocator()->Deallocate(pView);
  }

  ClearMainViews();
}
