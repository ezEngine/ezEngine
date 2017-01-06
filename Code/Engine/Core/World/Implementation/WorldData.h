#pragma once

#include <Foundation/Communication/MessageQueue.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>

#include <Foundation/Memory/BlockStorage.h>
#include <Foundation/Memory/CommonAllocators.h>

#include <Foundation/Threading/DelegateTask.h>

#include <Foundation/Types/UniquePtr.h>

#include <Foundation/Time/Clock.h>
#include <Foundation/Math/Random.h>

#include <Core/World/CoordinateSystem.h>
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

    static void UpdateGlobalTransform(ezGameObject::TransformationData* pData, float fInvDeltaSeconds);
    static void UpdateGlobalTransformWithParent(ezGameObject::TransformationData* pData, float fInvDeltaSeconds);

    void UpdateGlobalTransforms(float fInvDeltaSeconds);

    // game object lookups
    ezHashTable<ezUInt32, ezGameObjectId, ezHashHelper<ezUInt32>, ezLocalAllocatorWrapper> m_GlobalKeyToIdTable;
    ezHashTable<ezUInt32, ezHashedString, ezHashHelper<ezUInt32>, ezLocalAllocatorWrapper> m_IdToGlobalKeyTable;

    // modules
    ezDynamicArray<ezWorldModule*, ezLocalAllocatorWrapper> m_Modules;

    // component management
    ezDynamicArray<ezComponentManagerBase::ComponentStorageEntry, ezLocalAllocatorWrapper> m_DeadComponents;

    ezDynamicArray<ezComponentHandle, ezLocalAllocatorWrapper> m_ComponentsToInitialize;
    ezDynamicArray<ezComponentHandle, ezLocalAllocatorWrapper> m_ComponentsToStartSimulation;

    struct RegisteredUpdateFunction
    {
      ezWorldModule::UpdateFunction m_Function;
      ezHashedString m_sFunctionName;
      float m_fPriority;
      ezUInt16 m_uiGranularity;
      bool m_bOnlyUpdateWhenSimulating;

      void FillFromDesc(const ezWorldModule::UpdateFunctionDesc& desc);
      bool operator<(const RegisteredUpdateFunction& other) const;
    };

    struct UpdateTask : public ezTask
    {
      virtual void Execute() override;

      ezWorldModule::UpdateFunction m_Function;
      ezUInt32 m_uiStartIndex;
      ezUInt32 m_uiCount;
    };

    ezDynamicArray<RegisteredUpdateFunction, ezLocalAllocatorWrapper> m_UpdateFunctions[ezWorldModule::UpdateFunctionDesc::Phase::COUNT];
    ezDynamicArray<ezWorldModule::UpdateFunctionDesc, ezLocalAllocatorWrapper> m_UnresolvedUpdateFunctions;

    ezDynamicArray<UpdateTask*, ezLocalAllocatorWrapper> m_UpdateTasks;

    ezUniquePtr<ezCoordinateSystemProvider> m_pCoordinateSystemProvider;

    ezClock m_Clock;
    ezRandom m_Random;

    struct QueuedMsgMetaData
    {
      EZ_DECLARE_POD_TYPE();

      union
      {
        struct
        {
          ezUInt32 m_uiReceiverObject;
          ezObjectMsgRouting::Enum m_Routing;
        };

        struct
        {
          ezUInt64 m_uiReceiverComponent : 63;
          ezUInt64 m_uiReceiverIsComponent : 1;
        };

        ezUInt64 m_uiReceiverAndRouting;
      };

      ezTime m_Due;
    };

    typedef ezMessageQueue<QueuedMsgMetaData, ezLocalAllocatorWrapper> MessageQueue;
    MessageQueue m_MessageQueues[ezObjectMsgQueueType::COUNT];
    MessageQueue m_TimedMessageQueues[ezObjectMsgQueueType::COUNT];

    ezThreadID m_WriteThreadID;
    ezInt32 m_iWriteCounter;
    mutable ezAtomicInteger32 m_iReadCounter;

    bool m_bSimulateWorld;

  public:
    class ReadMarker
    {
    public:
      void Acquire();
      void Release();

    private:
      friend class ::ezInternal::WorldData;

      ReadMarker(const WorldData& data);
      const WorldData& m_Data;
    };

    class WriteMarker
    {
    public:
      void Acquire();
      void Release();

    private:
      friend class ::ezInternal::WorldData;

      WriteMarker(WorldData& data);
      WorldData& m_Data;
    };

  private:
    mutable ReadMarker m_ReadMarker;
    WriteMarker m_WriteMarker;

    void* m_pUserData;
  };
}

#include <Core/World/Implementation/WorldData_inl.h>

