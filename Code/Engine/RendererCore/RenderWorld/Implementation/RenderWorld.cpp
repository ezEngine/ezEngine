#include <RendererCorePCH.h>

#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>

ezCVarBool CVarMultithreadedRendering("r_Multithreading", true, ezCVarFlags::Default, "Enables multi-threaded update and rendering");
ezCVarBool CVarCacheRenderData("r_CacheRenderData", true, ezCVarFlags::Default, "Enables render data caching of static objects");

ezEvent<ezView*, ezMutex> ezRenderWorld::s_ViewCreatedEvent;
ezEvent<ezView*, ezMutex> ezRenderWorld::s_ViewDeletedEvent;

ezEvent<void*> ezRenderWorld::s_CameraConfigsModifiedEvent;
bool ezRenderWorld::s_bModifyingCameraConfigs = false;
ezMap<ezString, ezRenderWorld::CameraConfig> ezRenderWorld::s_CameraConfigs;

ezEvent<const ezRenderWorldExtractionEvent&, ezMutex> ezRenderWorld::s_ExtractionEvent;
ezEvent<const ezRenderWorldRenderEvent&, ezMutex> ezRenderWorld::s_RenderEvent;
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

  static ezMutex s_CachedRenderDataMutex;
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
      : m_PerObjectCaches(pAllocator)
    {
      for (ezUInt32 i = 0; i < MaxNumNewCacheEntries; ++i)
      {
        m_NewEntriesPerComponent.PushBack(NewEntryPerComponent(pAllocator));
      }
    }

    struct PerObjectCache
    {
      PerObjectCache() {}

      PerObjectCache(ezAllocatorBase* pAllocator)
        : m_Entries(pAllocator)
      {
      }

      ezHybridArray<RenderDataCacheEntry, 4> m_Entries;
      ezUInt16 m_uiVersion = 0;
    };

    ezDynamicArray<PerObjectCache> m_PerObjectCaches;

    struct NewEntryPerComponent
    {
      NewEntryPerComponent(ezAllocatorBase* pAllocator)
        : m_Cache(pAllocator)
      {
      }

      ezGameObjectHandle m_hOwnerObject;
      ezComponentHandle m_hOwnerComponent;
      PerObjectCache m_Cache;
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

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    ezRenderWorld::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
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

ezView* ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::Enum usageHint, ezCameraUsageHint::Enum alternativeUsageHint /*= ezCameraUsageHint::None*/, const ezWorld* pWorld /*= nullptr*/)
{
  EZ_LOCK(s_ViewsMutex);

  ezView* pAlternativeView = nullptr;

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    ezView* pView = it.Value();
    if (pWorld != nullptr && pView->GetWorld() != pWorld)
      continue;

    if (pView->GetCameraUsageHint() == usageHint)
    {
      return pView;
    }
    else if (alternativeUsageHint != ezCameraUsageHint::None && pView->GetCameraUsageHint() == alternativeUsageHint)
    {
      pAlternativeView = pView;
    }
  }

  return pAlternativeView;
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

void ezRenderWorld::CacheRenderData(const ezView& view, const ezGameObjectHandle& hOwnerObject, const ezComponentHandle& hOwnerComponent, ezUInt16 uiComponentVersion, ezArrayPtr<ezInternal::RenderDataCacheEntry> cacheEntries)
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
      newEntry.m_Cache.m_Entries = cacheEntries;
      newEntry.m_Cache.m_uiVersion = uiComponentVersion;
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
      pView->m_pRenderDataCache->m_PerObjectCaches.Clear();
    }
  }

  {
    EZ_LOCK(s_CachedRenderDataMutex);

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
}

void ezRenderWorld::DeleteCachedRenderData(const ezGameObjectHandle& hOwnerObject, const ezComponentHandle& hOwnerComponent)
{
  EZ_ASSERT_DEV(!s_bInExtract, "Cannot delete cached render data during extraction");

  DeleteCachedRenderDataInternal(hOwnerObject);

  EZ_LOCK(s_CachedRenderDataMutex);

  CachedRenderDataPerComponent* pCachedRenderDataPerComponent = nullptr;
  if (s_CachedRenderData.TryGetValue(hOwnerComponent, pCachedRenderDataPerComponent))
  {
    for (auto pCachedRenderData : *pCachedRenderDataPerComponent)
    {
      s_DeletedRenderData.PushBack(pCachedRenderData);
    }

    s_CachedRenderData.Remove(hOwnerComponent);
  }
}

void ezRenderWorld::ResetRenderDataCache(ezView& view)
{
  view.m_pRenderDataCache->m_PerObjectCaches.Clear();
  view.m_pRenderDataCache->m_NewEntriesCount = 0;

  if (view.GetWorld() != nullptr)
  {
    if (view.GetWorld()->GetObjectDeletionEvent().HasEventHandler(&ezRenderWorld::DeleteCachedRenderDataForObject) == false)
    {
      view.GetWorld()->GetObjectDeletionEvent().AddEventHandler(&ezRenderWorld::DeleteCachedRenderDataForObject);
    }
  }
}

void ezRenderWorld::DeleteCachedRenderDataForObject(const ezGameObject* pOwnerObject)
{
  EZ_ASSERT_DEV(!s_bInExtract, "Cannot delete cached render data during extraction");

  DeleteCachedRenderDataInternal(pOwnerObject->GetHandle());

  EZ_LOCK(s_CachedRenderDataMutex);

  auto components = pOwnerObject->GetComponents();
  for (auto pComponent : components)
  {
    ezComponentHandle hComponent = pComponent->GetHandle();

    CachedRenderDataPerComponent* pCachedRenderDataPerComponent = nullptr;
    if (s_CachedRenderData.TryGetValue(hComponent, pCachedRenderDataPerComponent))
    {
      for (auto pCachedRenderData : *pCachedRenderDataPerComponent)
      {
        s_DeletedRenderData.PushBack(pCachedRenderData);
      }

      s_CachedRenderData.Remove(hComponent);
    }
  }
}

void ezRenderWorld::DeleteCachedRenderDataForObjectRecursive(const ezGameObject* pOwnerObject)
{
  DeleteCachedRenderDataForObject(pOwnerObject);

  for (auto it = pOwnerObject->GetChildren(); it.IsValid(); ++it)
  {
    DeleteCachedRenderDataForObjectRecursive(it);
  }
}

ezArrayPtr<const ezInternal::RenderDataCacheEntry> ezRenderWorld::GetCachedRenderData(const ezView& view, const ezGameObjectHandle& hOwner, ezUInt16 uiComponentVersion)
{
  if (CVarCacheRenderData)
  {
    const auto& perObjectCaches = view.m_pRenderDataCache->m_PerObjectCaches;
    ezUInt32 uiCacheIndex = hOwner.GetInternalID().m_InstanceIndex;
    if (uiCacheIndex < perObjectCaches.GetCount())
    {
      auto& perObjectCache = perObjectCaches[uiCacheIndex];
      if (perObjectCache.m_uiVersion == uiComponentVersion)
      {
        return perObjectCache.m_Entries;
      }
    }
  }

  return ezArrayPtr<const ezInternal::RenderDataCacheEntry>();
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

  ezRenderWorldExtractionEvent extractionEvent;
  extractionEvent.m_Type = ezRenderWorldExtractionEvent::Type::BeginExtraction;
  extractionEvent.m_uiFrameCounter = s_uiFrameCounter;
  s_ExtractionEvent.Broadcast(extractionEvent);

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
      EZ_PROFILE_SCOPE("Wait for Extraction");

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

  extractionEvent.m_Type = ezRenderWorldExtractionEvent::Type::EndExtraction;
  s_ExtractionEvent.Broadcast(extractionEvent);

  s_bInExtract = false;
}

void ezRenderWorld::Render(ezRenderContext* pRenderContext)
{
  ezUInt64 uiRenderFrame = GetUseMultithreadedRendering() ? s_uiFrameCounter - 1 : s_uiFrameCounter;

  // TODO:
  //ezStringBuilder sb;
  //sb.Format("FRAME {}", uiRenderFrame);
  //EZ_PROFILE_AND_MARKER(ezGALDevice::GetDefaultDevice()->GetPrimaryContext(), sb.GetData());

  ezRenderWorldRenderEvent renderEvent;
  renderEvent.m_Type = ezRenderWorldRenderEvent::Type::BeginRender;
  renderEvent.m_uiFrameCounter = s_uiFrameCounter;
  s_RenderEvent.Broadcast(renderEvent);

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

  renderEvent.m_Type = ezRenderWorldRenderEvent::Type::EndRender;
  s_RenderEvent.Broadcast(renderEvent);
}

void ezRenderWorld::BeginFrame()
{
  EZ_PROFILE_SCOPE("BeginFrame");

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
  EZ_PROFILE_SCOPE("EndFrame");

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

void ezRenderWorld::DeleteCachedRenderDataInternal(const ezGameObjectHandle& hOwnerObject)
{
  ezUInt32 uiCacheIndex = hOwnerObject.GetInternalID().m_InstanceIndex;
  ezUInt8 uiWorldIndex = hOwnerObject.GetInternalID().m_WorldIndex;

  EZ_LOCK(s_ViewsMutex);

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    ezView* pView = it.Value();
    if (pView->GetWorld() != nullptr && pView->GetWorld()->GetIndex() == uiWorldIndex)
    {
      auto& perObjectCaches = pView->m_pRenderDataCache->m_PerObjectCaches;

      if (uiCacheIndex < perObjectCaches.GetCount())
      {
        perObjectCaches[uiCacheIndex].m_Entries.Clear();
        perObjectCaches[uiCacheIndex].m_uiVersion = 0;
      }
    }
  }
}

void ezRenderWorld::ClearRenderDataCache()
{
  EZ_PROFILE_SCOPE("Clear Render Data Cache");

  for (auto pRenderData : s_DeletedRenderData)
  {
    ezRenderData* ptr = const_cast<ezRenderData*>(pRenderData);
    EZ_DELETE(s_pCacheAllocator, ptr);
  }

  s_DeletedRenderData.Clear();
}

void ezRenderWorld::UpdateRenderDataCache()
{
  EZ_PROFILE_SCOPE("Update Render Data Cache");

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    ezView* pView = it.Value();
    ezUInt32 uiNumNewEntries = ezMath::Min<ezInt32>(pView->m_pRenderDataCache->m_NewEntriesCount, MaxNumNewCacheEntries);
    pView->m_pRenderDataCache->m_NewEntriesCount = 0;

    auto& perObjectCaches = pView->m_pRenderDataCache->m_PerObjectCaches;

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
      for (auto& newEntry : newEntries.m_Cache.m_Entries)
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
      const ezUInt32 uiCacheIndex = newEntries.m_hOwnerObject.GetInternalID().m_InstanceIndex;
      perObjectCaches.EnsureCount(uiCacheIndex + 1);

      auto& perObjectCache = perObjectCaches[uiCacheIndex];
      if (perObjectCache.m_uiVersion != newEntries.m_Cache.m_uiVersion)
      {
        perObjectCache.m_Entries.Clear();
        perObjectCache.m_uiVersion = newEntries.m_Cache.m_uiVersion;
      }

      for (auto& newEntry : newEntries.m_Cache.m_Entries)
      {
        if (!perObjectCache.m_Entries.Contains(newEntry))
        {
          perObjectCache.m_Entries.PushBack(newEntry);
        }
      }

      // keep entries sorted, otherwise the logic ezExtractor::ExtractRenderData doesn't work
      perObjectCache.m_Entries.Sort();
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
      if (pipelineToRebuild.m_pPipeline->Rebuild(*pView) == ezRenderPipeline::PipelineState::RebuildError)
      {
        ezLog::Error("Failed to rebuild pipeline '{}' for view '{}'", pipelineToRebuild.m_pPipeline->m_sName, pView->GetName());
      }
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
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  for (auto it : s_CachedRenderData)
  {
    auto& cachedRenderDataPerComponent = it.Value();
    if (cachedRenderDataPerComponent.IsEmpty() == false)
    {
      EZ_REPORT_FAILURE("Leaked cached render data of type '{}'", cachedRenderDataPerComponent[0]->GetDynamicRTTI()->GetTypeName());
    }
  }
#endif

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
