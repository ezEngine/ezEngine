#pragma once

#include <Foundation/Communication/MessageQueue.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>

#include <Foundation/Memory/BlockStorage.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Threading/TaskSystem.h>

#include <Core/World/GameObject.h>

namespace ezInternal
{
  class EZ_CORE_DLL WorldData
  {
    friend class ::ezWorld;
    friend class ::ezComponentManagerBase;

    WorldData(const char* szWorldName);
    ~WorldData();

    ezHashedString m_sName;
    ezProxyAllocator m_Allocator;
    ezLocalAllocatorWrapper m_AllocatorWrapper;
    ezInternal::WorldLargeBlockAllocator m_BlockAllocator;

    enum
    {
      GAME_OBJECTS_PER_BLOCK = ezDataBlock<ezGameObject, ezInternal::DEFAULT_BLOCK_SIZE>::CAPACITY,
      TRANSFORMATION_DATA_PER_BLOCK = ezDataBlock<ezGameObject::TransformationData, ezInternal::DEFAULT_BLOCK_SIZE>::CAPACITY
    };

    // object storage
    typedef ezBlockStorage<ezGameObject, ezInternal::DEFAULT_BLOCK_SIZE, true> ObjectStorage;
    ezIdTable<ezGameObjectId, ObjectStorage::Entry, ezLocalAllocatorWrapper> m_Objects;
    ObjectStorage m_ObjectStorage;

    ezDynamicArray<ObjectStorage::Entry, ezLocalAllocatorWrapper> m_DeadObjects;

    struct SetParentRequest
    {
      ezGameObjectHandle m_Object;
      ezGameObjectHandle m_NewParent;
    };

    ezDynamicArray<SetParentRequest, ezLocalAllocatorWrapper> m_SetParentRequests;

    // hierarchy structures
    struct Hierarchy
    {
      typedef ezDataBlock<ezGameObject::TransformationData, ezInternal::DEFAULT_BLOCK_SIZE> DataBlock;
      typedef ezDynamicArray<DataBlock> DataBlockArray;

      ezHybridArray<DataBlockArray*, 8, ezLocalAllocatorWrapper> m_Data;
    };

    struct HierarchyType
    {
      enum Enum
      {
        Static,
        Dynamic,
        COUNT
      };
    };

    Hierarchy m_Hierarchies[HierarchyType::COUNT];

    ezUInt32 CreateTransformationData(const ezBitflags<ezObjectFlags>& objectFlags, ezUInt32 uiHierarchyLevel,
      ezGameObject::TransformationData*& out_pData);

    void DeleteTransformationData(const ezBitflags<ezObjectFlags>& objectFlags, ezUInt32 uiHierarchyLevel, 
      ezUInt32 uiIndex);

    template <typename VISITOR>
    static bool TraverseHierarchyLevel(Hierarchy::DataBlockArray& blocks, void* pUserData = nullptr);

    typedef ezDelegate<bool(ezGameObject*)> VisitorFunc;
    void TraverseBreadthFirst(VisitorFunc& func);
    void TraverseDepthFirst(VisitorFunc& func);
    static bool TraverseObjectDepthFirst(ezGameObject* pObject, VisitorFunc& func);

    static void UpdateWorldTransform(ezGameObject::TransformationData* pData, float fInvDeltaSeconds);
    static void UpdateWorldTransformWithParent(ezGameObject::TransformationData* pData, float fInvDeltaSeconds);

    void UpdateWorldTransforms();

    // game object lookups
    /// \todo
    //ezHashTable<ezUInt64, ezGameObjectId, ezHashHelper<ezUInt64>, ezLocalAllocatorWrapper> m_PersistentToInternalTable;

    // component manager
    ezDynamicArray<ezComponentManagerBase*, ezLocalAllocatorWrapper> m_ComponentManagers;

    ezDynamicArray<ezComponentManagerBase::ComponentStorageEntry, ezLocalAllocatorWrapper> m_DeadComponents;

    typedef ezComponentManagerBase::UpdateFunction UpdateFunction;
    struct RegisteredUpdateFunction
    {
      EZ_DECLARE_POD_TYPE();

      UpdateFunction m_Function;
      const char* m_szFunctionName;
      ezUInt32 m_uiGranularity;
    };
  
    struct UpdateTask : public ezTask
    {
      virtual void Execute() override;

      UpdateFunction m_Function;
      ezUInt32 m_uiStartIndex;
      ezUInt32 m_uiCount;
    };

    ezProfilingId m_UpdateProfilingID;

    ezDynamicArray<RegisteredUpdateFunction, ezLocalAllocatorWrapper> m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::PHASE_COUNT];
    ezDynamicArray<ezComponentManagerBase::UpdateFunctionDesc, ezLocalAllocatorWrapper> m_UnresolvedUpdateFunctions;

    ezDynamicArray<UpdateTask*, ezLocalAllocatorWrapper> m_UpdateTasks;

    struct QueuedMsgMetaData
    {
      ezGameObjectHandle m_ReceiverObject;
      ezObjectMsgRouting::Enum m_Routing;
      ezTime m_Due;
    };

    typedef ezMessageQueue<QueuedMsgMetaData, ezLocalAllocatorWrapper> MessageQueue;
    MessageQueue m_MessageQueues[ezObjectMsgQueueType::COUNT];

    ezThreadID m_ThreadID;
    bool m_bIsInAsyncPhase;

    void* m_pUserData;
  };
}

#include <Core/World/Implementation/WorldData_inl.h>

