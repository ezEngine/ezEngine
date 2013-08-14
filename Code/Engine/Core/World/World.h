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

  void CreateObjects(const ezArrayPtr<const ezGameObjectDesc>& descs, ezArrayPtr<ezGameObjectHandle> out_objects);
  void CreateObjects(const ezArrayPtr<const ezGameObjectDesc>& descs, ezArrayPtr<ezGameObjectHandle> out_objects, 
    ezArrayPtr<ezGameObject*> out_pObjects);

  void DeleteObject(const ezGameObjectHandle& object);

  void DeleteObjects(const ezArrayPtr<const ezGameObjectHandle>& objects);

  bool IsValidObject(const ezGameObjectHandle& object) const;
  
  bool TryGetObject(const ezGameObjectHandle& object, ezGameObject*& out_pObject) const;
  
  // slow access
  bool TryGetObject(ezUInt64 uiPersistentId, ezGameObject*& out_pObject) const;
  bool TryGetObject(const char* szObjectName, ezGameObject*& out_pObject) const;

  ezUInt32 GetObjectCount() const;

  // component management
  template <typename ManagerType>
  ManagerType* CreateComponentManager();

  template <typename ManagerType>
  void DeleteComponentManager();

  template <typename ManagerType>
  ManagerType* GetComponentManager() const;

  bool IsValidComponent(const ezComponentHandle& component) const;
  
  template <typename ComponentType>
  bool IsComponentOfType(const ezComponentHandle& component) const;

  template <typename ComponentType>
  bool TryGetComponent(const ezComponentHandle& component, ComponentType*& out_pComponent) const;

  // update
  void Update();

  // memory
  ezIAllocator* GetAllocator();
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

  void SetObjectName(ezGameObjectId internalId, const char* szName);
  const char* GetObjectName(ezGameObjectId internalId) const;

  void SetParent(ezGameObject* pObject, const ezGameObjectHandle& parent);

  void QueueMessage(ezMessage& msg, ezBitflags<ezGameObject::MsgRouting> routing, const ezGameObjectHandle& senderObject);

  ezResult RegisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc);
  ezResult RegisterUpdateFunctionWithDependency(const ezComponentManagerBase::UpdateFunctionDesc& desc, bool bInsertAsUnresolved);
  ezResult DeregisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc);

  void UpdateSynchronous(const ezArrayPtr<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions);
  void UpdateAsynchronous();
  void DeleteDeadObjects();
  void DeleteDeadComponents();
  void UpdateWorldTransforms();

  ezInternal::WorldData m_Data;
  typedef ezInternal::WorldData::ObjectStorage::Entry ObjectStorageEntry;

  ezUInt32 m_uiIndex;
  static ezStaticArray<ezWorld*, 64> s_Worlds;
};

#include <Core/World/Implementation/World_inl.h>
