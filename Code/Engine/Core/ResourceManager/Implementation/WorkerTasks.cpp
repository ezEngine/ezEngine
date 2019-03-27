#include <CorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Profiling/Profiling.h>

ezResourceManagerWorkerDataLoad::ezResourceManagerWorkerDataLoad() = default;
ezResourceManagerWorkerDataLoad::~ezResourceManagerWorkerDataLoad() = default;

void ezResourceManagerWorkerDataLoad::Execute()
{
  DoWork(false);
}

void ezResourceManagerWorkerDataLoad::DoWork(bool bCalledExternally)
{
  EZ_PROFILE_SCOPE("LoadResourceFromDisk");

  ezResource* pResourceToLoad = nullptr;
  ezResourceTypeLoader* pLoader = nullptr;
  ezUniquePtr<ezResourceTypeLoader> pCustomLoader;

  {
    EZ_LOCK(ezResourceManager::s_ResourceMutex);

    ezResourceManager::UpdateLoadingDeadlines();

    if (ezResourceManager::s_RequireLoading.IsEmpty())
    {
      ezResourceManager::s_bTaskRunning = false;
      return;
    }

    auto it = ezResourceManager::s_RequireLoading.PeekFront();
    pResourceToLoad = it.m_pResource;
    ezResourceManager::s_RequireLoading.PopFront();

    if (pResourceToLoad->m_Flags.IsSet(ezResourceFlags::HasCustomDataLoader))
    {
      pCustomLoader = std::move(ezResourceManager::s_CustomLoaders[pResourceToLoad]);
      pLoader = pCustomLoader.Borrow();
      pResourceToLoad->m_Flags.Remove(ezResourceFlags::HasCustomDataLoader);
      pResourceToLoad->m_Flags.Add(ezResourceFlags::PreventFileReload);
    }
  }

  if (pLoader == nullptr)
    pLoader = ezResourceManager::GetResourceTypeLoader(pResourceToLoad->GetDynamicRTTI());

  if (pLoader == nullptr)
    pLoader = pResourceToLoad->GetDefaultResourceTypeLoader();

  EZ_ASSERT_DEV(
    pLoader != nullptr, "No Loader function available for Resource Type '{0}'", pResourceToLoad->GetDynamicRTTI()->GetTypeName());

  ezResourceLoadData LoaderData = pLoader->OpenDataStream(pResourceToLoad);

  // we need this info later to do some work in a lock, all the directly following code is outside the lock
  const bool bResourceIsLoadedOnMainThread = pResourceToLoad->GetBaseResourceFlags().IsAnySet(ezResourceFlags::UpdateOnMainThread);

  ezResourceManagerWorkerUpdateContent* pWorkerMainThread = nullptr;

  {
    EZ_LOCK(ezResourceManager::s_ResourceMutex);

    // take the oldest task from the queue, in hopes that it is the most likely one to have finished by now
    pWorkerMainThread = &ezResourceManager::s_WorkerTasksUpdateContent[ezResourceManager::s_uiCurrentUpdateContentWorkerTask];
    ezResourceManager::s_uiCurrentUpdateContentWorkerTask =
      (ezResourceManager::s_uiCurrentUpdateContentWorkerTask + 1) % ezResourceManager::MaxUpdateContentTasks;
  }

  // make sure the task that we grabbed has finished, before reusing it
  ezTaskSystem::WaitForTask(pWorkerMainThread);

  {
    EZ_LOCK(ezResourceManager::s_ResourceMutex);

    pWorkerMainThread->m_LoaderData = LoaderData;
    pWorkerMainThread->m_pLoader = pLoader;
    pWorkerMainThread->m_pCustomLoader = std::move(pCustomLoader);
    pWorkerMainThread->m_pResourceToLoad = pResourceToLoad;

    // schedule the task to run, either on the main thread or on some other thread
    ezTaskSystem::StartSingleTask(
      pWorkerMainThread, bResourceIsLoadedOnMainThread ? ezTaskPriority::SomeFrameMainThread : ezTaskPriority::NextFrame);
  }

  // all this will happen inside a lock
  {
    EZ_LOCK(ezResourceManager::s_ResourceMutex);

    if (!bCalledExternally)
    {
      // restart the next loading task (this one is about to finish)
      ezResourceManager::s_bTaskRunning = false;
      ezResourceManager::RunWorkerTask(nullptr);
    }

    pCustomLoader.Reset();
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

    EZ_ASSERT_DEV(MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its CPU memory usage",
      m_pResourceToLoad->GetResourceID());
    EZ_ASSERT_DEV(MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its GPU memory usage",
      m_pResourceToLoad->GetResourceID());

    m_pResourceToLoad->m_MemoryUsage = MemUsage;
  }

  m_pLoader->CloseDataStream(m_pResourceToLoad, m_LoaderData);

  {
    EZ_LOCK(ezResourceManager::s_ResourceMutex);
    EZ_ASSERT_DEV(m_pResourceToLoad->m_Flags.IsSet(ezResourceFlags::IsPreloading) == true, "");
    m_pResourceToLoad->m_Flags.Remove(ezResourceFlags::IsPreloading);
  }

  m_pLoader = nullptr;
  m_pResourceToLoad = nullptr;
}
