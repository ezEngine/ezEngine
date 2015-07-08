#pragma once

#include <Foundation/Logging/Log.h>

template<typename ResourceType>
ResourceType* ezResourceManager::GetResource(const char* szResourceID, bool bIsReloadable)
{
  ezResourceBase* pResource = NULL;

  const ezTempHashedString sResourceHash(szResourceID);

  EZ_LOCK(s_ResourceMutex);

  if (m_LoadedResources.TryGetValue(sResourceHash, pResource))
    return (ResourceType*) pResource;

  const ezRTTI* pRtti = ezGetStaticRTTI<ResourceType>();
  EZ_ASSERT_DEV(pRtti != NULL, "There is no RTTI information available for the given resource type '%s'", EZ_STRINGIZE(ResourceType));
  EZ_ASSERT_DEV(pRtti->GetAllocator() != NULL, "There is no RTTI allocator available for the given resource type '%s'", EZ_STRINGIZE(ResourceType));

  ResourceType* pNewResource = static_cast<ResourceType*>(pRtti->GetAllocator()->Allocate());
  pNewResource->SetUniqueID(szResourceID, bIsReloadable);

  m_LoadedResources.Insert(sResourceHash, pNewResource);

  return (ResourceType*) pNewResource;
}

template<typename ResourceType>
ezResourceHandle<ResourceType> ezResourceManager::LoadResource(const char* szResourceID)
{
  return ezResourceHandle<ResourceType>(GetResource<ResourceType>(szResourceID, true));
}

template<typename ResourceType>
ezResourceHandle<ResourceType> ezResourceManager::LoadResource(const char* szResourceID, ezResourcePriority Priority, ezResourceHandle<ResourceType> hFallbackResource)
{
  ezResourceHandle<ResourceType> hResource(GetResource<ResourceType>(szResourceID, true));

  ResourceType* pResource = ezResourceManager::BeginAcquireResource(hResource, ezResourceAcquireMode::PointerOnly, ezResourceHandle<ResourceType>(), Priority);

  if (hFallbackResource.IsValid())
  {
    pResource->SetFallbackResource(hFallbackResource);
  }

  ezResourceManager::EndAcquireResource(pResource);

  return hResource;
}

template<typename ResourceType>
ezResourceHandle<ResourceType> ezResourceManager::GetExistingResource(const char* szResourceID)
{
  ezResourceBase* pResource = NULL;

  const ezTempHashedString sResourceHash(szResourceID);

  EZ_LOCK(s_ResourceMutex);

  if (m_LoadedResources.TryGetValue(sResourceHash, pResource))
    return (ResourceType*) pResource;

  return ezResourceHandle<ResourceType>();
}

template<typename ResourceType>
ezResourceHandle<ResourceType> ezResourceManager::CreateResource(const char* szResourceID, const typename ResourceType::DescriptorType& descriptor, const char* szResourceDescription)
{
  EZ_LOG_BLOCK("ezResourceManager::CreateResource", szResourceID);

  EZ_LOCK(s_ResourceMutex);

  ezResourceHandle<ResourceType> hResource(GetResource<ResourceType>(szResourceID, false));

  ResourceType* pResource = BeginAcquireResource(hResource, ezResourceAcquireMode::PointerOnly);
  pResource->SetResourceDescription(szResourceDescription);

  EZ_ASSERT_DEV(pResource->GetLoadingState() == ezResourceState::Unloaded, "CreateResource was called on a resource that is already created");

  // If this does not compile, you have forgotten to make ezResourceManager a friend of your resource class.
  // which probably means that you did not derive from ezResource, which you should do!
  static_cast<ezResource<ResourceType, typename ResourceType::DescriptorType>*>(pResource)->CallCreateResource(descriptor);

  EZ_ASSERT_DEV(pResource->GetLoadingState() != ezResourceState::Unloaded, "CreateResource did not set the loading state properly.");

  EndAcquireResource(pResource);

  return hResource;
}

template<typename ResourceType>
ResourceType* ezResourceManager::BeginAcquireResource(const ezResourceHandle<ResourceType>& hResource, ezResourceAcquireMode mode, const ezResourceHandle<ResourceType>& hFallbackResource, ezResourcePriority Priority)
{
  //EZ_LOCK(s_ResourceMutex);

  EZ_ASSERT_DEV(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

  ResourceType* pResource = (ResourceType*) hResource.m_pResource;

  EZ_ASSERT_DEV(pResource->m_iLockCount < 20, "You probably forgot somewhere to call 'EndAcquireResource' in sync with 'BeginAcquireResource'.");
  EZ_ASSERT_DEBUG(pResource->GetDynamicRTTI() == ezGetStaticRTTI<ResourceType>(), "The requested resource does not have the same type ('%s') as the resource handle ('%s').", pResource->GetDynamicRTTI()->GetTypeName(), ezGetStaticRTTI<ResourceType>()->GetTypeName());

  if (mode == ezResourceAcquireMode::PointerOnly ||
      (mode == ezResourceAcquireMode::MetaInfo && pResource->GetLoadingState() >= ezResourceState::UnloadedMetaInfoAvailable))
  {
    pResource->m_iLockCount.Increment();
    return pResource;
  }

  // only set the last accessed time stamp, if it is actually needed, pointer-only access might not mean that the resource is used productively
  pResource->m_LastAcquire = m_LastFrameUpdate;

  if (pResource->GetLoadingState() != ezResourceState::LoadedResourceMissing)
  {
    if (pResource->GetLoadingState() != ezResourceState::Loaded)
    {
      // only modify the priority, if the resource is not yet loaded
      if (Priority != ezResourcePriority::Unchanged)
        pResource->SetPriority(Priority);

      // will append this at the preload array, thus will be loaded immediately
      // even after recalculating priorities, it will end up as top priority
      InternalPreloadResource(pResource, true);

      if (mode == ezResourceAcquireMode::AllowFallback && (pResource->m_hFallback.IsValid() || hFallbackResource.IsValid() || ResourceType::GetTypeFallbackResource().IsValid()))
      {
        // return the fallback resource for now, if there is one

        // Fallback order is as follows:
        //  1) Prefer any resource specific fallback resource
        //  2) If not available, use the fallback that is given to BeginAcquireResource, as that is at least specific to the situation
        //  3) If nothing else is available, take the fallback for the whole resource type

        if (pResource->m_hFallback.IsValid())
          return (ResourceType*) BeginAcquireResource(pResource->m_hFallback, ezResourceAcquireMode::NoFallback);
        else if (hFallbackResource.IsValid())
          return (ResourceType*) BeginAcquireResource(hFallbackResource, ezResourceAcquireMode::NoFallback);
        else
          return (ResourceType*) BeginAcquireResource(ResourceType::GetTypeFallbackResource(), ezResourceAcquireMode::NoFallback);
      }

      const ezResourceState RequestedState = (mode == ezResourceAcquireMode::MetaInfo) ? ezResourceState::UnloadedMetaInfoAvailable : ezResourceState::Loaded;

      // help loading until the requested resource is available
      while ((ezInt32) pResource->GetLoadingState() < (ezInt32) RequestedState && (pResource->GetLoadingState() != ezResourceState::LoadedResourceMissing))
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
      if (pResource->m_Flags.IsSet(ezResourceFlags::IsPreloading) == false && pResource->GetNumQualityLevelsLoadable() > 0)
        InternalPreloadResource(pResource, false);
    }
  }

  if (pResource->GetLoadingState() == ezResourceState::LoadedResourceMissing)
  {
    if (/*mode == ezResourceAcquireMode::AllowFallback && (hFallbackResource.IsValid() || */ResourceType::GetTypeMissingResource().IsValid())//)
    {
      // prefer the fallback given for this situation (might e.g. be a default normal map)
      // use the type specific missing resource otherwise

      //if (hFallbackResource.IsValid())
      //  return (ResourceType*) BeginAcquireResource(hFallbackResource, ezResourceAcquireMode::NoFallback);
      //else
        return (ResourceType*) BeginAcquireResource(ResourceType::GetTypeMissingResource(), ezResourceAcquireMode::NoFallback);
    }

    EZ_REPORT_FAILURE("The resource '%s' of type '%s' is missing and no fallback is available", pResource->GetResourceID().GetData(), ezGetStaticRTTI<ResourceType>()->GetTypeName());
    return nullptr;
  }

  pResource->m_iLockCount.Increment();
  return pResource;
}

template<typename ResourceType>
void ezResourceManager::EndAcquireResource(ResourceType* pResource)
{
  EZ_ASSERT_DEV(pResource != NULL, "Resource Pointer cannot be NULL.");
  EZ_ASSERT_DEV(pResource->m_iLockCount > 0, "The resource lock counter is incorrect: %i", (ezInt32) pResource->m_iLockCount);

  pResource->m_iLockCount.Decrement();
}

template<typename ResourceType>
void ezResourceManager::PreloadResource(const ezResourceHandle<ResourceType>& hResource, ezTime tShouldBeAvailableIn)
{
  ResourceType* pResource = BeginAcquireResource(hResource, ezResourceAcquireMode::PointerOnly);

  PreloadResource(pResource, tShouldBeAvailableIn);

  EndAcquireResource(pResource);
}

template<typename ResourceType>
void ezResourceManager::ReloadResource(const ezResourceHandle<ResourceType>& hResource)
{
  ResourceType* pResource = BeginAcquireResource(hResource, ezResourceAcquireMode::PointerOnly);

  ReloadResource(pResource);

  EndAcquireResource(pResource);
}

template<typename ResourceType>
void ezResourceManager::ReloadResourcesOfType()
{
  ReloadResourcesOfType(ezGetStaticRTTI<ResourceType>());
}

template<typename ResourceType>
void ezResourceManager::SetResourceTypeLoader(ezResourceTypeLoader* creator)
{
  EZ_LOCK(s_ResourceMutex);

  m_ResourceTypeLoader[ezGetStaticRTTI<ResourceType>()->GetTypeName()] = creator;
}

inline void ezResourceManager::SetDefaultResourceLoader(ezResourceTypeLoader* pDefaultLoader)
{
  EZ_LOCK(s_ResourceMutex);

  m_pDefaultResourceLoader = pDefaultLoader;
}

