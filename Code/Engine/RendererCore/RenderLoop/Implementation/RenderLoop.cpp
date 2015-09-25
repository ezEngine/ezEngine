#include <RendererCore/PCH.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Pipeline/View.h>

#include <Foundation/Configuration/CVar.h>

/// \todo This is broken: ezRenderPipeline::CreateRenderData -> m_RenderData.PushBack will resize an array not thread-safe
ezCVarBool CVarMultithreadedRendering("Renderer.Multithreading", true, ezCVarFlags::Default, "Enables multithreaded update and rendering");

ezUInt32 ezRenderLoop::s_uiFrameCounter;
ezDynamicArray<ezView*> ezRenderLoop::s_MainViews;

namespace
{
  static bool s_bInExtract;
  static ezTaskGroupID s_ExtractionFinishedTaskID;

  static ezMutex s_ViewsToRenderMutex;
  static ezDynamicArray<ezView*> s_ViewsToRender;

  static ezDynamicArray<ezRenderPipeline*> s_FilteredRenderPipelines[2];

  struct DeletedPipeline
  {
    EZ_DECLARE_MEM_RELOCATABLE_TYPE();

    ezUniquePtr<ezRenderPipeline> m_pPipeline;
    ezUInt32 m_uiFrameUntilDeletion;
  };

  static ezMutex s_DeletedPipelinesMutex;
  static ezDynamicArray<DeletedPipeline> s_DeletedPipelines;

  static bool IsDeleted(ezRenderPipeline* pRenderPipeline)
  {
    EZ_LOCK(s_DeletedPipelinesMutex);

    for (auto& deletedPipeline : s_DeletedPipelines)
    {
      if (deletedPipeline.m_pPipeline.Borrow() == pRenderPipeline)
        return true;
    }

    return false;
  }
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
  return pView;
}

void ezRenderLoop::DeleteView(ezView* pView)
{
  {
    EZ_LOCK(s_DeletedPipelinesMutex);

    auto& deletedPipeline = s_DeletedPipelines.ExpandAndGetRef();
    deletedPipeline.m_pPipeline = std::move(pView->m_pRenderPipeline);
    deletedPipeline.m_uiFrameUntilDeletion = 2;
  }

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
    auto& filteredRenderPipelines = s_FilteredRenderPipelines[s_uiFrameCounter & 1];
    filteredRenderPipelines.Clear();

    for (ezUInt32 i = s_ViewsToRender.GetCount(); i-- > 0;)
    {
      ezRenderPipeline* pRenderPipeline = s_ViewsToRender[i]->GetRenderPipeline();
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
  const ezUInt32 uiFrameCounter = s_uiFrameCounter + (CVarMultithreadedRendering ? 1 : 0);
  auto& filteredRenderPipelines = s_FilteredRenderPipelines[uiFrameCounter & 1];

  for (auto pRenderPipeline : filteredRenderPipelines)
  {
    if (IsDeleted(pRenderPipeline))
      continue;

    pRenderPipeline->Render(pRenderContext);
  }

  filteredRenderPipelines.Clear();

  {
    EZ_LOCK(s_DeletedPipelinesMutex);

    for (ezUInt32 i = s_DeletedPipelines.GetCount(); i-- > 0;)
    {
      auto& deletedPipeline = s_DeletedPipelines[i];
      deletedPipeline.m_uiFrameUntilDeletion--;

      if (deletedPipeline.m_uiFrameUntilDeletion == 0)
      {
        deletedPipeline.m_pPipeline.Reset();
        s_DeletedPipelines.RemoveAt(i);
      }
    }
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

void ezRenderLoop::OnEngineShutdown()
{
  for (auto pView : s_MainViews)
  {
    pView->~ezView();
    ezFoundation::GetDefaultAllocator()->Deallocate(pView);
  }

  ClearMainViews();

  s_DeletedPipelines.Clear();
}
