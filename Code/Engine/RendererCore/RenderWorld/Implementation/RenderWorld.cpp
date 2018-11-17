#include <PCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezCVarBool CVarMultithreadedRendering("r_Multithreading", true, ezCVarFlags::Default, "Enables multi-threaded update and rendering");
ezCVarBool CVarCacheRenderData("r_CacheRenderData", true, ezCVarFlags::Default, "Enables render data caching of static objects");

ezEvent<ezView*> ezRenderWorld::s_ViewCreatedEvent;
ezEvent<ezView*> ezRenderWorld::s_ViewDeletedEvent;
ezEvent<ezUInt64> ezRenderWorld::s_BeginExtractionEvent;
ezEvent<ezUInt64> ezRenderWorld::s_EndExtractionEvent;
ezEvent<ezUInt64> ezRenderWorld::s_BeginRenderEvent;
ezEvent<ezUInt64> ezRenderWorld::s_EndRenderEvent;

ezEvent<void*> ezRenderWorld::s_CameraConfigsModifiedEvent;
bool ezRenderWorld::s_bModifyingCameraConfigs = false;
ezMap<ezString, ezRenderWorld::CameraConfig> ezRenderWorld::s_CameraConfigs;

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

  static ezProxyAllocator* s_pCacheAllocator;
  typedef ezHybridArray<const ezRenderData*, 4> CachedRenderDataPerComponent;
  static ezHashTable<ezComponentHandle, CachedRenderDataPerComponent> s_CachedRenderData;
  static ezDynamicArray<const ezRenderData*> s_DeletedRenderData;

  enum
  {
    MaxNumNewCacheEntries = 32
  };
} // namespace

namespace ezInternal
{
  struct RenderDataCache
  {
    RenderDataCache(ezAllocatorBase* pAllocator)
        : m_EntriesPerObject(pAllocator)
    {
      m_NewEntriesPerComponent.SetCount(m_NewEntriesPerComponent.GetCapacity());
      for (auto& newEntry : m_NewEntriesPerComponent)
      {
        newEntry.m_CacheEntries = CacheEntriesPerObject(pAllocator);
      }
    }

    typedef ezHybridArray<RenderDataCacheEntry, 4> CacheEntriesPerObject;

    ezDynamicArray<CacheEntriesPerObject> m_EntriesPerObject;

    struct NewEntryPerComponent
    {
      ezGameObjectHandle m_hOwnerObject;
      ezComponentHandle m_hOwnerComponent;
      CacheEntriesPerObject m_CacheEntries;
    };

    ezStaticArray<NewEntryPerComponent, MaxNumNewCacheEntries> m_NewEntriesPerComponent;
    ezAtomicInteger32 m_NewEntriesCount;
  };

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
  EZ_CHECK_AT_COMPILETIME(sizeof(RenderDataCacheEntry) == 16);
#endif
} // namespace ezInternal

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RenderWorld)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_ENGINE_STARTUP
  {
    ezRenderWorld::OnEngineStartup();
  }

  ON_ENGINE_SHUTDOWN
  {
    ezRenderWorld::OnEngineShutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezViewHandle ezRenderWorld::CreateView(const char* szName, ezView*& out_pView)
{
  ezView* pView = EZ_DEFAULT_NEW(ezView);

  {
    EZ_LOCK(s_ViewsMutex);
    pView->m_InternalId = s_Views.Insert(pView);
  }

  pView->SetName(szName);
  pView->InitializePins();

  pView->m_pRenderDataCache = EZ_NEW(s_pCacheAllocator, ezInternal::RenderDataCache, s_pCacheAllocator);

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

  EZ_DELETE(s_pCacheAllocator, pView->m_pRenderDataCache);

  {
    EZ_LOCK(s_PipelinesToRebuildMutex);

    for (ezUInt32 i = s_PipelinesToRebuild.GetCount(); i-- > 0;)
    {
      if (s_PipelinesToRebuild[i].m_hView == hView)
      {
        s_PipelinesToRebuild.RemoveAtAndCopy(i);
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
  ezUInt32 uiIndex = s_MainViews.IndexOf(hView);
  if (uiIndex != ezInvalidIndex)
  {
    EZ_ASSERT_DEV(!s_bInExtract, "Cannot remove main view during extraction");
    s_MainViews.RemoveAtAndCopy(uiIndex);
  }
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

void ezRenderWorld::CacheRenderData(const ezView& view, const ezGameObjectHandle& hOwnerObject, const ezComponentHandle& hOwnerComponent,
                                    ezArrayPtr<ezInternal::RenderDataCacheEntry> cacheEntries)
{
  if (CVarCacheRenderData)
  {
    ezUInt32 uiNewEntriesCount = view.m_pRenderDataCache->m_NewEntriesCount;
    if (uiNewEntriesCount >= MaxNumNewCacheEntries)
    {
      return;
    }

    uiNewEntriesCount = view.m_pRenderDataCache->m_NewEntriesCount.Increment();
    if (uiNewEntriesCount <= MaxNumNewCacheEntries)
    {
      auto& newEntry = view.m_pRenderDataCache->m_NewEntriesPerComponent[uiNewEntriesCount - 1];
      newEntry.m_hOwnerObject = hOwnerObject;
      newEntry.m_hOwnerComponent = hOwnerComponent;
      newEntry.m_CacheEntries = cacheEntries;
    }
  }
}

void ezRenderWorld::DeleteAllCachedRenderData()
{
  EZ_ASSERT_DEV(!s_bInExtract, "Cannot delete cached render data during extraction");

  {
    EZ_LOCK(s_ViewsMutex);

    for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
    {
      ezView* pView = it.Value();
      pView->m_pRenderDataCache->m_EntriesPerObject.Clear();
    }
  }

  for (auto it = s_CachedRenderData.GetIterator(); it.IsValid(); ++it)
  {
    auto& cachedRenderDataPerComponent = it.Value();

    for (auto pCachedRenderData : cachedRenderDataPerComponent)
    {
      s_DeletedRenderData.PushBack(pCachedRenderData);
    }

    cachedRenderDataPerComponent.Clear();
  }
}

void ezRenderWorld::DeleteCachedRenderData(const ezGameObjectHandle& hOwnerObject, const ezComponentHandle& hOwnerComponent)
{
  EZ_ASSERT_DEV(!s_bInExtract, "Cannot delete cached render data during extraction");

  ezUInt32 uiCacheIndex = hOwnerObject.GetInternalID().m_InstanceIndex;

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    ezView* pView = it.Value();
    auto& entriesPerObject = pView->m_pRenderDataCache->m_EntriesPerObject;

    if (uiCacheIndex < entriesPerObject.GetCount())
    {
      entriesPerObject[uiCacheIndex].Clear();
    }
  }

  CachedRenderDataPerComponent* pCachedRenderDataPerComponent = nullptr;
  if (s_CachedRenderData.TryGetValue(hOwnerComponent, pCachedRenderDataPerComponent))
  {
    for (auto pCachedRenderData : *pCachedRenderDataPerComponent)
    {
      s_DeletedRenderData.PushBack(pCachedRenderData);
    }

    pCachedRenderDataPerComponent->Clear();
  }
}

void ezRenderWorld::DeleteCachedRenderDataRecursive(const ezGameObject* pOwnerObject)
{
  auto components = pOwnerObject->GetComponents();
  for (auto pComponent : components)
  {
    DeleteCachedRenderData(pOwnerObject->GetHandle(), pComponent->GetHandle());
  }

  for (auto it = pOwnerObject->GetChildren(); it.IsValid(); ++it)
  {
    DeleteCachedRenderDataRecursive(it);
  }
}

ezArrayPtr<ezInternal::RenderDataCacheEntry> ezRenderWorld::GetCachedRenderData(const ezView& view, const ezGameObjectHandle& hOwner)
{
  if (CVarCacheRenderData)
  {
    auto& entriesPerObject = view.m_pRenderDataCache->m_EntriesPerObject;
    ezUInt32 uiCacheIndex = hOwner.GetInternalID().m_InstanceIndex;
    if (uiCacheIndex < entriesPerObject.GetCount())
    {
      return entriesPerObject[uiCacheIndex];
    }
  }

  return ezArrayPtr<ezInternal::RenderDataCacheEntry>();
}

void ezRenderWorld::AddViewToRender(const ezViewHandle& hView)
{
  ezView* pView = nullptr;
  if (!TryGetView(hView, pView))
    return;

  if (!pView->IsValid())
    return;

  {
    EZ_LOCK(s_ViewsToRenderMutex);
    EZ_ASSERT_DEV(s_bInExtract, "Render views need to be collected during extraction");

    // make sure the view is put at the end of the array, if it is already there, reorder it
    // this ensures that the views that have been referenced by the last other view, get rendered first
    ezUInt32 uiIndex = s_ViewsToRender.IndexOf(pView);
    if (uiIndex != ezInvalidIndex)
    {
      s_ViewsToRender.RemoveAtAndCopy(uiIndex);
      s_ViewsToRender.PushBack(pView);
      return;
    }

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

  s_BeginExtractionEvent.Broadcast(s_uiFrameCounter);  

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

  s_EndExtractionEvent.Broadcast(s_uiFrameCounter);

  s_bInExtract = false;
}

void ezRenderWorld::Render(ezRenderContext* pRenderContext)
{
  s_BeginRenderEvent.Broadcast(s_uiFrameCounter);

  if (!CVarMultithreadedRendering)
  {
    RebuildPipelines();
  }

  auto& filteredRenderPipelines = s_FilteredRenderPipelines[GetDataIndexForRendering()];

  for (auto& pRenderPipeline : filteredRenderPipelines)
  {
    // If we are the only one holding a reference to the pipeline skip rendering. The pipeline is not needed anymore and will be deleted
    // soon.
    if (pRenderPipeline->GetRefCount() > 1)
    {
      pRenderPipeline->Render(pRenderContext);
    }
    pRenderPipeline = nullptr;
  }

  filteredRenderPipelines.Clear();

  s_EndRenderEvent.Broadcast(s_uiFrameCounter);
}

void ezRenderWorld::BeginFrame()
{
  EZ_PROFILE("BeginFrame");

  s_RenderingThreadID = ezThreadUtils::GetCurrentThreadID();

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    ezView* pView = it.Value();
    pView->EnsureUpToDate();
  }

  RebuildPipelines();
}

void ezRenderWorld::EndFrame()
{
  EZ_PROFILE("EndFrame");

  ++s_uiFrameCounter;

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    ezView* pView = it.Value();
    if (pView->IsValid())
    {
      pView->ReadBackPassProperties();
    }
  }

  ClearRenderDataCache();
  UpdateRenderDataCache();

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

void ezRenderWorld::ClearRenderDataCache()
{
  for (auto pRenderData : s_DeletedRenderData)
  {
    ezRenderData* ptr = const_cast<ezRenderData*>(pRenderData);
    EZ_DELETE(s_pCacheAllocator, ptr);
  }

  s_DeletedRenderData.Clear();
}

void ezRenderWorld::UpdateRenderDataCache()
{
  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    ezView* pView = it.Value();
    ezUInt32 uiNumNewEntries = ezMath::Min<ezInt32>(pView->m_pRenderDataCache->m_NewEntriesCount, MaxNumNewCacheEntries);
    pView->m_pRenderDataCache->m_NewEntriesCount = 0;

    auto& entriesPerObject = pView->m_pRenderDataCache->m_EntriesPerObject;

    for (ezUInt32 uiNewEntryIndex = 0; uiNewEntryIndex < uiNumNewEntries; ++uiNewEntryIndex)
    {
      auto& newEntries = pView->m_pRenderDataCache->m_NewEntriesPerComponent[uiNewEntryIndex];
      EZ_ASSERT_DEV(!newEntries.m_hOwnerObject.IsInvalidated(), "Implementation error");

      // find or create cached render data
      auto& cachedRenderDataPerComponent = s_CachedRenderData[newEntries.m_hOwnerComponent];

      const ezUInt32 uiNumCachedRenderData = cachedRenderDataPerComponent.GetCount();
      if (uiNumCachedRenderData == 0) // Nothing cached yet
      {
        cachedRenderDataPerComponent = CachedRenderDataPerComponent(s_pCacheAllocator);
      }

      ezUInt32 uiCachedRenderDataIndex = 0;
      for (auto& newEntry : newEntries.m_CacheEntries)
      {
        if (newEntry.m_pRenderData != nullptr)
        {
          if (uiCachedRenderDataIndex >= cachedRenderDataPerComponent.GetCount())
          {
            const ezRTTI* pRtti = newEntry.m_pRenderData->GetDynamicRTTI();
            newEntry.m_pRenderData = pRtti->GetAllocator()->Clone<ezRenderData>(newEntry.m_pRenderData, s_pCacheAllocator);

            cachedRenderDataPerComponent.PushBack(newEntry.m_pRenderData);
          }
          else
          {
            // replace with cached render data
            newEntry.m_pRenderData = cachedRenderDataPerComponent[uiCachedRenderDataIndex];
          }

          ++uiCachedRenderDataIndex;
        }
      }

      // add entry for this view
      ezUInt32 uiCacheIndex = newEntries.m_hOwnerObject.GetInternalID().m_InstanceIndex;
      if (uiCacheIndex >= entriesPerObject.GetCount())
      {
        entriesPerObject.SetCount(uiCacheIndex + 1);
      }

      auto& cacheEntries = entriesPerObject[uiCacheIndex];

      for (auto& newEntry : newEntries.m_CacheEntries)
      {
        if (!cacheEntries.Contains(newEntry))
        {
          cacheEntries.PushBack(newEntry);
        }
      }
    }
  }
}

// static
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

// static
void ezRenderWorld::RebuildPipelines()
{
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

void ezRenderWorld::OnEngineStartup()
{
  s_pCacheAllocator = EZ_DEFAULT_NEW(ezProxyAllocator, "Cached Render Data", ezFoundation::GetDefaultAllocator());

  s_CachedRenderData = ezHashTable<ezComponentHandle, CachedRenderDataPerComponent>(s_pCacheAllocator);
}

void ezRenderWorld::OnEngineShutdown()
{
  ClearRenderDataCache();

  EZ_DEFAULT_DELETE(s_pCacheAllocator);

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

void ezRenderWorld::BeginModifyCameraConfigs()
{
  EZ_ASSERT_DEBUG(!s_bModifyingCameraConfigs, "Recursive call not allowed.");
  s_bModifyingCameraConfigs = true;
}

void ezRenderWorld::EndModifyCameraConfigs()
{
  EZ_ASSERT_DEBUG(s_bModifyingCameraConfigs, "You have to call ezRenderWorld::BeginModifyCameraConfigs first");
  s_bModifyingCameraConfigs = false;
  s_CameraConfigsModifiedEvent.Broadcast(nullptr);
}

void ezRenderWorld::ClearCameraConfigs()
{
  EZ_ASSERT_DEBUG(s_bModifyingCameraConfigs, "You have to call ezRenderWorld::BeginModifyCameraConfigs first");
  s_CameraConfigs.Clear();
}

void ezRenderWorld::SetCameraConfig(const char* szName, const CameraConfig& config)
{
  EZ_ASSERT_DEBUG(s_bModifyingCameraConfigs, "You have to call ezRenderWorld::BeginModifyCameraConfigs first");
  s_CameraConfigs[szName] = config;
}

const ezRenderWorld::CameraConfig* ezRenderWorld::FindCameraConfig(const char* szName)
{
  auto it = s_CameraConfigs.Find(szName);

  if (!it.IsValid())
    return nullptr;

  return &it.Value();
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_RenderWorld_Implementation_RenderWorld);
