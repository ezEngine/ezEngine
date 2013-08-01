#pragma once

#include <Core/World/GameObject.h>
#include <Core/World/Implementation/WorldData.h>

class EZ_CORE_DLL ezWorld
{
public:
  ezWorld(const char* szWorldName);
  ~ezWorld();

  const char* GetName() const;
  
  ezGameObjectHandle CreateObject(const ezGameObjectDesc& desc, 
    const ezGameObjectHandle& parent = ezGameObjectHandle());

  void CreateObjects(ezArrayPtr<ezGameObjectHandle> out_objects, const ezArrayPtr<const ezGameObjectDesc>& descs, 
    const ezGameObjectHandle& parent = ezGameObjectHandle());

  void DeleteObject(const ezGameObjectHandle& object);

  void DeleteObjects(const ezArrayPtr<const ezGameObjectHandle>& objects);

  bool IsValidObject(const ezGameObjectHandle& object) const;
  
  ezGameObject* GetObject(const ezGameObjectHandle& object) const;
  
  // slow access
  ezGameObject* GetObject(ezUInt64 uiPersistentId) const;
  ezGameObject* GetObject(const char* szObjectName) const;

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
  ComponentType* GetComponent(const ezComponentHandle& component) const;

  ezComponent* GetComponent(const ezComponentHandle& component) const;

  // update
  void Update();

  // memory
  ezIAllocator* GetAllocator();
  ezLargeBlockAllocator* GetBlockAllocator();

  /// \todo Hack (?)
  void* m_pUserData;

public:
  static ezUInt32 GetWorldCount();
  static ezWorld* GetWorld(ezUInt32 uiIndex);
  
private:
  friend class ezGameObject;
  friend class ezComponentManagerBase;

  void SetObjectName(ezGameObjectId internalId, const char* szName);
  const char* GetObjectName(ezGameObjectId internalId) const;

  void SetParent(ezGameObject* pObject, const ezGameObjectHandle& parent);

  ezResult RegisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc);
  ezResult RegisterUpdateFunctionWithDependency(const ezComponentManagerBase::UpdateFunctionDesc& desc, bool bInsertAsUnresolved);
  ezResult DeregisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc);

  void UpdateSynchronous(const ezArrayPtr<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions);
  void UpdateAsynchronous();
  void DeleteDeadObjects();
  void UpdateWorldTransforms();

  ezInternal::WorldData m_Data;

  ezUInt32 m_uiIndex;
  static ezStaticArray<ezWorld*, 64> s_Worlds;
};

#include <Core/World/Implementation/World_inl.h>
