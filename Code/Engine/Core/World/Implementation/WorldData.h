#pragma once

#include <Foundation/Communication/MessageQueue.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Types/SharedPtr.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/GameObject.h>
#include <Core/World/WorldDesc.h>

namespace ezInternal
{
  class EZ_CORE_DLL WorldData
  {
  private:
    friend class ::ezWorld;
    friend class ::ezComponentManagerBase;

    WorldData(ezWorldDesc& desc);
    ~WorldData();

    void Clear();

    ezHashedString m_sName;
    mutable ezProxyAllocator m_Allocator;
    ezLocalAllocatorWrapper m_AllocatorWrapper;
    ezInternal::WorldLargeBlockAllocator m_BlockAllocator;
    ezDoubleBufferedLinearAllocator m_StackAllocator;

    enum
    {
      GAME_OBJECTS_PER_BLOCK = ezDataBlock<ezGameObject, ezInternal::DEFAULT_BLOCK_SIZE>::CAPACITY,
      TRANSFORMATION_DATA_PER_BLOCK = ezDataBlock<ezGameObject::TransformationData, ezInternal::DEFAULT_BLOCK_SIZE>::CAPACITY
    };

    // object storage
    using ObjectStorage = ezBlockStorage<ezGameObject, ezInternal::DEFAULT_BLOCK_SIZE, ezBlockStorageType::Compact>;
    ezIdTable<ezGameObjectId, ezGameObject*, ezLocalAllocatorWrapper> m_Objects;
    ObjectStorage m_ObjectStorage;

    ezSet<ezGameObject*, ezCompareHelper<ezGameObject*>, ezLocalAllocatorWrapper> m_DeadObjects;
    ezEvent<const ezGameObject*> m_ObjectDeletionEvent;

  public:
    class EZ_CORE_DLL ConstObjectIterator
    {
    public:
      const ezGameObject& operator*() const;
      const ezGameObject* operator->() const;

      operator const ezGameObject*() const;

      /// \brief Advances the iterator to the next object. The iterator will not be valid anymore, if the last object is reached.
      void Next();

      /// \brief Checks whether this iterator points to a valid object.
      bool IsValid() const;

      /// \brief Shorthand for 'Next'
      void operator++();

    private:
      friend class ::ezWorld;

      ConstObjectIterator(ObjectStorage::ConstIterator iterator);

      ObjectStorage::ConstIterator m_Iterator;
    };

    class EZ_CORE_DLL ObjectIterator
    {
    public:
      ezGameObject& operator*();
      ezGameObject* operator->();

      operator ezGameObject*();

      /// \brief Advances the iterator to the next object. The iterator will not be valid anymore, if the last object is reached.
      void Next();

      /// \brief Checks whether this iterator points to a valid object.
      bool IsValid() const;

      /// \brief Shorthand for 'Next'
      void operator++();

    private:
      friend class ::ezWorld;

      ObjectIterator(ObjectStorage::Iterator iterator);

      ObjectStorage::Iterator m_Iterator;
    };

  private:
    // hierarchy structures
    struct Hierarchy
    {
      using DataBlock = ezDataBlock<ezGameObject::TransformationData, ezInternal::DEFAULT_BLOCK_SIZE>;
      using DataBlockArray = ezDynamicArray<DataBlock>;

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
    template <typename VISITOR>
    ezVisitorExecution::Enum TraverseHierarchyLevelMultiThreaded(Hierarchy::DataBlockArray& blocks, void* pUserData = nullptr);

    using VisitorFunc = ezDelegate<ezVisitorExecution::Enum(ezGameObject*)>;
    void TraverseBreadthFirst(VisitorFunc& func);
    void TraverseDepthFirst(VisitorFunc& func);
    static ezVisitorExecution::Enum TraverseObjectDepthFirst(ezGameObject* pObject, VisitorFunc& func);

    static void UpdateGlobalTransform(ezGameObject::TransformationData* pData, ezUInt32 uiUpdateCounter);
    static void UpdateGlobalTransformWithParent(ezGameObject::TransformationData* pData, ezUInt32 uiUpdateCounter);

    static void UpdateGlobalTransformAndSpatialData(ezGameObject::TransformationData* pData, ezUInt32 uiUpdateCounter, ezSpatialSystem& spatialSystem);
    static void UpdateGlobalTransformWithParentAndSpatialData(ezGameObject::TransformationData* pData, ezUInt32 uiUpdateCounter, ezSpatialSystem& spatialSystem);

    void UpdateGlobalTransforms();

    void ResourceEventHandler(const ezResourceEvent& e);

    // game object lookups
    ezHashTable<ezUInt64, ezGameObjectId, ezHashHelper<ezUInt64>, ezLocalAllocatorWrapper> m_GlobalKeyToIdTable;
    ezHashTable<ezUInt64, ezHashedString, ezHashHelper<ezUInt64>, ezLocalAllocatorWrapper> m_IdToGlobalKeyTable;

    // modules
    ezDynamicArray<ezWorldModule*, ezLocalAllocatorWrapper> m_Modules;
    ezDynamicArray<ezWorldModule*, ezLocalAllocatorWrapper> m_ModulesToStartSimulation;

    // component management
    ezSet<ezComponent*, ezCompareHelper<ezComponent*>, ezLocalAllocatorWrapper> m_DeadComponents;

    struct InitBatch
    {
      InitBatch(ezAllocator* pAllocator, ezStringView sName, bool bMustFinishWithinOneFrame);

      ezHashedString m_sName;
      bool m_bMustFinishWithinOneFrame = true;
      bool m_bIsReady = false;

      ezUInt32 m_uiNextComponentToInitialize = 0;
      ezUInt32 m_uiNextComponentToStartSimulation = 0;
      ezDynamicArray<ezComponentHandle> m_ComponentsToInitialize;
      ezDynamicArray<ezComponentHandle> m_ComponentsToStartSimulation;
    };

    ezTime m_MaxInitializationTimePerFrame;
    ezIdTable<ezComponentInitBatchId, ezUniquePtr<InitBatch>, ezLocalAllocatorWrapper> m_InitBatches;
    InitBatch* m_pDefaultInitBatch = nullptr;
    InitBatch* m_pCurrentInitBatch = nullptr;

    struct RegisteredUpdateFunction
    {
      ezWorldModule::UpdateFunction m_Function;
      ezHashedString m_sFunctionName;
      float m_fPriority;
      ezUInt16 m_uiAsyncPhaseBatchSize;
      bool m_bOnlyUpdateWhenSimulating;

      void FillFromDesc(const ezWorldModule::UpdateFunctionDesc& desc);
      bool operator<(const RegisteredUpdateFunction& other) const;
    };

    struct UpdateTask final : public ezTask
    {
      virtual void Execute() override;

      ezWorldModule::UpdateFunction m_Function;
      ezUInt32 m_uiStartIndex;
      ezUInt32 m_uiCount;
    };

    ezDynamicArray<RegisteredUpdateFunction, ezLocalAllocatorWrapper> m_UpdateFunctions[ezWorldUpdatePhase::COUNT];
    ezDynamicArray<ezWorldModule::UpdateFunctionDesc, ezLocalAllocatorWrapper> m_UpdateFunctionsToRegister;

    ezDynamicArray<ezSharedPtr<UpdateTask>, ezLocalAllocatorWrapper> m_UpdateTasks;

    ezUniquePtr<ezSpatialSystem> m_pSpatialSystem;
    ezSharedPtr<ezCoordinateSystemProvider> m_pCoordinateSystemProvider;
    ezUniquePtr<ezTimeStepSmoothing> m_pTimeStepSmoothing;

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
        struct
        {
          ezUInt64 m_uiReceiverObjectOrComponent : 62;
          ezUInt64 m_uiReceiverIsComponent : 1;
          ezUInt64 m_uiRecursive : 1;
        };

        ezUInt64 m_uiReceiverData;
      };

      ezTime m_Due;
    };

    using MessageQueue = ezMessageQueue<QueuedMsgMetaData, ezLocalAllocatorWrapper>;
    mutable MessageQueue m_MessageQueues[ezObjectMsgQueueType::COUNT];
    mutable MessageQueue m_TimedMessageQueues[ezObjectMsgQueueType::COUNT];
    ezObjectMsgQueueType::Enum m_ProcessingMessageQueue = ezObjectMsgQueueType::COUNT;

    ezThreadID m_WriteThreadID;
    ezInt32 m_iWriteCounter = 0;
    mutable ezAtomicInteger32 m_iReadCounter;

    ezUInt32 m_uiUpdateCounter = 0;
    bool m_bSimulateWorld = true;
    bool m_bReportErrorWhenStaticObjectMoves = true;

    /// \brief Maps some data (given as void*) to an ezGameObjectHandle. Only available in special situations (e.g. editor use cases).
    ezDelegate<ezGameObjectHandle(const void*, ezComponentHandle, ezStringView)> m_GameObjectReferenceResolver;

    struct ResourceReloadContext
    {
      ezWorld* m_pWorld = nullptr;
      ezComponent* m_pComponent = nullptr;
      void* m_pUserData = nullptr;
    };

    using ResourceReloadFunc = ezDelegate<void(ResourceReloadContext&)>;

    struct ResourceReloadFunctionData
    {
      ezComponentHandle m_hComponent;
      void* m_pUserData = nullptr;
      ResourceReloadFunc m_Func;
    };

    using ReloadFunctionList = ezHybridArray<ResourceReloadFunctionData, 8>;
    ezHashTable<ezTypelessResourceHandle, ReloadFunctionList> m_ReloadFunctions;
    ezHashSet<ezTypelessResourceHandle> m_NeedReload;
    ReloadFunctionList m_TempReloadFunctions;

  public:
    class ReadMarker
    {
    public:
      void Lock();
      void Unlock();

    private:
      friend class ::ezInternal::WorldData;

      ReadMarker(const WorldData& data);
      const WorldData& m_Data;
    };

    class WriteMarker
    {
    public:
      void Lock();
      void Unlock();

    private:
      friend class ::ezInternal::WorldData;

      WriteMarker(WorldData& data);
      WorldData& m_Data;
    };

  private:
    mutable ReadMarker m_ReadMarker;
    WriteMarker m_WriteMarker;

    void* m_pUserData = nullptr;
  };
} // namespace ezInternal

#include <Core/World/Implementation/WorldData_inl.h>
