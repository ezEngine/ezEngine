#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Containers/StaticArray.h>

#include <Foundation/Memory/BlockStorage.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Memory/LargeBlockAllocator.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/TaskSystem.h>

#include <Core/World/GameObject.h>

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

  void GetMemStats(ezIAllocator::Stats& stats) const;

public:

  static ezUInt32 GetWorldCount();
  static ezWorld* GetWorld(ezUInt32 uiIndex);
  
private:
  friend class ezGameObject;
  friend class ezComponentManagerBase;

  void SetName(ezGameObjectId internalId, const char* szName);
  const char* GetName(ezGameObjectId internalId) const;

  void SetParent(ezGameObject* pObject, const ezGameObjectHandle& parent);

  ezString m_Name;
  ezProxyAllocator m_Allocator;
  ezLocalAllocatorWrapper m_AllocatorWrapper;
  ezLargeBlockAllocator m_BlockAllocator;

  enum
  {
    GAME_OBJECTS_PER_BLOCK = ezDataBlock<ezGameObject>::CAPACITY,
    HIERARCHICAL_DATA_PER_BLOCK = ezDataBlock<ezGameObject::HierarchicalData>::CAPACITY
  };

  // object storage
  ezIdTable<ezGameObjectId, ezGameObject*, ezLocalAllocatorWrapper> m_Objects;
  ezBlockStorage<ezGameObject> m_ObjectStorage;

  ezDynamicArray<ezGameObject*, ezLocalAllocatorWrapper> m_DeadObjects;

  // hierarchy structures
  struct Hierarchy
  {
    typedef ezDataBlock<ezGameObject::HierarchicalData> DataBlock;
    typedef ezDynamicArray<DataBlock> DataBlockArray;

    Hierarchy();
    ~Hierarchy();

    ezUInt32 ReserveData(ezArrayPtr<ezGameObject::HierarchicalData*> out_data, ezUInt32 uiHierarchyLevel, 
      ezUInt32 uiFirstIndex = ezInvalidIndex);

    void ReserveWholeBlocks(DataBlockArray& hierarchyBlocks, ezUInt32 uiCount,
      ezArrayPtr<ezGameObject::HierarchicalData*> out_data, ezUInt32 uiOutIndex = 0);
    
    void RemoveData(ezUInt32 uiHierarchyLevel, 
      ezUInt32 uiFirstIndex = ezInvalidIndex, ezUInt32 uiCount = 1);   

    ezHybridArray<DataBlockArray*, 8, ezLocalAllocatorWrapper> m_data;

    ezWorld* m_pWorld;
  };

  enum HierarchyType { HIERARCHY_STATIC, HIERARCHY_DYNAMIC, HIERARCHY_INACTIVE, HIERARCHY_COUNT };

  Hierarchy m_hierarchies[HIERARCHY_COUNT];

  static HierarchyType GetHierarchyType(const ezBitflags<ezGameObjectFlags>& objectFlags);

  // game object lookups
  ezHashTable<ezUInt64, ezGameObjectId, ezHashHelper<ezUInt64>, ezLocalAllocatorWrapper> m_PersistentToInternalTable;
  ezHashTable<ezGameObjectId, ezString, ezHashHelper<ezGameObjectId>, ezLocalAllocatorWrapper> m_InternalToNameTable;
  ezHashTable<const char*, ezGameObjectId, ezHashHelper<const char*>, ezLocalAllocatorWrapper> m_NameToInternalTable;

  // component manager
  ezDynamicArray<ezComponentManagerBase*, ezLocalAllocatorWrapper> m_ComponentManagers;

  typedef ezComponentManagerBase::UpdateFunction UpdateFunction;
  struct RegisteredUpdateFunction
  {
    EZ_DECLARE_POD_TYPE();

    UpdateFunction m_Function;
    ezUInt32 m_uiGranularity;
  };
  
  struct UpdateTask : public ezTask
  {
    virtual void Execute() EZ_OVERRIDE;

    UpdateFunction m_Function;
    ezUInt32 m_uiStartIndex;
    ezUInt32 m_uiCount;
  };

  ezDynamicArray<RegisteredUpdateFunction, ezLocalAllocatorWrapper> m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::PHASE_COUNT];
  ezDynamicArray<ezComponentManagerBase::UpdateFunctionDesc, ezLocalAllocatorWrapper> m_UnresolvedUpdateFunctions;

  ezDynamicArray<UpdateTask*, ezLocalAllocatorWrapper> m_UpdateTasks; // TODO: store tasks inplace

  ezResult RegisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc);
  ezResult RegisterUpdateFunctionWithDependency(const ezComponentManagerBase::UpdateFunctionDesc& desc, bool bInsertAsUnresolved);

  ezResult DeregisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc);

  void UpdateSynchronous(const ezArrayPtr<RegisteredUpdateFunction>& updateFunctions);
  void UpdateAsynchronous();
  void DeleteDeadObjects();
  void UpdateWorldTransforms();

  ezUInt32 m_uiIndex;

  static ezStaticArray<ezWorld*, 64> s_Worlds;
};

#include <Core/World/Implementation/World_inl.h>
