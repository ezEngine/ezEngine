#pragma once

template<typename ResourceType>
ResourceType* ezResourceManager::GetResource(const char* szResourceID)
{
  ezResourceBase* pResource = NULL;

  const ezTempHashedString sResourceHash(szResourceID);

  if (m_LoadedResources.TryGetValue(sResourceHash, pResource))
    return (ResourceType*) pResource;

  const ezRTTI* pRtti = ezGetStaticRTTI<ResourceType>();
  EZ_ASSERT(pRtti != NULL, "There is no RTTI information available for the given resource type '%s'", EZ_STRINGIZE(ResourceType));
  EZ_ASSERT(pRtti->GetAllocator() != NULL, "There is no RTTI allocator available for the given resource type '%s'", EZ_STRINGIZE(ResourceType));

  ResourceType* pNewResource = static_cast<ResourceType*>(pRtti->GetAllocator()->Allocate());
  pNewResource->SetUniqueID(szResourceID);

  m_LoadedResources.Insert(sResourceHash, pNewResource);

  return (ResourceType*) pNewResource;
}

template<typename ResourceType>
ezResourceHandle<ResourceType> ezResourceManager::LoadResource(const char* szResourceID)
{
  return ezResourceHandle<ResourceType>(GetResource<ResourceType>(szResourceID));
}

template<typename ResourceType>
ezResourceHandle<ResourceType> ezResourceManager::LoadResource(const char* szResourceID, ezResourcePriority::Enum Priority, ezResourceHandle<ResourceType> hFallbackResource)
{
  ezResourceHandle<ResourceType> hResource (GetResource<ResourceType>(szResourceID));

  ResourceType* pResource = ezResourceManager::BeginAcquireResource(hResource, ezResourceAcquireMode::PointerOnly, Priority);

  if (hFallbackResource.IsValid())
  {
    pResource->SetFallbackResource(hFallbackResource);
  }

  ezResourceManager::EndAcquireResource(pResource);

  return hResource;
}

template<typename ResourceType>
ezResourceHandle<ResourceType> ezResourceManager::GetCreatedResource(const char* szResourceID)
{
  ezResourceBase* pResource = NULL;

  const ezTempHashedString sResourceHash(szResourceID);

  if (m_LoadedResources.TryGetValue(sResourceHash, pResource))
  {
    /// \todo Verify the resource was created, not loaded

    return (ResourceType*) pResource;
  }

  return ezResourceHandle<ResourceType>();
}

template<typename ResourceType>
ezResourceHandle<ResourceType> ezResourceManager::CreateResource(const char* szResourceID, const typename ResourceType::DescriptorType& descriptor)
{
  ezResourceHandle<ResourceType> hResource(GetResource<ResourceType>(szResourceID));

  ResourceType* pResource = BeginAcquireResource(hResource, ezResourceAcquireMode::PointerOnly);

  EZ_ASSERT(pResource->GetLoadingState() == ezResourceLoadState::Uninitialized, "CreateResource was called on a resource this is already created");

  /// \todo Set a flag that this resource was created, and not loaded

  // If this does not compile, you have forgotten to make ezResourceManager a friend of your resource class.
  // which probably means that you did not derive from ezResource, which you should do!
  static_cast<ezResource<ResourceType, typename ResourceType::DescriptorType>*>(pResource)->CreateResource(descriptor);

  EZ_ASSERT(pResource->GetLoadingState() != ezResourceLoadState::Uninitialized, "CreateResource did not set the loading state properly.");
  EZ_ASSERT(pResource->GetMaxQualityLevel() > 0, "CreateResource did not set the max quality level properly.");

  EndAcquireResource(pResource);

  return hResource;
}

template<typename ResourceType>
ResourceType* ezResourceManager::BeginAcquireResource(const ezResourceHandle<ResourceType>& hResource, ezResourceAcquireMode::Enum mode, ezResourcePriority::Enum Priority)
{
  EZ_ASSERT(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

  ResourceType* pResource = (ResourceType*) hResource.m_pResource;

  EZ_ASSERT(pResource->m_iLockCount < 20, "You probably forgot somewhere to call 'EndAcquireResource' in sync with 'BeginAcquireResource'.");
  EZ_ASSERT(pResource->GetDynamicRTTI() == ezGetStaticRTTI<ResourceType>(), "The requested resource does not have the same type ('%s') as the resource handle ('%s').", pResource->GetDynamicRTTI()->GetTypeName(), ezGetStaticRTTI<ResourceType>()->GetTypeName());

  if (mode == ezResourceAcquireMode::PointerOnly ||
     (mode == ezResourceAcquireMode::MetaInfo && pResource->GetLoadingState() >= ezResourceLoadState::MetaInfoAvailable))
  {
    pResource->m_iLockCount.Increment();
    return pResource;
  }

  // only set the last accessed time stamp, if it is actually needed, pointer-only access might not mean that the resource is used productively
  pResource->m_LastAcquire = ezTime::Now();

  if (pResource->GetLoadingState() != ezResourceLoadState::Loaded)
  {
    // only modify the priority, if the resource is not yet loaded
    if (Priority != ezResourcePriority::Unchanged)
      pResource->SetPriority(Priority);

    // will append this at the preload array, thus will be loaded immediately
    // even after recalculating priorities, it will end up as top priority
    InternalPreloadResource(pResource, true);

    if (mode == ezResourceAcquireMode::AllowFallback && pResource->m_hFallback.IsValid())
    {
      // return the fallback resource for now, if there is one
      return (ResourceType*) BeginAcquireResource(pResource->m_hFallback);
    }

    const ezResourceLoadState::Enum RequestedState = (mode == ezResourceAcquireMode::MetaInfo) ? ezResourceLoadState::MetaInfoAvailable : ezResourceLoadState::Loaded;

    // help loading until the requested resource is available
    while (pResource->GetLoadingState() < RequestedState)
    {
      if (!m_WorkerTask[m_iCurrentWorker].IsTaskFinished())
        ezTaskSystem::WaitForTask(&m_WorkerTask[m_iCurrentWorker]);
      else
      {
        for (ezInt32 i = 0; i < 16; ++i)
        {
          // get the 'oldest' GPU task in the queue and try to finish that first
          const ezInt32 iWorkerGPU = (ezResourceManager::m_iCurrentWorkerGPU + i) % 16;

          if (!m_WorkerGPU[iWorkerGPU].IsTaskFinished())
          {
            ezTaskSystem::WaitForTask(&m_WorkerGPU[iWorkerGPU]);
            break; // we waited for one of them, that's enough for this round
          }
        }
      }
    }
  }
  else
  {
    // as long as there are more quality levels available, schedule the resource for more loading
    if (pResource->m_bIsPreloading == false && pResource->m_uiLoadedQualityLevel < pResource->m_uiMaxQualityLevel)
      InternalPreloadResource(pResource, false);
  }

  pResource->m_iLockCount.Increment();
  return pResource;
}

template<typename ResourceType>
void ezResourceManager::EndAcquireResource(ResourceType* pResource)
{
  EZ_ASSERT(pResource != NULL, "Resource Pointer cannot be NULL.");
  EZ_ASSERT(pResource->m_iLockCount > 0, "The resource lock counter is incorrect: %i", (ezInt32) pResource->m_iLockCount);

  pResource->m_iLockCount.Decrement();
}

template<typename ResourceType>
void ezResourceManager::PreloadResource(const ezResourceHandle<ResourceType>& hResource, ezTime tShouldBeAvailableIn)
{
  ResourceType* pResource = BeginAcquireResource(hResource, ezResourceAcquireMode::PointerOnly);

  const ezTime tNow = ezTime::Now();

  pResource->SetDueDate(ezMath::Min(tNow + tShouldBeAvailableIn, pResource->m_DueDate));
  InternalPreloadResource(pResource, tShouldBeAvailableIn <= ezTime::Seconds(0.0)); // if the user set the timeout to zero or below, it will be scheduled immediately

  EndAcquireResource(pResource);
}


template<typename ResourceType>
void ezResourceManager::SetResourceTypeLoader(ezResourceTypeLoader* creator)
{
  m_ResourceTypeLoader[ezGetStaticRTTI<ResourceType>()->GetTypeName()] = creator;
}

inline void ezResourceManager::SetDefaultResourceLoader(ezResourceTypeLoader* pDefaultLoader)
{
  m_pDefaultResourceLoader = pDefaultLoader;
}