#pragma once

#include <Core/ResourceManager/Implementation/WorkerTasks.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/LockedObject.h>
#include <Foundation/Types/UniquePtr.h>

class ezResourceManagerState;

/// \brief The central class for managing all types derived from ezResource
class EZ_CORE_DLL ezResourceManager
{
  friend class ezResourceManagerState;
  static ezUniquePtr<ezResourceManagerState> s_State;

  /// \name Events
  ///@{

public:
  /// Events on individual resources. Subscribe to this to get a notification for events happening on any resource.
  /// If you are only interested in events for a specific resource, subscribe on directly on that instance.
  static ezEvent<const ezResourceEvent&>& GetResourceEvents();

  /// Events for the resource manager that affect broader things.
  static ezEvent<const ezResourceManagerEvent&>& GetManagerEvents();

  /// \brief Goes through all existing resources and broadcasts the 'Exists' event.
  ///
  /// Used to announce all currently existing resources to interested event listeners (ie tools).
  static void BroadcastExistsEvent();

  ///@}
  /// \name Loading and creating resources
  ///@{

public:
  /// \brief Returns a handle to the requested resource. szResourceID must uniquely identify the resource, different spellings / casing
  /// will result in different resources.
  ///
  /// After the call to this function the resource definitely exists in memory. Upon access through BeginAcquireResource / ezResourceLock
  /// the resource will be loaded. If it is not possible to load the resource it will change to a 'missing' state. If the code accessing the
  /// resource cannot handle that case, the application will 'terminate' (that means crash).
  template <typename ResourceType>
  static ezTypedResourceHandle<ResourceType> LoadResource(const char* szResourceID);

  /// \brief Same as LoadResource(), but additionally allows to set a priority on the resource and a custom fallback resource for this
  /// instance.
  ///
  /// Pass in ezResourcePriority::Unchanged, if you only want to specify a custom fallback resource.
  /// If a resource priority is specified, the target resource will get that priority.
  /// If a valid fallback resource is specified, the resource will store that as its instance specific fallback resource. This will be used
  /// when trying to acquire the resource later.
  template <typename ResourceType>
  static ezTypedResourceHandle<ResourceType> LoadResource(const char* szResourceID, ezTypedResourceHandle<ResourceType> hLoadingFallback);


  /// \brief Same as LoadResource(), but instead of a template argument, the resource type to use is given as ezRTTI info. Returns a
  /// typeless handle due to the missing template argument.
  static ezTypelessResourceHandle LoadResourceByType(const ezRTTI* pResourceType, const char* szResourceID);

  /// \brief Checks whether any resource loading is in progress
  static bool IsAnyLoadingInProgress();

  /// \brief Generates a unique resource ID with the given prefix.
  ///
  /// Provide a prefix that is preferably not used anywhere else (i.e., closely related to your code).
  /// If the prefix is not also used to manually generate resource IDs, this function is guaranteed to return a unique resource ID.
  static ezString GenerateUniqueResourceID(const char* szResourceIDPrefix);

  /// \brief Creates a resource from a descriptor.
  ///
  /// \param szResourceID The unique ID by which the resource is identified. E.g. in GetExistingResource()
  /// \param descriptor A type specific descriptor that holds all the information to create the resource.
  /// \param szResourceDescription An optional description that might help during debugging. Often a human readable name or path is stored
  /// here, to make it easier to identify this resource.
  template <typename ResourceType, typename DescriptorType>
  static ezTypedResourceHandle<ResourceType> CreateResource(
    const char* szResourceID, DescriptorType&& descriptor, const char* szResourceDescription = nullptr);

  /// \brief Returns a handle to the resource with the given ID. If the resource does not exist, the handle is invalid.
  ///
  /// Use this if a resource needs to be created procedurally (with CreateResource()), but might already have been created.
  /// If the returned handle is invalid, then just go through the resource creation step.
  template <typename ResourceType>
  static ezTypedResourceHandle<ResourceType> GetExistingResource(const char* szResourceID);

  /// \brief Same as GetExistingResourceByType() but allows to specify the resource type as an ezRTTI.
  static ezTypelessResourceHandle GetExistingResourceByType(const ezRTTI* pResourceType, const char* szResourceID);

  /// \brief Triggers loading of the given resource. tShouldBeAvailableIn specifies how long the resource is not yet needed, thus allowing
  /// other resources to be loaded first. This is only a hint and there are no guarantees when the resource is available.
  static void PreloadResource(const ezTypelessResourceHandle& hResource);

  /// \brief Similar to locking a resource with 'BlockTillLoaded' acquire mode, but can be done with a typeless handle and does not return a result.
  static void ForceLoadResourceNow(const ezTypelessResourceHandle& hResource);

  /// \brief Returns the current loading state of the given resource.
  static ezResourceState GetLoadingState(const ezTypelessResourceHandle& hResource);

  ///@}
  /// \name Reloading resources
  ///@{

public:
  /// \brief Goes through all resources and makes sure they are reloaded, if they have changed. If bForce is true, all resources
  /// are updated, even if there is no indication that they have changed.
  static ezUInt32 ReloadAllResources(bool bForce);

  /// \brief Goes through all resources of the given type and makes sure they are reloaded, if they have changed. If bForce is true,
  /// resources are updated, even if there is no indication that they have changed.
  template <typename ResourceType>
  static ezUInt32 ReloadResourcesOfType(bool bForce);

  /// \brief Goes through all resources of the given type and makes sure they are reloaded, if they have changed. If bForce is true,
  /// resources are updated, even if there is no indication that they have changed.
  static ezUInt32 ReloadResourcesOfType(const ezRTTI* pType, bool bForce);

  /// \brief Reloads only the one specific resource. If bForce is true, it is updated, even if there is no indication that it has changed.
  template <typename ResourceType>
  static bool ReloadResource(const ezTypedResourceHandle<ResourceType>& hResource, bool bForce);

  /// \brief Calls ReloadResource() on the given resource, but makes sure that the reload happens with the given custom loader.
  ///
  /// Use this e.g. with a ezResourceLoaderFromMemory to replace an existing resource with new data that was created on-the-fly.
  /// Using this function will set the 'PreventFileReload' flag on the resource and thus prevent further reload actions.
  ///
  /// \sa RestoreResource()
  static void UpdateResourceWithCustomLoader(const ezTypelessResourceHandle& hResource, ezUniquePtr<ezResourceTypeLoader>&& loader);

  /// \brief Removes the 'PreventFileReload' flag and forces a reload on the resource.
  ///
  /// \sa UpdateResourceWithCustomLoader()
  static void RestoreResource(const ezTypelessResourceHandle& hResource);

  ///@}
  /// \name Acquiring resources
  ///@{

public:
  /// \brief Acquires a resource pointer from a handle. Prefer to use ezResourceLock, which wraps BeginAcquireResource / EndAcquireResource
  ///
  /// \param hResource The resource to acquire
  /// \param mode The desired way to acquire the resource. See ezResourceAcquireMode for details.
  /// \param hLoadingFallback A custom fallback resource that should be returned if hResource is not yet available. Allows to use domain
  /// specific knowledge to get a better fallback.
  /// \param Priority Allows to adjust the priority of the resource. This will affect how fast
  /// the resource is loaded, in case it is not yet available.
  /// \param out_AcquireResult Returns how successful the acquisition was. See ezResourceAcquireResult for details.
  template <typename ResourceType>
  static ResourceType* BeginAcquireResource(const ezTypedResourceHandle<ResourceType>& hResource, ezResourceAcquireMode mode,
    const ezTypedResourceHandle<ResourceType>& hLoadingFallback = ezTypedResourceHandle<ResourceType>(),
    ezResourceAcquireResult* out_AcquireResult = nullptr);

  /// \brief Needs to be called in concert with BeginAcquireResource() after accessing a resource has been finished. Prefer to use
  /// ezResourceLock instead.
  template <typename ResourceType>
  static void EndAcquireResource(ResourceType* pResource);

  /// \brief Forces the resource manager to treat ezResourceAcquireMode::AllowLoadingFallback as ezResourceAcquireMode::BlockTillLoaded on
  /// BeginAcquireResource.
  static void ForceNoFallbackAcquisition(ezUInt32 uiNumFrames = 0xFFFFFFFF);

  /// \brief Retrieves an array of pointers to resources of the indicated type which
  /// are loaded at the moment. Destroy the returned object as soon as possible as it
  /// holds the entire resource manager locked.
  template <typename ResourceType>
  static ezLockedObject<ezMutex, ezDynamicArray<ezResource*>> GetAllResourcesOfType();

  ///@}
  /// \name Unloading resources
  ///@{

public:
  /// \brief Deallocates all resources whose refcount has reached 0. Returns the number of deleted resources.
  static ezUInt32 FreeAllUnusedResources();

  /// \brief Deallocates resources whose refcount has reached 0. Returns the number of deleted resources.
  static ezUInt32 FreeUnusedResources(ezTime timeout, ezTime lastAcquireThreshold);

  /// \brief If timeout is not zero, FreeUnusedResources() is called once every frame with the given parameters.
  static void SetAutoFreeUnused(ezTime timeout, ezTime lastAcquireThreshold);

  template <typename ResourceType>
  static void SetIncrementalUnloadForResourceType(bool bDeactivate);


private:
  static ezResult DeallocateResource(ezResource* pResource);

  ///@}
  /// \name Miscellaneous
  ///@{

public:
  /// \brief Returns the resource manager mutex. Allows to lock the manager on a thread when multiple operations need to be done in
  /// sequence.
  static ezMutex& GetMutex() { return s_ResourceMutex; }

  /// \brief Must be called once per frame for some bookkeeping.
  static void PerFrameUpdate();

  /// \brief Makes sure that no further resource loading will take place.
  static void EngineAboutToShutdown();

  /// \brief Calls ezResource::ResetResource() on all resources.
  ///
  /// This is mostly for usage in tools to reset resource whose state can be modified at runtime, to reset them to their original state.
  static void ResetAllResources();

  /// \brief Calls ezResource::UpdateContent() to fill the resource with 'low resolution' data
  ///
  /// This will early out, if the resource has gotten low-res data before.
  /// The resource itself may ignore the data, if it has already gotten low/high res data before.
  ///
  /// The typical use case is, that some other piece of code stores a low-res version of a resource to be able to get
  /// a resource into a usable state. For instance, a material may store low resolution texture data for every texture that it references.
  /// Then when 'loading' the textures, it can pass this low-res data to the textures, such that rendering can give decent results right
  /// away. If the textures have already been loaded before, or some other material already had low-res data, the call exits quickly.
  static void SetResourceLowResData(const ezTypelessResourceHandle& hResource, ezStreamReader* pStream);

  ///@}
  /// \name Type specific loaders
  ///@{

public:
  /// \brief Sets the resource loader to use when no type specific resource loader is available.
  static void SetDefaultResourceLoader(ezResourceTypeLoader* pDefaultLoader);

  /// \brief Returns the resource loader to use when no type specific resource loader is available.
  static ezResourceTypeLoader* GetDefaultResourceLoader();

  /// \brief Sets the resource loader to use for the given resource type.
  ///
  /// \note This is bound to one specific type. Derived types do not inherit the type loader.
  template <typename ResourceType>
  static void SetResourceTypeLoader(ezResourceTypeLoader* pCreator);

  ///@}
  /// \name Named resources
  ///@{

public:
  /// \brief Registers a 'named' resource. When a resource is looked up using \a szLookupName, the lookup will be redirected to \a
  /// szRedirectionResource.
  ///
  /// This can be used to register a resource under an easier to use name. For example one can register "MenuBackground" as the name for "{
  /// E50DCC85-D375-4999-9CFE-42F1377FAC85 }". If the lookup name already exists, it will be overwritten.
  static void RegisterNamedResource(const char* szLookupName, const char* szRedirectionResource);

  /// \brief Removes a previously registered name from the redirection table.
  static void UnregisterNamedResource(const char* szLookupName);


  ///@}
  /// \name Asset system interaction
  ///@{

public:
  /// \brief Registers which resource type to use to load an asset with the given type name
  static void RegisterResourceForAssetType(const char* szAssetTypeName, const ezRTTI* pResourceType);

  /// \brief Returns the resource type that was registered to handle the given asset type for loading. nullptr if no resource type was
  /// registered for this asset type.
  static const ezRTTI* FindResourceForAssetType(const char* szAssetTypeName);

  ///@}
  /// \name Export mode
  ///@{

public:
  /// \brief Enables export mode. In this mode the resource manager will assert when it actually tries to load a resource.
  /// This can be useful when exporting resource handles but the actual resource content is not needed.
  static void EnableExportMode(bool enable);

  /// \brief Returns whether export mode is active.
  static bool IsExportModeEnabled();

  /// \brief Creates a resource handle for the given resource ID. This method can only be used if export mode is enabled.
  /// Internally it will create a resource but does not load the content. This way it can be ensured that the resource handle is always only
  /// the size of a pointer.
  template <typename ResourceType>
  static ezTypedResourceHandle<ResourceType> GetResourceHandleForExport(const char* szResourceID);


  ///@}
  /// \name Resource Type Overrides
  ///@{

public:
  /// \brief Registers a resource type to be used instead of any of it's base classes, when loading specific data
  ///
  /// When resource B is derived from A it can be registered to be instantiated when loading data, even if the code specifies to use a
  /// resource of type A.
  /// Whenever LoadResource<A>() is executed, the registered callback \a OverrideDecider is run to figure out whether B should be
  /// instantiated instead. If OverrideDecider returns true, B is used.
  ///
  /// OverrideDecider is given the resource ID after it has been resolved by the ezFileSystem. So it has to be able to make its decision
  /// from the file path, name or extension.
  /// The override is registered for all base classes of \a pDerivedTypeToUse, in case the derivation hierarchy is longer.
  ///
  /// Without calling this at startup, a derived resource type has to be manually requested in code.
  static void RegisterResourceOverrideType(const ezRTTI* pDerivedTypeToUse, ezDelegate<bool(const ezStringBuilder&)> OverrideDecider);

  /// \brief Unregisters \a pDerivedTypeToUse as an override resource
  ///
  /// \sa RegisterResourceOverrideType()
  static void UnregisterResourceOverrideType(const ezRTTI* pDerivedTypeToUse);

  ///@}
  /// \name Resource Fallbacks
  ///@{

public:
  /// \brief Specifies which resource to use as a loading fallback for the given type, while a resource is not yet loaded.
  template <typename RESOURCE_TYPE>
  static void SetResourceTypeLoadingFallback(const ezTypedResourceHandle<RESOURCE_TYPE>& hResource)
  {
    RESOURCE_TYPE::SetResourceTypeLoadingFallback(hResource);
  }

  /// \sa SetResourceTypeLoadingFallback()
  template <typename RESOURCE_TYPE>
  static inline const ezTypedResourceHandle<RESOURCE_TYPE>& GetResourceTypeLoadingFallback()
  {
    return RESOURCE_TYPE::GetResourceTypeLoadingFallback();
  }

  /// \brief Specifies which resource to use as a missing fallback for the given type, when a resource cannot be loaded.
  ///
  /// \note If no missing fallback is specified, trying to load a resource that does not exist will assert at runtime.
  template <typename RESOURCE_TYPE>
  static void SetResourceTypeMissingFallback(const ezTypedResourceHandle<RESOURCE_TYPE>& hResource)
  {
    RESOURCE_TYPE::SetResourceTypeMissingFallback(hResource);
  }

  /// \sa SetResourceTypeMissingFallback()
  template <typename RESOURCE_TYPE>
  static inline const ezTypedResourceHandle<RESOURCE_TYPE>& GetResourceTypeMissingFallback()
  {
    return RESOURCE_TYPE::GetResourceTypeMissingFallback();
  }

  using ResourceCleanupCB = ezDelegate<void()>;

  /// \brief [internal] Used by ezResource to register a cleanup function to be called at resource manager shutdown.
  static void AddResourceCleanupCallback(ResourceCleanupCB cb);

  /// \sa AddResourceCleanupCallback()
  static void ClearResourceCleanupCallback(ResourceCleanupCB cb);

  /// \brief This will clear ALL resources that were registered as 'missing' or 'loading' fallback resources. This is called early during
  /// system shutdown to clean up resources.
  static void ExecuteAllResourceCleanupCallbacks();

  ///@}
  /// \name Resource Priorities
  ///@{

public:
  /// \brief Specifies which resource to use as a loading fallback for the given type, while a resource is not yet loaded.
  template <typename RESOURCE_TYPE>
  static void SetResourceTypeDefaultPriority(ezResourcePriority priority)
  {
    GetResourceTypePriorities()[ezGetStaticRTTI<RESOURCE_TYPE>()] = priority;
  }

private:
  static ezMap<const ezRTTI*, ezResourcePriority>& GetResourceTypePriorities();
  ///@}

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

private:
  friend class ezResource;
  friend class ezResourceManagerWorkerDataLoad;
  friend class ezResourceManagerWorkerUpdateContent;
  friend class ezResourceHandleReadContext;

  // Events
private:
  static void BroadcastResourceEvent(const ezResourceEvent& e);

  // Miscellaneous
private:
  static ezMutex s_ResourceMutex;

  // Startup / shutdown
private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, ResourceManager);
  static void OnEngineShutdown();
  static void OnCoreShutdown();
  static void OnCoreStartup();
  static void PluginEventHandler(const ezPlugin::PluginEvent& e);

  // Loading / reloading / creating resources
private:
  struct LoadedResources
  {
    ezHashTable<ezTempHashedString, ezResource*> m_Resources;
  };

  struct LoadingInfo
  {
    float m_fPriority = 0;
    ezResource* m_pResource = nullptr;

    EZ_ALWAYS_INLINE bool operator==(const LoadingInfo& rhs) const { return m_pResource == rhs.m_pResource; }
    EZ_ALWAYS_INLINE bool operator<(const LoadingInfo& rhs) const { return m_fPriority < rhs.m_fPriority; }
  };
  static void EnsureResourceLoadingState(ezResource* pResource, const ezResourceState RequestedState);
  static bool HelpResourceLoading();
  static void PreloadResource(ezResource* pResource);
  static void InternalPreloadResource(ezResource* pResource, bool bHighestPriority);

  template <typename ResourceType>
  static ResourceType* GetResource(const char* szResourceID, bool bIsReloadable);
  static ezResource* GetResource(const ezRTTI* pRtti, const char* szResourceID, bool bIsReloadable);
  static void RunWorkerTask(ezResource* pResource);
  static void UpdateLoadingDeadlines();
  static void ReverseBubbleSortStep(ezDeque<LoadingInfo>& data);
  static bool ReloadResource(ezResource* pResource, bool bForce);

  static void SetupWorkerTasks();
  static ezUInt32 GetForceNoFallbackAcquisition();
  static ezTime GetLastFrameUpdate();
  static ezHashTable<const ezRTTI*, LoadedResources>& GetLoadedResources();
  static ezDynamicArray<ezResource*>& GetLoadedResourceOfTypeTempContainer();

  EZ_ALWAYS_INLINE static bool IsQueuedForLoading(ezResource* pResource) { return pResource->m_Flags.IsSet(ezResourceFlags::IsQueuedForLoading); }
  static ezResult RemoveFromLoadingQueue(ezResource* pResource);
  static void AddToLoadingQueue(ezResource* pResource, bool bHighPriority);

  struct ResourceTypeInfo
  {
    bool m_bIncrementalUnload = true;
  };

  static ResourceTypeInfo& GetResourceTypeInfo(const ezRTTI* pRtti);

  // Type loaders
private:
  static ezResourceTypeLoader* GetResourceTypeLoader(const ezRTTI* pRTTI);

  static ezMap<const ezRTTI*, ezResourceTypeLoader*>& GetResourceTypeLoaders();

  // Override / derived resources
private:
  struct DerivedTypeInfo
  {
    const ezRTTI* m_pDerivedType = nullptr;
    ezDelegate<bool(const ezStringBuilder&)> m_Decider;
  };

  /// \brief Checks whether there is a type override for pRtti given szResourceID and returns that
  static const ezRTTI* FindResourceTypeOverride(const ezRTTI* pRtti, const char* szResourceID);
};

#include <Core/ResourceManager/Implementation/ResourceLock.h>
#include <Core/ResourceManager/Implementation/ResourceManager_inl.h>
