#pragma once

#include <Foundation/Logging/Log.h>

template <typename ResourceType>
ResourceType* ezResourceManager::GetResource(const char* szResourceID, bool bIsReloadable)
{
  return static_cast<ResourceType*>(GetResource(ezGetStaticRTTI<ResourceType>(), szResourceID, bIsReloadable));
}

template <typename ResourceType>
ezTypedResourceHandle<ResourceType> ezResourceManager::LoadResource(const char* szResourceID)
{
  return ezTypedResourceHandle<ResourceType>(GetResource<ResourceType>(szResourceID, true));
}

template <typename ResourceType>
ezTypedResourceHandle<ResourceType> ezResourceManager::LoadResource(const char* szResourceID, ezResourcePriority Priority,
                                                                    ezTypedResourceHandle<ResourceType> hFallbackResource)
{
  ezTypedResourceHandle<ResourceType> hResource(GetResource<ResourceType>(szResourceID, true));

  ResourceType* pResource = ezResourceManager::BeginAcquireResource(hResource, ezResourceAcquireMode::PointerOnly,
                                                                    ezTypedResourceHandle<ResourceType>(), Priority);

  if (hFallbackResource.IsValid())
  {
    pResource->SetFallbackResource(hFallbackResource);
  }

  ezResourceManager::EndAcquireResource(pResource);

  return hResource;
}

template <typename ResourceType>
ezTypedResourceHandle<ResourceType> ezResourceManager::GetExistingResource(const char* szResourceID)
{
  ezResourceBase* pResource = nullptr;

  const ezTempHashedString sResourceHash(szResourceID);

  EZ_LOCK(s_ResourceMutex);
  
  if (s_LoadedResources[ezGetStaticRTTI<ResourceType>()].m_Resources.TryGetValue(sResourceHash, pResource))
    return ezTypedResourceHandle<ResourceType>((ResourceType*)pResource);

  return ezTypedResourceHandle<ResourceType>();
}

template <typename ResourceType>
ezTypedResourceHandle<ResourceType> ezResourceManager::CreateResource(const char* szResourceID,
                                                                      const typename ResourceType::DescriptorType& descriptor,
                                                                      const char* szResourceDescription)
{
  EZ_LOG_BLOCK("ezResourceManager::CreateResource", szResourceID);

  EZ_LOCK(s_ResourceMutex);

  ezTypedResourceHandle<ResourceType> hResource(GetResource<ResourceType>(szResourceID, false));

  ResourceType* pResource = BeginAcquireResource(hResource, ezResourceAcquireMode::PointerOnly);
  pResource->SetResourceDescription(szResourceDescription);

  EZ_ASSERT_DEV(pResource->GetLoadingState() == ezResourceState::Unloaded,
                "CreateResource was called on a resource that is already created");

  // If this does not compile, you have forgotten to make ezResourceManager a friend of your resource class.
  // which probably means that you did not derive from ezResource, which you should do!
  static_cast<ezResource<ResourceType, typename ResourceType::DescriptorType>*>(pResource)->CallCreateResource(descriptor);

  EZ_ASSERT_DEV(pResource->GetLoadingState() != ezResourceState::Unloaded, "CreateResource did not set the loading state properly.");

  EndAcquireResource(pResource);

  return hResource;
}

template <typename ResourceType>
ResourceType* ezResourceManager::BeginAcquireResource(const ezTypedResourceHandle<ResourceType>& hResource, ezResourceAcquireMode mode,
                                                      const ezTypedResourceHandle<ResourceType>& hFallbackResource,
                                                      ezResourcePriority Priority, ezResourceAcquireResult* out_AcquireResult /*= nullptr*/)
{
  // EZ_LOCK(s_ResourceMutex);

  EZ_ASSERT_DEV(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

  ResourceType* pResource = (ResourceType*)hResource.m_Typeless.m_pResource;

  EZ_ASSERT_DEV(pResource->m_iLockCount < 20,
                "You probably forgot somewhere to call 'EndAcquireResource' in sync with 'BeginAcquireResource'.");
  EZ_ASSERT_DEBUG(pResource->GetDynamicRTTI() == ezGetStaticRTTI<ResourceType>(),
                  "The requested resource does not have the same type ('{0}') as the resource handle ('{1}').",
                  pResource->GetDynamicRTTI()->GetTypeName(), ezGetStaticRTTI<ResourceType>()->GetTypeName());

  if (mode == ezResourceAcquireMode::PointerOnly ||
      (mode == ezResourceAcquireMode::MetaInfo && pResource->GetLoadingState() >= ezResourceState::UnloadedMetaInfoAvailable))
  {
    if (Priority != ezResourcePriority::Unchanged)
      pResource->m_Priority = Priority;

    if (out_AcquireResult)
      *out_AcquireResult = ezResourceAcquireResult::Final;

    pResource->m_iLockCount.Increment();
    return pResource;
  }

  // only set the last accessed time stamp, if it is actually needed, pointer-only access might not mean that the resource is used
  // productively
  pResource->m_LastAcquire = m_LastFrameUpdate;

  if (pResource->GetLoadingState() != ezResourceState::LoadedResourceMissing)
  {
    if (pResource->GetLoadingState() != ezResourceState::Loaded)
    {
      // only modify the priority, if the resource is not yet loaded
      if (Priority != ezResourcePriority::Unchanged)
        pResource->SetPriority(Priority);

      // will prepend this at the preload array, thus will be loaded immediately
      // even after recalculating priorities, it will end up as top priority
      InternalPreloadResource(pResource, true);

      if (mode == ezResourceAcquireMode::AllowFallback &&
          (pResource->m_hFallback.IsValid() || hFallbackResource.IsValid() || ResourceType::GetTypeFallbackResource().IsValid()))
      {
        // return the fallback resource for now, if there is one
        if (out_AcquireResult)
          *out_AcquireResult = ezResourceAcquireResult::LoadingFallback;

        // Fallback order is as follows:
        //  1) Prefer any resource specific fallback resource
        //  2) If not available, use the fallback that is given to BeginAcquireResource, as that is at least specific to the situation
        //  3) If nothing else is available, take the fallback for the whole resource type

        if (pResource->m_hFallback.IsValid())
          return (ResourceType*)BeginAcquireResource(pResource->m_hFallback, ezResourceAcquireMode::NoFallback);
        else if (hFallbackResource.IsValid())
          return (ResourceType*)BeginAcquireResource(hFallbackResource, ezResourceAcquireMode::NoFallback);
        else
          return (ResourceType*)BeginAcquireResource(ResourceType::GetTypeFallbackResource(), ezResourceAcquireMode::NoFallback);
      }

      const ezResourceState RequestedState =
          (mode == ezResourceAcquireMode::MetaInfo) ? ezResourceState::UnloadedMetaInfoAvailable : ezResourceState::Loaded;

      EnsureResourceLoadingState(pResource, RequestedState);
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
    // When you get a crash with a stack overflow in this code path, then the resource to be used as the
    // 'missing resource' replacement might be missing itself.

    if (/*mode == ezResourceAcquireMode::AllowFallback && (hFallbackResource.IsValid() || */ ResourceType::GetTypeMissingResource()
            .IsValid()) //)
    {
      if (out_AcquireResult)
        *out_AcquireResult = ezResourceAcquireResult::MissingFallback;

      // prefer the fallback given for this situation (might e.g. be a default normal map)
      // use the type specific missing resource otherwise

      // if (hFallbackResource.IsValid())
      //  return (ResourceType*) BeginAcquireResource(hFallbackResource, ezResourceAcquireMode::NoFallback);
      // else
      return (ResourceType*)BeginAcquireResource(ResourceType::GetTypeMissingResource(), ezResourceAcquireMode::NoFallback);
    }

    EZ_REPORT_FAILURE("The resource '{0}' of type '{1}' is missing and no fallback is available", pResource->GetResourceID(),
                      ezGetStaticRTTI<ResourceType>()->GetTypeName());

    if (out_AcquireResult)
      *out_AcquireResult = ezResourceAcquireResult::None;

    return nullptr;
  }

  if (out_AcquireResult)
    *out_AcquireResult = ezResourceAcquireResult::Final;

  pResource->m_iLockCount.Increment();
  return pResource;
}

template <typename ResourceType>
void ezResourceManager::EndAcquireResource(ResourceType* pResource)
{
  EZ_ASSERT_DEV(pResource != nullptr, "Resource Pointer cannot be nullptr.");
  EZ_ASSERT_DEV(pResource->m_iLockCount > 0, "The resource lock counter is incorrect: {0}", (ezInt32)pResource->m_iLockCount);

  pResource->m_iLockCount.Decrement();
}


template <typename ResourceType>
void ezResourceManager::RestoreResource(const ezTypedResourceHandle<ResourceType>& hResource)
{
  ResourceType* pResource = BeginAcquireResource(hResource, ezResourceAcquireMode::PointerOnly);
  pResource->m_Flags.Remove(ezResourceFlags::PreventFileReload);

  ReloadResource(pResource, true);

  EndAcquireResource(pResource);
}

template <typename ResourceType>
bool ezResourceManager::ReloadResource(const ezTypedResourceHandle<ResourceType>& hResource, bool bForce)
{
  ResourceType* pResource = BeginAcquireResource(hResource, ezResourceAcquireMode::PointerOnly);

  bool res = ReloadResource(pResource, bForce);

  EndAcquireResource(pResource);

  return res;
}

template <typename ResourceType>
ezUInt32 ezResourceManager::ReloadResourcesOfType(bool bForce)
{
  return ReloadResourcesOfType(ezGetStaticRTTI<ResourceType>(), bForce);
}

template <typename ResourceType>
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
