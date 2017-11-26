#pragma once

#include <Foundation/Communication/MessageQueue.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Time/Clock.h>

#include <Core/World/GameObject.h>
#include <Core/World/WorldDesc.h>

namespace ezInternal
{
  class EZ_CORE_DLL WorldData
  {
    friend class ::ezWorld;
    friend class ::ezComponentManagerBase;

    WorldData(ezWorldDesc& desc);
    ~WorldData();

    ezHashedString m_sName;
    mutable ezProxyAllocator m_Allocator;
    ezLocalAllocatorWrapper m_AllocatorWrapper;
    ezInternal::WorldLargeBlockAllocator m_BlockAllocator;
    ezDoubleBufferedStackAllocator m_StackAllocator;

    enum
    {
      GAME_OBJECTS_PER_BLOCK = ezDataBlock<ezGameObject, ezInternal::DEFAULT_BLOCK_SIZE>::CAPACITY,
      TRANSFORMATION_DATA_PER_BLOCK = ezDataBlock<ezGameObject::TransformationData, ezInternal::DEFAULT_BLOCK_SIZE>::CAPACITY
    };

    // object storage
    typedef ezBlockStorage<ezGameObject, ezInternal::DEFAULT_BLOCK_SIZE, ezBlockStorageType::Compact> ObjectStorage;
    ezIdTable<ezGameObjectId, ezGameObject*, ezLocalAllocatorWrapper> m_Objects;
    ObjectStorage m_ObjectStorage;

    ezDynamicArray<ezGameObject*, ezLocalAllocatorWrapper> m_DeadObjects;

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

    static HierarchyType::Enum GetHierarchyType(bool bDynamic);

    ezGameObject::TransformationData* CreateTransformationData(bool bDynamic, ezUInt32 uiHierarchyLevel);

    void DeleteTransformationData(bool bDynamic, ezUInt32 uiHierarchyLevel, ezGameObject::TransformationData* pData);

    template <typename VISITOR>
    static ezVisitorExecution::Enum TraverseHierarchyLevel(Hierarchy::DataBlockArray& blocks, void* pUserData = nullptr);

    typedef ezDelegate<ezVisitorExecution::Enum(ezGameObject*)> VisitorFunc;
    void TraverseBreadthFirst(VisitorFunc& func);
    void TraverseDepthFirst(VisitorFunc& func);
    static ezVisitorExecution::Enum TraverseObjectDepthFirst(ezGameObject* pObject, VisitorFunc& func);

    static void UpdateGlobalTransform(ezGameObject::TransformationData* pData, const ezSimdFloat& fInvDeltaSeconds);
    static void UpdateGlobalTransformWithParent(ezGameObject::TransformationData* pData, const ezSimdFloat& fInvDeltaSeconds);

    void UpdateGlobalTransforms(float fInvDeltaSeconds);

    // game object lookups
    ezHashTable<ezUInt32, ezGameObjectId, ezHashHelper<ezUInt32>, ezLocalAllocatorWrapper> m_GlobalKeyToIdTable;
    ezHashTable<ezUInt32, ezHashedString, ezHashHelper<ezUInt32>, ezLocalAllocatorWrapper> m_IdToGlobalKeyTable;

    // modules
    ezDynamicArray<ezWorldModule*, ezLocalAllocatorWrapper> m_Modules;
    ezDynamicArray<ezWorldModule*, ezLocalAllocatorWrapper> m_ModulesToStartSimulation;

    // component management
    ezDynamicArray<ezComponent*, ezLocalAllocatorWrapper> m_DeadComponents;

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

    ezUniquePtr<ezSpatialSystem> m_pSpatialSystem;
    ezUniquePtr<ezCoordinateSystemProvider> m_pCoordinateSystemProvider;

    ezClock m_Clock;
    ezRandom m_Random;

    struct QueuedMsgMetaData
    {
      EZ_DECLARE_POD_TYPE();

      EZ_ALWAYS_INLINE QueuedMsgMetaData()
        : m_uiReceiverData(0)
      {
      }

      union
      {
        ezUInt32 m_uiReceiverObject;

        struct
        {
          ezUInt64 m_uiReceiverComponent : 62;
          ezUInt64 m_uiReceiverIsComponent : 1;
          ezUInt64 m_uiRecursive : 1;
        };

        ezUInt64 m_uiReceiverData;
      };

      ezTime m_Due;
    };

    typedef ezMessageQueue<QueuedMsgMetaData, ezLocalAllocatorWrapper> MessageQueue;
    mutable MessageQueue m_MessageQueues[ezObjectMsgQueueType::COUNT];
    mutable MessageQueue m_TimedMessageQueues[ezObjectMsgQueueType::COUNT];

    ezThreadID m_WriteThreadID;
    ezInt32 m_iWriteCounter;
    mutable ezAtomicInteger32 m_iReadCounter;

    bool m_bSimulateWorld;
    bool m_bReportErrorWhenStaticObjectMoves;

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

