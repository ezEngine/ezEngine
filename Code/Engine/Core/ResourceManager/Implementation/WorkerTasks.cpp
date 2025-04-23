#include <Core/CorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceManagerState.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Profiling/Profiling.h>

ezResourceManagerWorkerDataLoad::ezResourceManagerWorkerDataLoad() = default;
ezResourceManagerWorkerDataLoad::~ezResourceManagerWorkerDataLoad() = default;

void ezResourceManagerWorkerDataLoad::Execute()
{
  EZ_PROFILE_SCOPE("LoadResourceFromDisk");

  ezResource* pResourceToLoad = nullptr;
  ezResourceTypeLoader* pLoader = nullptr;
  ezUniquePtr<ezResourceTypeLoader> pCustomLoader;

  {
    EZ_LOCK(ezResourceManager::s_ResourceMutex);

    if (ezResourceManager::s_pState->m_LoadingQueue.IsEmpty())
    {
      ezResourceManager::s_pState->m_bAllowLaunchDataLoadTask = true;
      return;
    }

    ezResourceManager::UpdateLoadingDeadlines();

    auto it = ezResourceManager::s_pState->m_LoadingQueue.PeekFront();
    pResourceToLoad = it.m_pResource;
    ezResourceManager::s_pState->m_LoadingQueue.PopFront();

    if (pResourceToLoad->m_Flags.IsSet(ezResourceFlags::HasCustomDataLoader))
    {
      pCustomLoader = std::move(ezResourceManager::s_pState->m_CustomLoaders[pResourceToLoad]);
      pLoader = pCustomLoader.Borrow();
      pResourceToLoad->m_Flags.Remove(ezResourceFlags::HasCustomDataLoader);
      pResourceToLoad->m_Flags.Add(ezResourceFlags::PreventFileReload);
    }
  }

  if (pLoader == nullptr)
    pLoader = ezResourceManager::GetResourceTypeLoader(pResourceToLoad->GetDynamicRTTI());

  if (pLoader == nullptr)
    pLoader = pResourceToLoad->GetDefaultResourceTypeLoader();

  EZ_ASSERT_DEV(pLoader != nullptr, "No Loader function available for Resource Type '{0}'", pResourceToLoad->GetDynamicRTTI()->GetTypeName());

  ezResourceLoadData LoaderData = pLoader->OpenDataStream(pResourceToLoad);

  // we need this info later to do some work in a lock, all the directly following code is outside the lock
  const bool bResourceIsLoadedOnMainThread = pResourceToLoad->GetBaseResourceFlags().IsAnySet(ezResourceFlags::UpdateOnMainThread);

  ezSharedPtr<ezResourceManagerWorkerUpdateContent> pUpdateContentTask;
  ezTaskGroupID* pUpdateContentGroup = nullptr;

  EZ_LOCK(ezResourceManager::s_ResourceMutex);

  // try to find an update content task that has finished and can be reused
  for (ezUInt32 i = 0; i < ezResourceManager::s_pState->m_WorkerTasksUpdateContent.GetCount(); ++i)
  {
    auto& td = ezResourceManager::s_pState->m_WorkerTasksUpdateContent[i];

    if (ezTaskSystem::IsTaskGroupFinished(td.m_GroupId))
    {
      pUpdateContentTask = td.m_pTask;
      pUpdateContentGroup = &td.m_GroupId;
      break;
    }
  }

  // if no such task could be found, we must allocate a new one
  if (pUpdateContentTask == nullptr)
  {
    ezStringBuilder s;
    s.SetFormat("Resource Content Updater {0}", ezResourceManager::s_pState->m_WorkerTasksUpdateContent.GetCount());

    auto& td = ezResourceManager::s_pState->m_WorkerTasksUpdateContent.ExpandAndGetRef();
    td.m_pTask = EZ_DEFAULT_NEW(ezResourceManagerWorkerUpdateContent);
    td.m_pTask->ConfigureTask(s, ezTaskNesting::Maybe);

    pUpdateContentTask = td.m_pTask;
    pUpdateContentGroup = &td.m_GroupId;
  }

  // always updated together with pUpdateContentTask
  EZ_MSVC_ANALYSIS_ASSUME(pUpdateContentGroup != nullptr);

  // set up the data load task and launch it
  {
    pUpdateContentTask->m_LoaderData = LoaderData;
    pUpdateContentTask->m_pLoader = pLoader;
    pUpdateContentTask->m_pCustomLoader = std::move(pCustomLoader);
    pUpdateContentTask->m_pResourceToLoad = pResourceToLoad;
    pUpdateContentTask->m_pResourceToLoad->m_iReferenceCount.Increment();

    // schedule the task to run, either on the main thread or on some other thread
    *pUpdateContentGroup = ezTaskSystem::StartSingleTask(
      pUpdateContentTask, bResourceIsLoadedOnMainThread ? ezTaskPriority::SomeFrameMainThread : ezTaskPriority::LateNextFrame);

    // restart the next loading task (this one is about to finish)
    ezResourceManager::s_pState->m_bAllowLaunchDataLoadTask = true;
    ezResourceManager::RunWorkerTask();

    pCustomLoader.Clear();
  }
}


//////////////////////////////////////////////////////////////////////////

ezResourceManagerWorkerUpdateContent::ezResourceManagerWorkerUpdateContent() = default;
ezResourceManagerWorkerUpdateContent::~ezResourceManagerWorkerUpdateContent() = default;

void ezResourceManagerWorkerUpdateContent::Execute()
{
  if (!m_LoaderData.m_sResourceDescription.IsEmpty())
    m_pResourceToLoad->SetResourceDescription(m_LoaderData.m_sResourceDescription);

  m_pResourceToLoad->CallUpdateContent(m_LoaderData.m_pDataStream);

  if (m_pResourceToLoad->m_uiQualityLevelsLoadable > 0)
  {
    // if the resource can have more details loaded, put it into the preload queue right away again
    ezResourceManager::PreloadResource(m_pResourceToLoad);
  }

  // update the file modification date, if available
  if (m_LoaderData.m_LoadedFileModificationDate.IsValid())
    m_pResourceToLoad->m_LoadedFileModificationTime = m_LoaderData.m_LoadedFileModificationDate;

  EZ_ASSERT_DEV(m_pResourceToLoad->GetLoadingState() != ezResourceState::Unloaded, "The resource should have changed its loading state.");

  // Update Memory Usage
  {
    ezResource::MemoryUsage MemUsage;
    MemUsage.m_uiMemoryCPU = 0xFFFFFFFF;
    MemUsage.m_uiMemoryGPU = 0xFFFFFFFF;
    m_pResourceToLoad->UpdateMemoryUsage(MemUsage);

    EZ_ASSERT_DEV(
      MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its CPU memory usage", m_pResourceToLoad->GetResourceID());
    EZ_ASSERT_DEV(
      MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its GPU memory usage", m_pResourceToLoad->GetResourceID());

    m_pResourceToLoad->m_MemoryUsage = MemUsage;
  }

  m_pLoader->CloseDataStream(m_pResourceToLoad, m_LoaderData);

  {
    EZ_LOCK(ezResourceManager::s_ResourceMutex);
    EZ_ASSERT_DEV(ezResourceManager::IsQueuedForLoading(m_pResourceToLoad), "Multi-threaded access detected");
    m_pResourceToLoad->m_Flags.Remove(ezResourceFlags::IsQueuedForLoading);
    m_pResourceToLoad->m_LastAcquire = ezResourceManager::GetLastFrameUpdate();
  }

  m_pLoader = nullptr;
  m_pResourceToLoad->m_iReferenceCount.Decrement();
  m_pResourceToLoad = nullptr;
}
