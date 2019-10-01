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
ezTypedResourceHandle<ResourceType> ezResourceManager::LoadResource(
  const char* szResourceID, ezTypedResourceHandle<ResourceType> hLoadingFallback)
{
  ezTypedResourceHandle<ResourceType> hResource(GetResource<ResourceType>(szResourceID, true));

  ResourceType* pResource =
    ezResourceManager::BeginAcquireResource(hResource, ezResourceAcquireMode::PointerOnly, ezTypedResourceHandle<ResourceType>());

  if (hLoadingFallback.IsValid())
  {
    pResource->SetLoadingFallbackResource(hLoadingFallback);
  }

  ezResourceManager::EndAcquireResource(pResource);

  return hResource;
}

template <typename ResourceType>
ezTypedResourceHandle<ResourceType> ezResourceManager::GetExistingResource(const char* szResourceID)
{
  ezResource* pResource = nullptr;

  const ezTempHashedString sResourceHash(szResourceID);

  EZ_LOCK(s_ResourceMutex);

  const ezRTTI* pRtti = FindResourceTypeOverride(ezGetStaticRTTI<ResourceType>(), szResourceID);

  if (GetLoadedResources()[pRtti].m_Resources.TryGetValue(sResourceHash, pResource))
    return ezTypedResourceHandle<ResourceType>((ResourceType*)pResource);

  return ezTypedResourceHandle<ResourceType>();
}

template <typename ResourceType, typename DescriptorType>
ezTypedResourceHandle<ResourceType> ezResourceManager::CreateResource(
  const char* szResourceID, DescriptorType&& descriptor, const char* szResourceDescription)
{
  static_assert(std::is_rvalue_reference<DescriptorType&&>::value, "Please std::move the descriptor into this function");

  EZ_LOG_BLOCK("ezResourceManager::CreateResource", szResourceID);

  EZ_LOCK(s_ResourceMutex);

  ezTypedResourceHandle<ResourceType> hResource(GetResource<ResourceType>(szResourceID, false));

  ResourceType* pResource = BeginAcquireResource(hResource, ezResourceAcquireMode::PointerOnly);
  pResource->SetResourceDescription(szResourceDescription);
  pResource->m_Flags.Add(ezResourceFlags::IsCreatedResource);

  EZ_ASSERT_DEV(
    pResource->GetLoadingState() == ezResourceState::Unloaded, "CreateResource was called on a resource that is already created");

  // If this does not compile, you either passed in the wrong descriptor type for the given resource type
  // or you forgot to std::move the descriptor when calling CreateResource
  {
    auto localDescriptor = std::move(descriptor);
    ezResourceLoadDesc ld = pResource->CreateResource(std::move(localDescriptor));
    pResource->VerifyAfterCreateResource(ld);
  }

  EZ_ASSERT_DEV(pResource->GetLoadingState() != ezResourceState::Unloaded, "CreateResource did not set the loading state properly.");

  EndAcquireResource(pResource);

  return hResource;
}

template <typename ResourceType>
ResourceType* ezResourceManager::BeginAcquireResource(const ezTypedResourceHandle<ResourceType>& hResource, ezResourceAcquireMode mode,
  const ezTypedResourceHandle<ResourceType>& hFallbackResource, ezResourceAcquireResult* out_AcquireResult /*= nullptr*/)
{
  EZ_ASSERT_DEV(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

  ResourceType* pResource = (ResourceType*)hResource.m_Typeless.m_pResource;

  EZ_ASSERT_DEV(
    pResource->m_iLockCount < 20, "You probably forgot somewhere to call 'EndAcquireResource' in sync with 'BeginAcquireResource'.");
  EZ_ASSERT_DEBUG(pResource->GetDynamicRTTI()->IsDerivedFrom<ResourceType>(),
    "The requested resource does not have the same type ('{0}') as the resource handle ('{1}').",
    pResource->GetDynamicRTTI()->GetTypeName(), ezGetStaticRTTI<ResourceType>()->GetTypeName());

  if (mode == ezResourceAcquireMode::AllowLoadingFallback && GetForceNoFallbackAcquisition() > 0)
  {
    mode = ezResourceAcquireMode::BlockTillLoaded;
  }

  if (mode == ezResourceAcquireMode::PointerOnly)
  {
    if (out_AcquireResult)
      *out_AcquireResult = ezResourceAcquireResult::Final;

    pResource->m_iLockCount.Increment();
    return pResource;
  }

  // only set the last accessed time stamp, if it is actually needed, pointer-only access might not mean that the resource is used
  // productively
  pResource->m_LastAcquire = GetLastFrameUpdate();

  if (pResource->GetLoadingState() != ezResourceState::LoadedResourceMissing)
  {
    if (pResource->GetLoadingState() != ezResourceState::Loaded)
    {
      // if BlockTillLoaded is specified, it will prepended to the preload array, thus will be loaded immediately
      InternalPreloadResource(pResource, mode >= ezResourceAcquireMode::BlockTillLoaded);

      if (mode == ezResourceAcquireMode::AllowLoadingFallback && (pResource->m_hLoadingFallback.IsValid() || hFallbackResource.IsValid() ||
                                                                   GetResourceTypeLoadingFallback<ResourceType>().IsValid()))
      {
        // return the fallback resource for now, if there is one
        if (out_AcquireResult)
          *out_AcquireResult = ezResourceAcquireResult::LoadingFallback;

        // Fallback order is as follows:
        //  1) Prefer any resource specific fallback resource
        //  2) If not available, use the fallback that is given to BeginAcquireResource, as that is at least specific to the situation
        //  3) If nothing else is available, take the fallback for the whole resource type

        if (pResource->m_hLoadingFallback.IsValid())
          return (ResourceType*)BeginAcquireResource(pResource->m_hLoadingFallback, ezResourceAcquireMode::BlockTillLoaded);
        else if (hFallbackResource.IsValid())
          return (ResourceType*)BeginAcquireResource(hFallbackResource, ezResourceAcquireMode::BlockTillLoaded);
        else
          return (ResourceType*)BeginAcquireResource(
            GetResourceTypeLoadingFallback<ResourceType>(), ezResourceAcquireMode::BlockTillLoaded);
      }

      EnsureResourceLoadingState(pResource, ezResourceState::Loaded);
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

    if (ezResourceManager::GetResourceTypeMissingFallback<ResourceType>().IsValid())
    {
      if (out_AcquireResult)
        *out_AcquireResult = ezResourceAcquireResult::MissingFallback;

      return (ResourceType*)BeginAcquireResource(
        ezResourceManager::GetResourceTypeMissingFallback<ResourceType>(), ezResourceAcquireMode::BlockTillLoaded);
    }

    if (mode != ezResourceAcquireMode::AllowLoadingFallback_NeverFail && mode != ezResourceAcquireMode::BlockTillLoaded_NeverFail)
    {
      EZ_REPORT_FAILURE("The resource '{0}' of type '{1}' is missing and no fallback is available", pResource->GetResourceID(),
        ezGetStaticRTTI<ResourceType>()->GetTypeName());
    }

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
ezLockedObject<ezMutex, ezDynamicArray<ezResource*>> ezResourceManager::GetAllResourcesOfType()
{
  const ezRTTI* pBaseType = ezGetStaticRTTI<ResourceType>();

  // We use a static container here to ensure its life-time is extended beyond
  // calls to this function as the locked object does not own the passed-in object
  // and thus does not extend the data life-time. It is safe to do this, as the
  // locked object holding the container ensures the container will not be
  // accessed concurrently.
  ezLockedObject<ezMutex, ezDynamicArray<ezResource*>> loadedResourcesLock(s_ResourceMutex, &GetLoadedResourceOfTypeTempContainer());

  GetLoadedResourceOfTypeTempContainer().Clear();

  for (auto itType = GetLoadedResources().GetIterator(); itType.IsValid(); itType.Next())
  {
    const ezRTTI* pDerivedType = itType.Key();

    if (pDerivedType->IsDerivedFrom(pBaseType))
    {
      const LoadedResources& lr = GetLoadedResources()[pDerivedType];

      if (lr.m_Resources.IsEmpty())
      {
        // Nothing to do, avoid alloc/reserve below.
        continue;
      }

      GetLoadedResourceOfTypeTempContainer().Reserve(GetLoadedResourceOfTypeTempContainer().GetCount() + lr.m_Resources.GetCount());

      for (auto itResource = lr.m_Resources.GetIterator(); itResource.IsValid(); itResource.Next())
      {
        GetLoadedResourceOfTypeTempContainer().PushBack(itResource.Value());
      }
    }
  }

  return loadedResourcesLock;
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

  GetResourceTypeLoaders()[ezGetStaticRTTI<ResourceType>()] = creator;
}

template <typename ResourceType>
ezTypedResourceHandle<ResourceType> ezResourceManager::GetResourceHandleForExport(const char* szResourceID)
{
  EZ_ASSERT_DEV(IsExportModeEnabled(), "Export mode needs to be enabled");

  return LoadResource<ResourceType>(szResourceID);
}
