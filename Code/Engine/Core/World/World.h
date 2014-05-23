#pragma once

#include <Core/World/GameObject.h>
#include <Core/World/Implementation/WorldData.h>

class EZ_CORE_DLL ezWorld
{
public:
  ezWorld(const char* szWorldName);
  ~ezWorld();

  const char* GetName() const;
  
  ezGameObjectHandle CreateObject(const ezGameObjectDesc& desc);
  ezGameObjectHandle CreateObject(const ezGameObjectDesc& desc, ezGameObject*& out_pObject);

  void DeleteObject(const ezGameObjectHandle& object);

  bool IsValidObject(const ezGameObjectHandle& object) const;
  
  bool TryGetObject(const ezGameObjectHandle& object, ezGameObject*& out_pObject) const;
  
  /// \todo
  //bool TryGetObjectWithUniqueId(ezUInt64 uiPersistentId, ezGameObject*& out_pObject) const;
  

  ezUInt32 GetObjectCount() const;
  ezBlockStorage<ezGameObject>::Iterator GetObjects() const;

  // component management
  template <typename ManagerType>
  ManagerType* CreateComponentManager();

  template <typename ManagerType>
  void DeleteComponentManager();

  template <typename ManagerType>
  ManagerType* GetComponentManager() const;

  bool IsValidComponent(const ezComponentHandle& component) const;
  
  template <typename ComponentType>
  bool TryGetComponent(const ezComponentHandle& component, ComponentType*& out_pComponent) const;

  // messaging
  void SendMessage(const ezGameObjectHandle& receiverObject, ezMessage& msg, 
    ezBitflags<ezObjectMsgRouting> routing = ezObjectMsgRouting::Default);

  void PostMessage(const ezGameObjectHandle& receiverObject, ezMessage& msg, 
    ezObjectMsgQueueType::Enum queueType, ezBitflags<ezObjectMsgRouting> routing = ezObjectMsgRouting::Default);
  void PostMessage(const ezGameObjectHandle& receiverObject, ezMessage& msg, 
    ezObjectMsgQueueType::Enum queueType, ezTime delay, ezBitflags<ezObjectMsgRouting> routing = ezObjectMsgRouting::Default);

  // update
  void Update();

  // memory
  ezAllocatorBase* GetAllocator();
  ezLargeBlockAllocator* GetBlockAllocator();

  // user data
  void SetUserData(void* pUserData);
  void* GetUserData() const;

public:
  static ezUInt32 GetWorldCount();
  static ezWorld* GetWorld(ezUInt32 uiIndex);
  
private:
  friend class ezGameObject;
  friend class ezComponentManagerBase;

  void CheckForMultithreadedAccess() const;

  ezGameObject* GetObjectUnchecked(ezUInt32 uiIndex) const;

  void SetParent(ezGameObject* pObject, ezGameObject* pParent);

  void ProcessQueuedMessages(ezObjectMsgQueueType::Enum queueType);

  ezResult RegisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc);
  ezResult RegisterUpdateFunctionWithDependency(const ezComponentManagerBase::UpdateFunctionDesc& desc, bool bInsertAsUnresolved);
  ezResult DeregisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc);

  void UpdateSynchronous(const ezArrayPtr<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions);
  void UpdateAsynchronous();
  void DeleteDeadObjects();
  void DeleteDeadComponents();
  void UpdateHierarchy();

  ezInternal::WorldData m_Data;
  typedef ezInternal::WorldData::ObjectStorage::Entry ObjectStorageEntry;

  typedef ezInternal::WorldData::QueuedMsgMetaData QueuedMsgMetaData;

  ezUInt32 m_uiIndex;
  static ezStaticArray<ezWorld*, 64> s_Worlds;
};

#include <Core/World/Implementation/World_inl.h>

