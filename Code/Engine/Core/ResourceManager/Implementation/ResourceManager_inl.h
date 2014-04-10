#pragma once

template<typename ResourceType>
typename ezResourceHandle<ResourceType> ezResourceManager::GetResourceHandle(const ezResourceID& ResourceID)
{
  ezResourceBase* pResource = NULL;

  if (m_LoadedResources.TryGetValue(ResourceID, pResource))
    return ezResourceHandle<ResourceType>((ResourceType*) pResource);

  /// \todo Use reflection allocator here
  ResourceType* pNewResource = EZ_DEFAULT_NEW(ResourceType);
  pNewResource->SetUniqueID(ResourceID);

  m_LoadedResources.Insert(ResourceID, pNewResource);

  return ezResourceHandle<ResourceType>(pNewResource);
}

template<typename ResourceType>
void ezResourceManager::CreateResource(const ezResourceHandle<ResourceType>& hResource, const typename ResourceType::DescriptorType& descriptor)
{
  ResourceType* pResource = BeginAcquireResource(hResource, ezResourceAcquireMode::PointerOnly);

  // If this does not compile, you have forgotten to make ezResourceManager a friend of your resource class.
  pResource->CreateResource(descriptor);

  EndAcquireResource(pResource);
}

template<typename ResourceType>
ResourceType* ezResourceManager::BeginAcquireResource(const ezResourceHandle<ResourceType>& hResource, ezResourceAcquireMode::Enum mode, ezResourcePriority::Enum Priority)
{
  EZ_ASSERT(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

  /// \todo Debug check through RTTI -> is of type

  ResourceType* pResource = (ResourceType*) hResource.m_pResource;

  if (Priority != ezResourcePriority::Unchanged)
    pResource->SetPriority(Priority);

  if (mode == ezResourceAcquireMode::PointerOnly)
  {
    pResource->m_iLockCount.Increment();
    return pResource;
  }

  // only set the last accessed time stamp, if it is actually needed, pointer-only access might not mean that the resource is used productively
  pResource->m_LastAcquire = ezTime::Now();

  if (pResource->GetLoadingState() != ezResourceLoadState::Loaded)
  {
    // will append this at the preload array, thus will be loaded immediately
    // even after recalculating priorities, it will end up as top priority
    PreloadResource(pResource, true);

    if (mode != ezResourceAcquireMode::LoadedNoFallback && pResource->m_hFallback.IsValid())
    {
      // return the fallback resource for now, if there is one
      return (ResourceType*) BeginAcquireResource(pResource->m_hFallback);
    }

    // help loading until the requested resource is available
    while (pResource->GetLoadingState() != ezResourceLoadState::Loaded)
    {
      if (!m_WorkerTask[m_iCurrentWorker].IsTaskFinished())
        ezTaskSystem::WaitForTask(&m_WorkerTask[m_iCurrentWorker]);
      else
      {
        for (ezInt32 i = 0; i < 16; ++i)
        {
          if (!m_WorkerGPU[i].IsTaskFinished())
          {
            ezTaskSystem::WaitForTask(&m_WorkerGPU[i]);
            break;
          }
        }
      }
    }
  }
  else
  {
    if (pResource->m_bIsPreloading == false && pResource->m_uiLoadedQualityLevel < pResource->m_uiMaxQualityLevel)
      PreloadResource(pResource, false);
  }

  pResource->m_iLockCount.Increment();
  return pResource;
}

template<typename ResourceType>
void ezResourceManager::EndAcquireResource(ResourceType* pResource)
{
  EZ_ASSERT(pResource != NULL, "Resource Pointer cannot be NULL.");
  EZ_ASSERT(pResource->m_iLockCount > 0, "The resource lock counter is incorrect: %i", pResource->m_iLockCount);

  pResource->m_iLockCount.Decrement();
}

template<typename ResourceType>
void ezResourceManager::PreloadResource(const ezResourceHandle<ResourceType>& hResource, ezTime tShouldBeAvailableIn)
{
  ResourceType* pResource = BeginAcquireResource(hResource, ezResourceAcquireMode::PointerOnly);

  pResource->SetDueDate(ezTime::Now() + tShouldBeAvailableIn);
  PreloadResource(pResource, false);

  EndAcquireResource(pResource);
}


template<typename ResourceType>
void ezResourceManager::SetResourceTypeLoader(ezResourceTypeLoader* creator)
{
  m_ResourceTypeLoader[ezGetStaticRTTI<ResourceType>()->GetTypeName()] = creator;
}