#include <PCH.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Profiling/Profiling.h>

ezCVarBool CVarMultithreadedRendering("r_Multithreading", true, ezCVarFlags::Default, "Enables multi-threaded update and rendering");

ezEvent<ezView*> ezRenderWorld::s_ViewCreatedEvent;
ezEvent<ezView*> ezRenderWorld::s_ViewDeletedEvent;
ezEvent<ezUInt64> ezRenderWorld::s_BeginFrameEvent;
ezEvent<ezUInt64> ezRenderWorld::s_EndFrameEvent;

ezUInt64 ezRenderWorld::s_uiFrameCounter;

namespace
{
  static bool s_bInExtract;
  static ezThreadID s_RenderingThreadID;

  static ezMutex s_ExtractTasksMutex;
  static ezDynamicArray<ezTaskGroupID> s_ExtractTasks;

  static ezMutex s_ViewsMutex;
  static ezIdTable<ezViewId, ezView*> s_Views;

  static ezDynamicArray<ezViewHandle> s_MainViews;

  static ezMutex s_ViewsToRenderMutex;
  static ezDynamicArray<ezView*> s_ViewsToRender;

  static ezDynamicArray<ezSharedPtr<ezRenderPipeline>> s_FilteredRenderPipelines[2];

  struct PipelineToRebuild
  {
    EZ_DECLARE_POD_TYPE();

    ezRenderPipeline* m_pPipeline;
    ezViewHandle m_hView;
  };

  static ezMutex s_PipelinesToRebuildMutex;
  static ezDynamicArray<PipelineToRebuild> s_PipelinesToRebuild;
}

EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RenderWorld)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_ENGINE_SHUTDOWN
  {
    ezRenderWorld::OnEngineShutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION

ezViewHandle ezRenderWorld::CreateView(const char* szName, ezView*& out_pView)
{
  ezView* pView = EZ_DEFAULT_NEW(ezView);

  {
    EZ_LOCK(s_ViewsMutex);
    pView->m_InternalId = s_Views.Insert(pView);
  }

  pView->SetName(szName);
  pView->InitializePins();

  s_ViewCreatedEvent.Broadcast(pView);

  out_pView = pView;
  return pView->GetHandle();
}

void ezRenderWorld::DeleteView(const ezViewHandle& hView)
{
  ezView* pView = nullptr;

  {
    EZ_LOCK(s_ViewsMutex);
    if (!s_Views.Remove(hView, &pView))
      return;
  }

  s_ViewDeletedEvent.Broadcast(pView);

  {
    EZ_LOCK(s_PipelinesToRebuildMutex);

    for (ezUInt32 i = s_PipelinesToRebuild.GetCount(); i-- > 0;)
    {
      if (s_PipelinesToRebuild[i].m_hView == hView)
      {
        s_PipelinesToRebuild.RemoveAt(i);
      }
    }
  }

  RemoveMainView(hView);

  EZ_DEFAULT_DELETE(pView);
}

bool ezRenderWorld::TryGetView(const ezViewHandle& hView, ezView*& out_pView)
{
  EZ_LOCK(s_ViewsMutex);
  return s_Views.TryGetValue(hView, out_pView);
}

ezView* ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::Enum usageHint)
{
  EZ_LOCK(s_ViewsMutex);

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    ezView* pView = it.Value();
    if (pView->GetCameraUsageHint() == usageHint)
    {
      return pView;
    }
  }

  return nullptr;
}

void ezRenderWorld::AddMainView(const ezViewHandle& hView)
{
  EZ_ASSERT_DEV(!s_bInExtract, "Cannot add main view during extraction");

  if (!s_MainViews.Contains(hView))
    s_MainViews.PushBack(hView);
}

void ezRenderWorld::RemoveMainView(const ezViewHandle& hView)
{
  EZ_ASSERT_DEV(!s_bInExtract, "Cannot remove main view during extraction");

  s_MainViews.Remove(hView);
}

void ezRenderWorld::ClearMainViews()
{
  EZ_ASSERT_DEV(!s_bInExtract, "Cannot clear main views during extraction");

  s_MainViews.Clear();
}

ezArrayPtr<ezViewHandle> ezRenderWorld::GetMainViews()
{
  return s_MainViews;
}

void ezRenderWorld::AddViewToRender(const ezViewHandle& hView)
{
  ezView* pView = nullptr;
  if (!TryGetView(hView, pView))
    return;

  if (!pView->IsValid())
    return;

  if (pView->m_uiFrameCounterWhenAddedLast >= s_uiFrameCounter)
    return;

  pView->m_uiFrameCounterWhenAddedLast = s_uiFrameCounter;

  {
    EZ_LOCK(s_ViewsToRenderMutex);
    EZ_ASSERT_DEV(s_bInExtract, "Render views need to be collected during extraction");

    s_ViewsToRender.PushBack(pView);
  }

  if (CVarMultithreadedRendering)
  {
    ezTaskGroupID extractTaskID = ezTaskSystem::StartSingleTask(pView->GetExtractTask(), ezTaskPriority::EarlyThisFrame);

    {
      EZ_LOCK(s_ExtractTasksMutex);
      s_ExtractTasks.PushBack(extractTaskID);
    }
  }
  else
  {
    pView->ExtractData();
  }
}

void ezRenderWorld::ExtractMainViews()
{
  EZ_ASSERT_DEV(!s_bInExtract, "ExtractMainViews must not be called from multiple threads.");

  s_bInExtract = true;

  if (CVarMultithreadedRendering)
  {
    s_ExtractTasks.Clear();

    ezTaskGroupID extractTaskID = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);
    s_ExtractTasks.PushBack(extractTaskID);

    {
      EZ_LOCK(s_ViewsMutex);

      for (ezUInt32 i = 0; i < s_MainViews.GetCount(); ++i)
      {
        ezView* pView = nullptr;
        if (s_Views.TryGetValue(s_MainViews[i], pView) && pView->IsValid())
        {
          s_ViewsToRender.PushBack(pView);
          ezTaskSystem::AddTaskToGroup(extractTaskID, pView->GetExtractTask());
        }
      }
    }

    ezTaskSystem::StartTaskGroup(extractTaskID);

    {
      EZ_PROFILE("Wait for Extraction");

      while (true)
      {
        ezTaskGroupID taskID;

        {
          EZ_LOCK(s_ExtractTasksMutex);
          if (s_ExtractTasks.IsEmpty())
            break;

          taskID = s_ExtractTasks.PeekBack();
          s_ExtractTasks.PopBack();
        }

        ezTaskSystem::WaitForGroup(taskID);
      }
    }
  }
  else
  {
    for (ezUInt32 i = 0; i < s_MainViews.GetCount(); ++i)
    {
      ezView* pView = nullptr;
      if (s_Views.TryGetValue(s_MainViews[i], pView) && pView->IsValid())
      {
        s_ViewsToRender.PushBack(pView);
        pView->ExtractData();
      }
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

void ezRenderWorld::Render(ezRenderContext* pRenderContext)
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

void ezRenderWorld::BeginFrame()
{
  s_RenderingThreadID = ezThreadUtils::GetCurrentThreadID();

  s_BeginFrameEvent.Broadcast(s_uiFrameCounter);

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    ezView* pView = it.Value();
    pView->EnsureUpToDate();
  }

  for (auto& pipelineToRebuild : s_PipelinesToRebuild)
  {
    ezView* pView = nullptr;
    if (s_Views.TryGetValue(pipelineToRebuild.m_hView, pView))
    {
      pipelineToRebuild.m_pPipeline->Rebuild(*pView);
    }
  }

  s_PipelinesToRebuild.Clear();
}

void ezRenderWorld::EndFrame()
{
  s_EndFrameEvent.Broadcast(s_uiFrameCounter);

  ++s_uiFrameCounter;

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    ezView* pView = it.Value();
    if (pView->IsValid())
    {
      pView->ReadBackPassProperties();
    }
  }

  s_RenderingThreadID = (ezThreadID)0;
}

bool ezRenderWorld::GetUseMultithreadedRendering()
{
  return CVarMultithreadedRendering;
}


bool ezRenderWorld::IsRenderingThread()
{
  return s_RenderingThreadID == ezThreadUtils::GetCurrentThreadID();
}

void ezRenderWorld::AddRenderPipelineToRebuild(ezRenderPipeline* pRenderPipeline, const ezViewHandle& hView)
{
  EZ_LOCK(s_PipelinesToRebuildMutex);

  for (auto& pipelineToRebuild : s_PipelinesToRebuild)
  {
    if (pipelineToRebuild.m_hView == hView)
    {
      pipelineToRebuild.m_pPipeline = pRenderPipeline;
      return;
    }
  }

  auto& pipelineToRebuild = s_PipelinesToRebuild.ExpandAndGetRef();
  pipelineToRebuild.m_pPipeline = pRenderPipeline;
  pipelineToRebuild.m_hView = hView;
}

void ezRenderWorld::OnEngineShutdown()
{
  s_FilteredRenderPipelines[0].Clear();
  s_FilteredRenderPipelines[1].Clear();

  ClearMainViews();

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    ezView* pView = it.Value();
    EZ_DEFAULT_DELETE(pView);
  }

  s_Views.Clear();
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_RenderLoop_Implementation_RenderLoop);

