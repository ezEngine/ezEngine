#include <Core/CorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/Implementation/WorldData.h>
#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Core/World/World.h>

#include <Foundation/Time/DefaultTimeStepSmoothing.h>

namespace ezInternal
{
  class DefaultCoordinateSystemProvider : public ezCoordinateSystemProvider
  {
  public:
    DefaultCoordinateSystemProvider()
      : ezCoordinateSystemProvider(nullptr)
    {
    }

    virtual void GetCoordinateSystem(const ezVec3& vGlobalPosition, ezCoordinateSystem& out_coordinateSystem) const override
    {
      EZ_IGNORE_UNUSED(vGlobalPosition);

      out_coordinateSystem.m_vForwardDir = ezVec3(1.0f, 0.0f, 0.0f);
      out_coordinateSystem.m_vRightDir = ezVec3(0.0f, 1.0f, 0.0f);
      out_coordinateSystem.m_vUpDir = ezVec3(0.0f, 0.0f, 1.0f);
    }
  };

  ////////////////////////////////////////////////////////////////////////////////////////////////////

  void WorldData::UpdateTask::Execute()
  {
    ezWorldModule::UpdateContext context;
    context.m_uiFirstComponentIndex = m_uiStartIndex;
    context.m_uiComponentCount = m_uiCount;

    m_Function(context);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////

  WorldData::WorldData(ezWorldDesc& desc)
    : m_sName(desc.m_sName)
    , m_Allocator(desc.m_sName, ezFoundation::GetDefaultAllocator())
    , m_AllocatorWrapper(&m_Allocator)
    , m_BlockAllocator(desc.m_sName, &m_Allocator)
    , m_StackAllocator(desc.m_sName, ezFoundation::GetAlignedAllocator())
    , m_ObjectStorage(&m_BlockAllocator, &m_Allocator)
    , m_MaxInitializationTimePerFrame(desc.m_MaxComponentInitializationTimePerFrame)
    , m_Clock(desc.m_sName)
    , m_WriteThreadID((ezThreadID)0)
    , m_bReportErrorWhenStaticObjectMoves(desc.m_bReportErrorWhenStaticObjectMoves)
    , m_ReadMarker(*this)
    , m_WriteMarker(*this)

  {
    m_AllocatorWrapper.Reset();

    if (desc.m_uiRandomNumberGeneratorSeed == 0)
    {
      m_Random.InitializeFromCurrentTime();
    }
    else
    {
      m_Random.Initialize(desc.m_uiRandomNumberGeneratorSeed);
    }

    // insert dummy entry to save some checks
    m_Objects.Insert(nullptr);

#if EZ_ENABLED(EZ_GAMEOBJECT_VELOCITY)
    static_assert(sizeof(ezGameObject::TransformationData) == 240);
#else
    static_assert(sizeof(ezGameObject::TransformationData) == 192);
#endif

    static_assert(sizeof(ezGameObject) == 128);
    static_assert(sizeof(QueuedMsgMetaData) == 16);
    static_assert(EZ_COMPONENT_TYPE_INDEX_BITS <= sizeof(ezWorldModuleTypeId) * 8);

    auto pDefaultInitBatch = EZ_NEW(&m_Allocator, InitBatch, &m_Allocator, "Default", true);
    pDefaultInitBatch->m_bIsReady = true;
    m_InitBatches.Insert(pDefaultInitBatch);
    m_pDefaultInitBatch = pDefaultInitBatch;
    m_pCurrentInitBatch = pDefaultInitBatch;

    m_pSpatialSystem = std::move(desc.m_pSpatialSystem);
    m_pCoordinateSystemProvider = desc.m_pCoordinateSystemProvider;

    if (m_pSpatialSystem == nullptr && desc.m_bAutoCreateSpatialSystem)
    {
      m_pSpatialSystem = EZ_NEW(ezFoundation::GetAlignedAllocator(), ezSpatialSystem_RegularGrid);
    }

    if (m_pCoordinateSystemProvider == nullptr)
    {
      m_pCoordinateSystemProvider = EZ_NEW(&m_Allocator, DefaultCoordinateSystemProvider);
    }

    if (m_pTimeStepSmoothing == nullptr)
    {
      m_pTimeStepSmoothing = EZ_NEW(&m_Allocator, ezDefaultTimeStepSmoothing);
    }

    m_Clock.SetTimeStepSmoothing(m_pTimeStepSmoothing.Borrow());

    // BEGIN-DOCS-CODE-SNIPPET: resource-management-listen-all
    // Listening to all resource events
    ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&WorldData::ResourceEventHandler, this));
    // END-DOCS-CODE-SNIPPET
  }

  WorldData::~WorldData()
  {
    ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&WorldData::ResourceEventHandler, this));
  }

  void WorldData::Clear()
  {
    // allow reading and writing during destruction
    m_WriteThreadID = ezThreadUtils::GetCurrentThreadID();
    m_iReadCounter.Increment();

    // deactivate all objects and components before destroying them
    for (auto it = m_ObjectStorage.GetIterator(); it.IsValid(); it.Next())
    {
      it->SetActiveFlag(false);
    }

    // deinitialize all modules before we invalidate the world. Components can still access the world during deinitialization.
    for (ezWorldModule* pModule : m_Modules)
    {
      if (pModule != nullptr)
      {
        pModule->Deinitialize();
      }
    }

    // now delete all modules
    for (ezWorldModule* pModule : m_Modules)
    {
      if (pModule != nullptr)
      {
        EZ_DELETE(&m_Allocator, pModule);
      }
    }
    m_Modules.Clear();

    // this deletes the ezGameObject instances
    m_ObjectStorage.Clear();

    // delete all transformation data
    for (ezUInt32 uiHierarchyIndex = 0; uiHierarchyIndex < HierarchyType::COUNT; ++uiHierarchyIndex)
    {
      Hierarchy& hierarchy = m_Hierarchies[uiHierarchyIndex];

      for (ezUInt32 i = hierarchy.m_Data.GetCount(); i-- > 0;)
      {
        Hierarchy::DataBlockArray* blocks = hierarchy.m_Data[i];
        for (ezUInt32 j = blocks->GetCount(); j-- > 0;)
        {
          m_BlockAllocator.DeallocateBlock((*blocks)[j]);
        }
        EZ_DELETE(&m_Allocator, blocks);
      }

      hierarchy.m_Data.Clear();
    }

    // delete task storage
    m_UpdateTasks.Clear();

    // delete queued messages
    for (ezUInt32 i = 0; i < ezObjectMsgQueueType::COUNT; ++i)
    {
      {
        MessageQueue& queue = m_MessageQueues[i];

        // The messages in this queue are allocated through a frame allocator and thus mustn't (and don't need to be) deallocated
        queue.Clear();
      }

      {
        MessageQueue& queue = m_TimedMessageQueues[i];
        while (!queue.IsEmpty())
        {
          MessageQueue::Entry& entry = queue.Peek();
          EZ_DELETE(&m_Allocator, entry.m_pMessage);

          queue.Dequeue();
        }
      }
    }
  }

  ezGameObject::TransformationData* WorldData::CreateTransformationData(bool bDynamic, ezUInt32 uiHierarchyLevel)
  {
    Hierarchy& hierarchy = m_Hierarchies[GetHierarchyType(bDynamic)];

    while (uiHierarchyLevel >= hierarchy.m_Data.GetCount())
    {
      hierarchy.m_Data.PushBack(EZ_NEW(&m_Allocator, Hierarchy::DataBlockArray, &m_Allocator));
    }

    Hierarchy::DataBlockArray& blocks = *hierarchy.m_Data[uiHierarchyLevel];
    Hierarchy::DataBlock* pBlock = nullptr;

    if (!blocks.IsEmpty())
    {
      pBlock = &blocks.PeekBack();
    }

    if (pBlock == nullptr || pBlock->IsFull())
    {
      blocks.PushBack(m_BlockAllocator.AllocateBlock<ezGameObject::TransformationData>());
      pBlock = &blocks.PeekBack();
    }

    return pBlock->ReserveBack();
  }

  void WorldData::DeleteTransformationData(bool bDynamic, ezUInt32 uiHierarchyLevel, ezGameObject::TransformationData* pData)
  {
    Hierarchy& hierarchy = m_Hierarchies[GetHierarchyType(bDynamic)];
    Hierarchy::DataBlockArray& blocks = *hierarchy.m_Data[uiHierarchyLevel];

    Hierarchy::DataBlock& lastBlock = blocks.PeekBack();
    const ezGameObject::TransformationData* pLast = lastBlock.PopBack();

    if (pData != pLast)
    {
      ezMemoryUtils::Copy(pData, pLast, 1);
      pData->m_pObject->m_pTransformationData = pData;

      // fix parent transform data for children as well
      auto it = pData->m_pObject->GetChildren();
      while (it.IsValid())
      {
        auto pTransformData = it->m_pTransformationData;
        pTransformData->m_pParentData = pData;
        it.Next();
      }
    }

    if (lastBlock.IsEmpty())
    {
      m_BlockAllocator.DeallocateBlock(lastBlock);
      blocks.PopBack();
    }
  }

  void WorldData::TraverseBreadthFirst(VisitorFunc& func)
  {
    struct Helper
    {
      EZ_ALWAYS_INLINE static ezVisitorExecution::Enum Visit(ezGameObject::TransformationData* pData, void* pUserData) { return (*static_cast<VisitorFunc*>(pUserData))(pData->m_pObject); }
    };

    const ezUInt32 uiMaxHierarchyLevel = ezMath::Max(m_Hierarchies[HierarchyType::Static].m_Data.GetCount(), m_Hierarchies[HierarchyType::Dynamic].m_Data.GetCount());

    for (ezUInt32 uiHierarchyLevel = 0; uiHierarchyLevel < uiMaxHierarchyLevel; ++uiHierarchyLevel)
    {
      for (ezUInt32 uiHierarchyIndex = 0; uiHierarchyIndex < HierarchyType::COUNT; ++uiHierarchyIndex)
      {
        Hierarchy& hierarchy = m_Hierarchies[uiHierarchyIndex];
        if (uiHierarchyLevel < hierarchy.m_Data.GetCount())
        {
          ezVisitorExecution::Enum execution = TraverseHierarchyLevel<Helper>(*hierarchy.m_Data[uiHierarchyLevel], &func);
          EZ_ASSERT_DEV(execution != ezVisitorExecution::Skip, "Skip is not supported when using breadth first traversal");
          if (execution == ezVisitorExecution::Stop)
            return;
        }
      }
    }
  }

  void WorldData::TraverseDepthFirst(VisitorFunc& func)
  {
    struct Helper
    {
      EZ_ALWAYS_INLINE static ezVisitorExecution::Enum Visit(ezGameObject::TransformationData* pData, void* pUserData) { return WorldData::TraverseObjectDepthFirst(pData->m_pObject, *static_cast<VisitorFunc*>(pUserData)); }
    };

    for (ezUInt32 uiHierarchyIndex = 0; uiHierarchyIndex < HierarchyType::COUNT; ++uiHierarchyIndex)
    {
      Hierarchy& hierarchy = m_Hierarchies[uiHierarchyIndex];
      if (!hierarchy.m_Data.IsEmpty())
      {
        if (TraverseHierarchyLevel<Helper>(*hierarchy.m_Data[0], &func) == ezVisitorExecution::Stop)
          return;
      }
    }
  }

  // static
  ezVisitorExecution::Enum WorldData::TraverseObjectDepthFirst(ezGameObject* pObject, VisitorFunc& func)
  {
    ezVisitorExecution::Enum execution = func(pObject);
    if (execution == ezVisitorExecution::Stop)
      return ezVisitorExecution::Stop;

    if (execution != ezVisitorExecution::Skip) // skip all children
    {
      for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
      {
        if (TraverseObjectDepthFirst(it, func) == ezVisitorExecution::Stop)
          return ezVisitorExecution::Stop;
      }
    }

    return ezVisitorExecution::Continue;
  }

  void WorldData::UpdateGlobalTransforms()
  {
    struct UserData
    {
      ezSpatialSystem* m_pSpatialSystem;
      ezUInt32 m_uiUpdateCounter;
    };

    UserData userData;
    userData.m_pSpatialSystem = m_pSpatialSystem.Borrow();
    userData.m_uiUpdateCounter = m_uiUpdateCounter;

    struct RootLevel
    {
      EZ_ALWAYS_INLINE static ezVisitorExecution::Enum Visit(ezGameObject::TransformationData* pData, void* pUserData0)
      {
        auto pUserData = static_cast<const UserData*>(pUserData0);
        WorldData::UpdateGlobalTransform(pData, pUserData->m_uiUpdateCounter);
        return ezVisitorExecution::Continue;
      }
    };

    struct WithParent
    {
      EZ_ALWAYS_INLINE static ezVisitorExecution::Enum Visit(ezGameObject::TransformationData* pData, void* pUserData0)
      {
        auto pUserData = static_cast<const UserData*>(pUserData0);
        WorldData::UpdateGlobalTransformWithParent(pData, pUserData->m_uiUpdateCounter);
        return ezVisitorExecution::Continue;
      }
    };

    struct RootLevelWithSpatialData
    {
      EZ_ALWAYS_INLINE static ezVisitorExecution::Enum Visit(ezGameObject::TransformationData* pData, void* pUserData0)
      {
        auto pUserData = static_cast<UserData*>(pUserData0);
        WorldData::UpdateGlobalTransformAndSpatialData(pData, pUserData->m_uiUpdateCounter, *pUserData->m_pSpatialSystem);
        return ezVisitorExecution::Continue;
      }
    };

    struct WithParentWithSpatialData
    {
      EZ_ALWAYS_INLINE static ezVisitorExecution::Enum Visit(ezGameObject::TransformationData* pData, void* pUserData0)
      {
        auto pUserData = static_cast<UserData*>(pUserData0);
        WorldData::UpdateGlobalTransformWithParentAndSpatialData(pData, pUserData->m_uiUpdateCounter, *pUserData->m_pSpatialSystem);
        return ezVisitorExecution::Continue;
      }
    };

    Hierarchy& hierarchy = m_Hierarchies[HierarchyType::Dynamic];
    if (!hierarchy.m_Data.IsEmpty())
    {
      auto dataPtr = hierarchy.m_Data.GetData();

      // If we have no spatial system, we perform multi-threaded update as we do not
      // have to acquire a write lock in the process.
      if (m_pSpatialSystem == nullptr)
      {
        TraverseHierarchyLevelMultiThreaded<RootLevel>(*dataPtr[0], &userData);

        for (ezUInt32 i = 1; i < hierarchy.m_Data.GetCount(); ++i)
        {
          TraverseHierarchyLevelMultiThreaded<WithParent>(*dataPtr[i], &userData);
        }
      }
      else
      {
        TraverseHierarchyLevel<RootLevelWithSpatialData>(*dataPtr[0], &userData);

        for (ezUInt32 i = 1; i < hierarchy.m_Data.GetCount(); ++i)
        {
          TraverseHierarchyLevel<WithParentWithSpatialData>(*dataPtr[i], &userData);
        }
      }
    }
  }

  void WorldData::ResourceEventHandler(const ezResourceEvent& e)
  {
    if (e.m_Type != ezResourceEvent::Type::ResourceContentUnloading || e.m_pResource->GetReferenceCount() == 0)
      return;

    ezTypelessResourceHandle hResource(e.m_pResource);
    if (m_ReloadFunctions.Contains(hResource))
    {
      m_NeedReload.Insert(hResource);
    }
  }

} // namespace ezInternal
