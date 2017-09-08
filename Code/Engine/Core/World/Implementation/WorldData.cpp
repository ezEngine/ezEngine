#include <PCH.h>

#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Core/World/World.h>


namespace ezInternal
{
  class DefaultCoordinateSystemProvider : public ezCoordinateSystemProvider
  {
  public:
    DefaultCoordinateSystemProvider() : ezCoordinateSystemProvider(nullptr)
    {
    }

    virtual void GetCoordinateSystem(const ezVec3& vGlobalPosition, ezCoordinateSystem& out_CoordinateSystem) const override
    {
      out_CoordinateSystem.m_vForwardDir = ezVec3(1.0f, 0.0f, 0.0f);
      out_CoordinateSystem.m_vRightDir   = ezVec3(0.0f, 1.0f, 0.0f);
      out_CoordinateSystem.m_vUpDir      = ezVec3(0.0f, 0.0f, 1.0f);
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
  , m_StackAllocator(ezFoundation::GetAlignedAllocator())
  , m_ObjectStorage(&m_BlockAllocator, &m_Allocator)
  , m_Clock(desc.m_sName)
  , m_WriteThreadID((ezThreadID)0)
  , m_iWriteCounter(0)
  , m_bSimulateWorld(true)
  , m_ReadMarker(*this)
  , m_WriteMarker(*this)
  , m_pUserData(nullptr)
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

  EZ_CHECK_AT_COMPILETIME(sizeof(ezGameObject::TransformationData) == 256);
  //EZ_CHECK_AT_COMPILETIME(sizeof(ezGameObject) == 128); /// \todo get game object size back to 128
  EZ_CHECK_AT_COMPILETIME(sizeof(QueuedMsgMetaData) == 16);

  m_pSpatialSystem = std::move(desc.m_pSpatialSystem);
  m_pCoordinateSystemProvider = std::move(desc.m_pCoordinateSystemProvider);

  if (m_pSpatialSystem == nullptr)
  {
    m_pSpatialSystem = EZ_NEW(ezFoundation::GetAlignedAllocator(), ezSpatialSystem_RegularGrid);
  }

  if (m_pCoordinateSystemProvider == nullptr)
  {
    m_pCoordinateSystemProvider = EZ_NEW(&m_Allocator, DefaultCoordinateSystemProvider);
  }
}

WorldData::~WorldData()
{
  EZ_ASSERT_DEV(m_Modules.IsEmpty(), "Modules should be cleaned up already.");

  // delete all transformation data
  for (ezUInt32 uiHierarchyIndex = 0; uiHierarchyIndex < HierarchyType::COUNT; ++uiHierarchyIndex)
  {
    Hierarchy& hierarchy = m_Hierarchies[uiHierarchyIndex];

    for (ezUInt32 i = hierarchy.m_Data.GetCount(); i-- > 0; )
    {
      Hierarchy::DataBlockArray* blocks = hierarchy.m_Data[i];
      for (ezUInt32 j = blocks->GetCount(); j-- > 0; )
      {
        m_BlockAllocator.DeallocateBlock((*blocks)[j]);
      }
      EZ_DELETE(&m_Allocator, blocks);
    }
  }

  // delete task storage
  for (ezUInt32 i = 0; i < m_UpdateTasks.GetCount(); ++i)
  {
    EZ_DELETE(&m_Allocator, m_UpdateTasks[i]);
  }

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

  if (uiHierarchyLevel >= hierarchy.m_Data.GetCount())
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
    EZ_ALWAYS_INLINE static ezVisitorExecution::Enum Visit(ezGameObject::TransformationData* pData, void* pUserData)
    {
      return (*static_cast<VisitorFunc*>(pUserData))(pData->m_pObject);
    }
  };

  const ezUInt32 uiMaxHierarchyLevel = ezMath::Max(m_Hierarchies[HierarchyType::Static].m_Data.GetCount(),
    m_Hierarchies[HierarchyType::Dynamic].m_Data.GetCount());

  for (ezUInt32 uiHierarchyLevel = 0; uiHierarchyLevel < uiMaxHierarchyLevel; ++uiHierarchyLevel)
  {
    for (ezUInt32 uiHierarchyIndex = 0; uiHierarchyIndex < HierarchyType::COUNT; ++uiHierarchyIndex)
    {
      Hierarchy& hierarchy = m_Hierarchies[uiHierarchyIndex];
      if (uiHierarchyLevel < hierarchy.m_Data.GetCount())
      {
        if (TraverseHierarchyLevel<Helper>(*hierarchy.m_Data[uiHierarchyLevel], &func) == ezVisitorExecution::Stop)
          return;
      }
    }
  }
}

void WorldData::TraverseDepthFirst(VisitorFunc& func)
{
  struct Helper
  {
    EZ_ALWAYS_INLINE static ezVisitorExecution::Enum Visit(ezGameObject::TransformationData* pData, void* pUserData)
    {
      return WorldData::TraverseObjectDepthFirst(pData->m_pObject, *static_cast<VisitorFunc*>(pUserData));
    }
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
  if (func(pObject) == ezVisitorExecution::Stop)
    return ezVisitorExecution::Stop;

  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    if (TraverseObjectDepthFirst(it, func) == ezVisitorExecution::Stop)
      return ezVisitorExecution::Stop;
  }

  return ezVisitorExecution::Continue;
}

void WorldData::UpdateGlobalTransforms(float fInvDeltaSeconds)
{
  struct RootLevel
  {
    EZ_ALWAYS_INLINE static ezVisitorExecution::Enum Visit(ezGameObject::TransformationData* pData, void* pUserData)
    {
      WorldData::UpdateGlobalTransform(pData, *static_cast<ezSimdFloat*>(pUserData));
      return ezVisitorExecution::Continue;
    }
  };

  struct WithParent
  {
    EZ_ALWAYS_INLINE static ezVisitorExecution::Enum Visit(ezGameObject::TransformationData* pData, void* pUserData)
    {
      WorldData::UpdateGlobalTransformWithParent(pData, *static_cast<ezSimdFloat*>(pUserData));
      return ezVisitorExecution::Continue;
    }
  };

  ezSimdFloat fInvDt = fInvDeltaSeconds;

  Hierarchy& hierarchy = m_Hierarchies[HierarchyType::Dynamic];
  if (!hierarchy.m_Data.IsEmpty())
  {
    TraverseHierarchyLevel<RootLevel>(*hierarchy.m_Data[0], &fInvDt);

    for (ezUInt32 i = 1; i < hierarchy.m_Data.GetCount(); ++i)
    {
      TraverseHierarchyLevel<WithParent>(*hierarchy.m_Data[i], &fInvDt);
    }
  }
}

}


EZ_STATICLINK_FILE(Core, Core_World_Implementation_WorldData);

