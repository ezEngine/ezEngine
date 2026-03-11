#pragma once

#include <Foundation/Logging/Log.h>

template <typename ResourceType>
EZ_FORCE_INLINE ResourceType* ezResourceManager::GetResource(ezStringView sResourceID, bool bIsReloadable)
{
  return static_cast<ResourceType*>(GetResource(ezGetStaticRTTI<ResourceType>(), sResourceID, bIsReloadable));
}

template <typename ResourceType>
EZ_FORCE_INLINE ezTypedResourceHandle<ResourceType> ezResourceManager::LoadResource(ezStringView sResourceID)
{
  // the mutex here is necessary to prevent a race between resource unloading and storing the pointer in the handle
  EZ_LOCK(s_ResourceMutex);
  return ezTypedResourceHandle<ResourceType>(GetResource<ResourceType>(sResourceID, true));
}

template <typename ResourceType>
ezTypedResourceHandle<ResourceType> ezResourceManager::LoadResource(ezStringView sResourceID, ezTypedResourceHandle<ResourceType> hLoadingFallback)
{
  ezTypedResourceHandle<ResourceType> hResource;
  {
    // the mutex here is necessary to prevent a race between resource unloading and storing the pointer in the handle
    EZ_LOCK(s_ResourceMutex);
    hResource = ezTypedResourceHandle<ResourceType>(GetResource<ResourceType>(sResourceID, true));
  }

  if (hLoadingFallback.IsValid())
  {
    hResource.m_pResource->SetLoadingFallbackResource(hLoadingFallback);
  }

  return hResource;
}

template <typename ResourceType>
ezTypedResourceHandle<ResourceType> ezResourceManager::GetExistingResource(ezStringView sResourceID)
{
  ezResource* pResource = nullptr;

  const ezTempHashedString sResourceHash(sResourceID);

  EZ_LOCK(s_ResourceMutex);

  const ezRTTI* pRtti = FindResourceTypeOverride(ezGetStaticRTTI<ResourceType>(), sResourceID);

  if (GetLoadedResources()[pRtti].m_Resources.TryGetValue(sResourceHash, pResource))
    return ezTypedResourceHandle<ResourceType>((ResourceType*)pResource);

  return ezTypedResourceHandle<ResourceType>();
}

template <typename ResourceType, typename DescriptorType>
ezTypedResourceHandle<ResourceType> ezResourceManager::CreateResource(ezStringView sResourceID, DescriptorType&& descriptor, ezStringView sResourceDescription)
{
  return CreateResourceInternal<ResourceType, DescriptorType>(sResourceID, std::move(descriptor), sResourceDescription, false);
}

template <typename ResourceType, typename DescriptorType>
ezTypedResourceHandle<ResourceType> ezResourceManager::CreateResourceInternal(ezStringView sResourceID, DescriptorType&& descriptor, ezStringView sResourceDescription, bool bAllowGetFallback)
{
  static_assert(std::is_rvalue_reference<DescriptorType&&>::value, "Please std::move the descriptor into this function");

  ezTypedResourceHandle<ResourceType> hResource;
  ResourceType* pResource = nullptr;
  EZ_LOG_BLOCK("ezResourceManager::CreateResource", sResourceID);
  {
    EZ_LOCK(s_ResourceMutex);
    // In this locked scope, we decide whether we are creating the resource or waiting for it to be created by someone else.
    if (bAllowGetFallback)
      hResource = GetExistingResource<ResourceType>(sResourceID);

    if (!hResource.IsValid())
    {
      hResource = ezTypedResourceHandle<ResourceType>(GetResource<ResourceType>(sResourceID, false));
      pResource = BeginAcquireResource(hResource, ezResourceAcquireMode::PointerOnly);
      pResource->SetResourceDescription(sResourceDescription);
      pResource->m_Flags.Add(ezResourceFlags::IsCreatedResource);

      EZ_ASSERT_DEV(pResource->GetLoadingState() == ezResourceState::Unloaded, "CreateResource was called on a resource that is already created");
    }
  }

  if (!pResource)
  {
    // If we didn't acquire the resource yet, someone else already did so we just wait for them to finish creation.
    pResource = BeginAcquireResource(hResource, ezResourceAcquireMode::BlockTillLoaded);
    EndAcquireResource(pResource);
    return hResource;
  }


  // If this does not compile, you either passed in the wrong descriptor type for the given resource type
  // or you forgot to std::move the descriptor when calling CreateResource
  auto localDescriptor = std::move(descriptor);
  ezResourceLoadDesc ld = pResource->CreateResource(std::move(localDescriptor));

  {
    EZ_LOCK(s_ResourceMutex);
    pResource->VerifyAfterCreateResource(ld);
    EZ_ASSERT_DEV(pResource->GetLoadingState() != ezResourceState::Unloaded, "CreateResource did not set the loading state properly.");
  }

  EndAcquireResource(pResource);
  return hResource;
}

template <typename ResourceType, typename DescriptorType>
ezTypedResourceHandle<ResourceType>
ezResourceManager::GetOrCreateResource(ezStringView sResourceID, DescriptorType&& descriptor, ezStringView sResourceDescription)
{
  return CreateResourceInternal<ResourceType, DescriptorType>(sResourceID, std::move(descriptor), sResourceDescription, true);
}

EZ_FORCE_INLINE ezResource* ezResourceManager::BeginAcquireResourcePointer(const ezRTTI* pType, const ezTypelessResourceHandle& hResource)
{
  EZ_IGNORE_UNUSED(pType);
  EZ_ASSERT_DEV(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

  ezResource* pResource = (ezResource*)hResource.m_pResource;

  EZ_ASSERT_DEBUG(pResource->GetDynamicRTTI()->IsDerivedFrom(pType),
    "The requested resource does not have the same type ('{0}') as the resource handle ('{1}').", pResource->GetDynamicRTTI()->GetTypeName(),
    pType->GetTypeName());

  // pResource->m_iLockCount.Increment();
  return pResource;
}

template <typename ResourceType>
ResourceType* ezResourceManager::BeginAcquireResource(const ezTypedResourceHandle<ResourceType>& hResource, ezResourceAcquireMode mode,
  const ezTypedResourceHandle<ResourceType>& hFallbackResource, ezResourceAcquireResult* out_pAcquireResult /*= nullptr*/)
{
  EZ_ASSERT_DEV(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  const ezResource* pCurrentlyUpdatingContent = ezResource::GetCurrentlyUpdatingContent();
  if (pCurrentlyUpdatingContent != nullptr)
  {
    EZ_LOCK(s_ResourceMutex);
    EZ_ASSERT_DEV(mode == ezResourceAcquireMode::PointerOnly || IsResourceTypeAcquireDuringUpdateContentAllowed(pCurrentlyUpdatingContent->GetDynamicRTTI(), ezGetStaticRTTI<ResourceType>()),
      "Trying to acquire a resource of type '{0}' during '{1}::UpdateContent()'. This has to be enabled by calling "
      "ezResourceManager::AllowResourceTypeAcquireDuringUpdateContent<{1}, {0}>(); at engine startup, for example in "
      "ezGameApplication::Init_SetupDefaultResources().",
      ezGetStaticRTTI<ResourceType>()->GetTypeName(), pCurrentlyUpdatingContent->GetDynamicRTTI()->GetTypeName());
  }
#endif

  ResourceType* pResource = (ResourceType*)hResource.m_hTypeless.m_pResource;

  // EZ_ASSERT_DEV(pResource->m_iLockCount < 20, "You probably forgot somewhere to call 'EndAcquireResource' in sync with 'BeginAcquireResource'.");
  EZ_ASSERT_DEBUG(pResource->GetDynamicRTTI()->template IsDerivedFrom<ResourceType>(),
    "The requested resource does not have the same type ('{0}') as the resource handle ('{1}').", pResource->GetDynamicRTTI()->GetTypeName(),
    ezGetStaticRTTI<ResourceType>()->GetTypeName());

  if (mode == ezResourceAcquireMode::AllowLoadingFallback && GetForceNoFallbackAcquisition() > 0)
  {
    mode = ezResourceAcquireMode::BlockTillLoaded;
  }

  if (mode == ezResourceAcquireMode::PointerOnly)
  {
    if (out_pAcquireResult)
      *out_pAcquireResult = ezResourceAcquireResult::Final;

    // pResource->m_iLockCount.Increment();
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

      if (mode == ezResourceAcquireMode::AllowLoadingFallback &&
          (pResource->m_hLoadingFallback.IsValid() || hFallbackResource.IsValid() || GetResourceTypeLoadingFallback<ResourceType>().IsValid()))
      {
        // return the fallback resource for now, if there is one
        if (out_pAcquireResult)
          *out_pAcquireResult = ezResourceAcquireResult::LoadingFallback;

        // Fallback order is as follows:
        //  1) Prefer any resource specific fallback resource
        //  2) If not available, use the fallback that is given to BeginAcquireResource, as that is at least specific to the situation
        //  3) If nothing else is available, take the fallback for the whole resource type

        if (pResource->m_hLoadingFallback.IsValid())
          return (ResourceType*)BeginAcquireResource(pResource->m_hLoadingFallback, ezResourceAcquireMode::BlockTillLoaded);
        else if (hFallbackResource.IsValid())
          return (ResourceType*)BeginAcquireResource(hFallbackResource, ezResourceAcquireMode::BlockTillLoaded);
        else
          return (ResourceType*)BeginAcquireResource(GetResourceTypeLoadingFallback<ResourceType>(), ezResourceAcquireMode::BlockTillLoaded);
      }

      EnsureResourceLoadingState(pResource, ezResourceState::Loaded);
    }
    else
    {
      // as long as there are more quality levels available, schedule the resource for more loading
      // accessing IsQueuedForLoading without a lock here is save because InternalPreloadResource() will lock and early out if necessary
      // and accidentally skipping InternalPreloadResource() is no problem
      if (IsQueuedForLoading(pResource) == false && pResource->GetNumQualityLevelsLoadable() > 0)
      {
        InternalPreloadResource(pResource, false);

        if (GetForceNoFallbackAcquisition() > 0)
        {
          EnsureResourceCondition(pResource, [=]() -> bool
            { return ((ezInt32)pResource->GetLoadingState() >= (ezInt32)ezResourceState::Loaded && pResource->GetNumQualityLevelsLoadable() == 0) ||
                     (pResource->GetLoadingState() == ezResourceState::LoadedResourceMissing); });
        }
      }
    }
  }

  if (pResource->GetLoadingState() == ezResourceState::LoadedResourceMissing)
  {
    // When you get a crash with a stack overflow in this code path, then the resource to be used as the
    // 'missing resource' replacement might be missing itself.

    if (ezResourceManager::GetResourceTypeMissingFallback<ResourceType>().IsValid())
    {
      if (out_pAcquireResult)
        *out_pAcquireResult = ezResourceAcquireResult::MissingFallback;

      return (ResourceType*)BeginAcquireResource(
        ezResourceManager::GetResourceTypeMissingFallback<ResourceType>(), ezResourceAcquireMode::BlockTillLoaded);
    }

    if (mode != ezResourceAcquireMode::AllowLoadingFallback_NeverFail && mode != ezResourceAcquireMode::BlockTillLoaded_NeverFail)
    {
      EZ_REPORT_FAILURE("The resource '{0}' of type '{1}' is missing and no fallback is available", pResource->GetResourceID(),
        ezGetStaticRTTI<ResourceType>()->GetTypeName());
    }

    if (out_pAcquireResult)
      *out_pAcquireResult = ezResourceAcquireResult::None;

    return nullptr;
  }

  if (out_pAcquireResult)
    *out_pAcquireResult = ezResourceAcquireResult::Final;

  // pResource->m_iLockCount.Increment();
  return pResource;
}

template <typename ResourceType>
void ezResourceManager::EndAcquireResource(ResourceType* pResource)
{
  EZ_IGNORE_UNUSED(pResource);
  // EZ_ASSERT_DEV(pResource->m_iLockCount > 0, "The resource lock counter is incorrect: {0}", (ezInt32)pResource->m_iLockCount);
  // pResource->m_iLockCount.Decrement();
}

EZ_FORCE_INLINE void ezResourceManager::EndAcquireResourcePointer(ezResource* pResource)
{
  EZ_IGNORE_UNUSED(pResource);
  // EZ_ASSERT_DEV(pResource->m_iLockCount > 0, "The resource lock counter is incorrect: {0}", (ezInt32)pResource->m_iLockCount);
  // pResource->m_iLockCount.Decrement();
}

template <typename ResourceType>
ezLockedObject<ezMutex, ezDynamicArray<ezResource*>> ezResourceManager::GetAllResourcesOfType()
{
  const ezRTTI* pBaseType = ezGetStaticRTTI<ResourceType>();

  auto& container = GetLoadedResourceOfTypeTempContainer();

  // We use a static container here to ensure its life-time is extended beyond
  // calls to this function as the locked object does not own the passed-in object
  // and thus does not extend the data life-time. It is safe to do this, as the
  // locked object holding the container ensures the container will not be
  // accessed concurrently.
  ezLockedObject<ezMutex, ezDynamicArray<ezResource*>> loadedResourcesLock(s_ResourceMutex, &container);

  container.Clear();

  for (auto itType = GetLoadedResources().GetIterator(); itType.IsValid(); itType.Next())
  {
    const ezRTTI* pDerivedType = itType.Key();

    if (pDerivedType->IsDerivedFrom(pBaseType))
    {
      const LoadedResources& lr = GetLoadedResources()[pDerivedType];

      container.Reserve(container.GetCount() + lr.m_Resources.GetCount());

      for (auto itResource : lr.m_Resources)
      {
        container.PushBack(itResource.Value());
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

EZ_FORCE_INLINE bool ezResourceManager::ReloadResource(const ezRTTI* pType, const ezTypelessResourceHandle& hResource, bool bForce)
{
  ezResource* pResource = BeginAcquireResourcePointer(pType, hResource);

  bool res = ReloadResource(pResource, bForce);

  EndAcquireResourcePointer(pResource);

  return res;
}

template <typename ResourceType>
ezUInt32 ezResourceManager::ReloadResourcesOfType(bool bForce)
{
  return ReloadResourcesOfType(ezGetStaticRTTI<ResourceType>(), bForce);
}

template <typename ResourceType>
void ezResourceManager::SetResourceTypeLoader(ezResourceTypeLoader* pCreator)
{
  EZ_LOCK(s_ResourceMutex);

  GetResourceTypeLoaders()[ezGetStaticRTTI<ResourceType>()] = pCreator;
}

template <typename ResourceType>
ezTypedResourceHandle<ResourceType> ezResourceManager::GetResourceHandleForExport(ezStringView sResourceID)
{
  EZ_ASSERT_DEV(IsExportModeEnabled(), "Export mode needs to be enabled");

  return LoadResource<ResourceType>(sResourceID);
}

template <typename ResourceType>
void ezResourceManager::SetIncrementalUnloadForResourceType(bool bActive)
{
  GetResourceTypeInfo(ezGetStaticRTTI<ResourceType>()).m_bIncrementalUnload = bActive;
}
