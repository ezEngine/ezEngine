#pragma once

#include <Core/CoreInternal.h>
EZ_CORE_INTERNAL_HEADER

#include <Core/ResourceManager/ResourceManager.h>

class ezResourceManagerState
{
private:
  friend class ezResource;
  friend class ezResourceManager;
  friend class ezResourceManagerWorkerDataLoad;
  friend class ezResourceManagerWorkerUpdateContent;
  friend class ezResourceHandleReadContext;

  /// \name Events
  ///@{

  ezEvent<const ezResourceEvent&, ezMutex> m_ResourceEvents;
  ezEvent<const ezResourceManagerEvent&, ezMutex> m_ManagerEvents;

  ///@}
  /// \name Resource Fallbacks
  ///@{

  ezDynamicArray<ezResourceManager::ResourceCleanupCB> m_ResourceCleanupCallbacks;

  ///@}
  /// \name Resource Priorities
  ///@{

  ezMap<const ezRTTI*, ezResourcePriority> m_ResourceTypePriorities;

  ///@}

  struct TaskDataUpdateContent
  {
    ezSharedPtr<ezResourceManagerWorkerUpdateContent> m_pTask;
    ezTaskGroupID m_GroupId;
  };

  struct TaskDataDataLoad
  {
    ezSharedPtr<ezResourceManagerWorkerDataLoad> m_pTask;
    ezTaskGroupID m_GroupId;
  };

  bool m_bTaskNamesInitialized = false;
  bool m_bBroadcastExistsEvent = false;
  ezUInt32 m_uiForceNoFallbackAcquisition = 0;

  // resources in this queue are waiting for a task to load them
  ezDeque<ezResourceManager::LoadingInfo> m_LoadingQueue;

  ezHashTable<const ezRTTI*, ezResourceManager::LoadedResources> m_LoadedResources;

  bool m_bAllowLaunchDataLoadTask = true;
  bool m_bShutdown = false;

  ezHybridArray<TaskDataUpdateContent, 24> m_WorkerTasksUpdateContent;
  ezHybridArray<TaskDataDataLoad, 8> m_WorkerTasksDataLoad;

  ezTime m_LastFrameUpdate;
  ezUInt32 m_uiLastResourcePriorityUpdateIdx = 0;

  ezDynamicArray<ezResource*> m_LoadedResourceOfTypeTempContainer;
  ezHashTable<ezTempHashedString, const ezRTTI*> m_ResourcesToUnloadOnMainThread;

  const ezRTTI* m_pFreeUnusedLastType = nullptr;
  ezTempHashedString m_sFreeUnusedLastResourceID;

  // Type Loaders

  ezMap<const ezRTTI*, ezResourceTypeLoader*> m_ResourceTypeLoader;
  ezResourceLoaderFromFile m_FileResourceLoader;
  ezResourceTypeLoader* m_pDefaultResourceLoader = &m_FileResourceLoader;
  ezMap<ezResource*, ezUniquePtr<ezResourceTypeLoader>> m_CustomLoaders;


  // Override / derived resources

  ezMap<const ezRTTI*, ezHybridArray<ezResourceManager::DerivedTypeInfo, 4>> m_DerivedTypeInfos;


  // Named resources

  ezHashTable<ezTempHashedString, ezHashedString> m_NamedResources;

  // Asset system interaction

  ezMap<ezString, const ezRTTI*> m_AssetToResourceType;


  // Export mode

  bool m_bExportMode = false;
  ezUInt32 m_uiNextResourceID = 0;

  // Resource Unloading
  ezTime m_AutoFreeUnusedTimeout = ezTime::Zero();
  ezTime m_AutoFreeUnusedThreshold = ezTime::Zero();

  ezMap<const ezRTTI*, ezResourceManager::ResourceTypeInfo> m_TypeInfo;
};
