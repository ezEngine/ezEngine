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

  ezEvent<const ezResourceEvent&> s_ResourceEvents;
  ezEvent<const ezResourceManagerEvent&> s_ManagerEvents;

  ///@}
  /// \name Resource Fallbacks
  ///@{

  ezDynamicArray<ezResourceManager::ResourceCleanupCB> s_ResourceCleanupCallbacks;

  ///@}
  /// \name Resource Priorities
  ///@{

  ezMap<const ezRTTI*, ezResourcePriority> s_ResourceTypePriorities;

  ///@}

  bool m_bTaskNamesInitialized = false;
  bool s_bBroadcastExistsEvent = false;
  ezUInt32 s_uiForceNoFallbackAcquisition = 0;

  // resources in this queue are waiting for a task to load them
  ezDeque<ezResourceManager::LoadingInfo> s_LoadingQueue;

  ezHashTable<const ezRTTI*, ezResourceManager::LoadedResources> s_LoadedResources;
  static const ezUInt32 MaxDataLoadTasks = 4;
  static const ezUInt32 MaxUpdateContentTasks = 16;
  bool s_bDataLoadTaskRunning = false;
  bool s_bShutdown = false;
  ezResourceManagerWorkerDataLoad s_WorkerTasksDataLoad[MaxDataLoadTasks];
  ezResourceManagerWorkerUpdateContent s_WorkerTasksUpdateContent[MaxUpdateContentTasks];
  ezUInt8 s_uiCurrentUpdateContentWorkerTask = 0;
  ezUInt8 s_uiCurrentLoadDataWorkerTask = 0;
  ezTime s_LastFrameUpdate;
  ezUInt32 s_uiLastResourcePriorityUpdateIdx = 0;

  ezDynamicArray<ezResource*> s_LoadedResourceOfTypeTempContainer;
  ezHashTable<ezTempHashedString, const ezRTTI*> s_ResourcesToUnloadOnMainThread;

  const ezRTTI* s_pFreeUnusedLastType = nullptr;
  ezTempHashedString s_FreeUnusedLastResourceID;

  // Type Loaders

  ezMap<const ezRTTI*, ezResourceTypeLoader*> s_ResourceTypeLoader;
  ezResourceLoaderFromFile s_FileResourceLoader;
  ezResourceTypeLoader* s_pDefaultResourceLoader = &s_FileResourceLoader;
  ezMap<ezResource*, ezUniquePtr<ezResourceTypeLoader>> s_CustomLoaders;


  // Override / derived resources

  ezMap<const ezRTTI*, ezHybridArray<ezResourceManager::DerivedTypeInfo, 4>> s_DerivedTypeInfos;


  // Named resources

  ezHashTable<ezTempHashedString, ezHashedString> s_NamedResources;

  // Asset system interaction

  ezMap<ezString, const ezRTTI*> s_AssetToResourceType;


  // Export mode

  bool s_bExportMode = false;
  ezUInt32 s_uiNextResourceID = 0;

  // Resource Unloading
  ezTime m_AutoFreeUnusedTimeout = ezTime::Zero();
  ezTime m_AutoFreeUnusedThreshold = ezTime::Zero();

  ezMap<const ezRTTI*, ezResourceManager::ResourceTypeInfo> m_TypeInfo;
};
