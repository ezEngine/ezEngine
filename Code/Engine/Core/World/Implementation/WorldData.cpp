#include <Core/PCH.h>

#include <Foundation/Time/Clock.h>

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
  m_Function(m_uiStartIndex, m_uiCount);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
  
WorldData::WorldData(const char* szWorldName) :
  m_Allocator(szWorldName, ezFoundation::GetDefaultAllocator()),
  m_AllocatorWrapper(&m_Allocator),
  m_BlockAllocator(szWorldName, &m_Allocator),
  m_ObjectStorage(&m_BlockAllocator, &m_Allocator),
  m_WriteThreadID((ezThreadID)0),
  m_iWriteCounter(0),
  m_ReadMarker(*this),
  m_WriteMarker(*this),
  m_pUserData(nullptr)
{
  m_AllocatorWrapper.Reset();

  m_sName.Assign(szWorldName);

  // insert dummy entry to save some checks
  ObjectStorage::Entry entry = { nullptr };
  m_Objects.Insert(entry);

  EZ_CHECK_AT_COMPILETIME(sizeof(ezGameObject::TransformationData) == 192);
  //EZ_CHECK_AT_COMPILETIME(sizeof(ezGameObject) == 128); /// \todo get game object size back to 128

  m_pCoordinateSystemProvider = EZ_NEW(&m_Allocator, DefaultCoordinateSystemProvider);
}

WorldData::~WorldData()
{
  EZ_ASSERT_DEV(m_ComponentManagers.IsEmpty(), "Component managers should be cleaned up already.");

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
    MessageQueue& queue = m_MessageQueues[i];
    while (!queue.IsEmpty())
    {
      MessageQueue::Entry& entry = queue.Peek();
      EZ_DELETE(&m_Allocator, entry.m_pMessage);

      queue.Dequeue();
    }
  }
}

ezUInt32 WorldData::CreateTransformationData(const ezBitflags<ezObjectFlags>& objectFlags, ezUInt32 uiHierarchyLevel,
  ezGameObject::TransformationData*& out_pData)
{
  HierarchyType::Enum hierarchyType = objectFlags.IsSet(ezObjectFlags::Dynamic) ? 
    WorldData::HierarchyType::Dynamic : WorldData::HierarchyType::Static;
  Hierarchy& hierarchy = m_Hierarchies[hierarchyType];
  
  if (uiHierarchyLevel >= hierarchy.m_Data.GetCount())
  {
    hierarchy.m_Data.PushBack(EZ_NEW(&m_Allocator, Hierarchy::DataBlockArray, &m_Allocator));
  }

  Hierarchy::DataBlockArray& blocks = *hierarchy.m_Data[uiHierarchyLevel];
  Hierarchy::DataBlock* pBlock = NULL;

  if (!blocks.IsEmpty())
  {
    pBlock = &blocks.PeekBack();
  }

  if (pBlock == NULL || pBlock->IsFull())
  {
    blocks.PushBack(m_BlockAllocator.AllocateBlock<ezGameObject::TransformationData>());
    pBlock = &blocks.PeekBack();
  }

  ezUInt32 uiInnerIndex = pBlock->m_uiCount;
  out_pData = pBlock->ReserveBack();

  return uiInnerIndex + (blocks.GetCount() - 1) * TRANSFORMATION_DATA_PER_BLOCK;
}

void WorldData::DeleteTransformationData(const ezBitflags<ezObjectFlags>& objectFlags, ezUInt32 uiHierarchyLevel, 
  ezUInt32 uiIndex)
{
  HierarchyType::Enum hierarchyType = objectFlags.IsSet(ezObjectFlags::Dynamic) ? 
    WorldData::HierarchyType::Dynamic : WorldData::HierarchyType::Static;
  Hierarchy& hierarchy = m_Hierarchies[hierarchyType];
  Hierarchy::DataBlockArray& blocks = *hierarchy.m_Data[uiHierarchyLevel];

  Hierarchy::DataBlock& lastBlock = blocks.PeekBack();
  const ezGameObject::TransformationData* pLast = lastBlock.PopBack();
  const ezUInt32 uiLastIndex = lastBlock.m_uiCount + (blocks.GetCount() - 1) * TRANSFORMATION_DATA_PER_BLOCK;

  if (uiLastIndex != uiIndex)
  {
    const ezUInt32 uiBlockIndex = uiIndex / TRANSFORMATION_DATA_PER_BLOCK;
    const ezUInt32 uiInnerIndex = uiIndex - uiBlockIndex * TRANSFORMATION_DATA_PER_BLOCK;

    Hierarchy::DataBlock& block = blocks[uiBlockIndex];
    ezGameObject::TransformationData* pCurrent = &block[uiInnerIndex];

    ezMemoryUtils::Copy(pCurrent, pLast, 1);
    pCurrent->m_pObject->m_pTransformationData = pCurrent;
    pCurrent->m_pObject->m_uiTransformationDataIndex = uiIndex;
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
    EZ_FORCE_INLINE static bool Visit(ezGameObject::TransformationData* pData, void* pUserData)
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
        if (!TraverseHierarchyLevel<Helper>(*hierarchy.m_Data[uiHierarchyLevel], &func))
          return;
      }
    }
  }
}

void WorldData::TraverseDepthFirst(VisitorFunc& func)
{
  struct Helper
  {
    EZ_FORCE_INLINE static bool Visit(ezGameObject::TransformationData* pData, void* pUserData)
    {
      return WorldData::TraverseObjectDepthFirst(pData->m_pObject, *static_cast<VisitorFunc*>(pUserData));
    }
  };

  for (ezUInt32 uiHierarchyIndex = 0; uiHierarchyIndex < HierarchyType::COUNT; ++uiHierarchyIndex)
  {
    Hierarchy& hierarchy = m_Hierarchies[uiHierarchyIndex];
    if (!hierarchy.m_Data.IsEmpty())
    {
      if (!TraverseHierarchyLevel<Helper>(*hierarchy.m_Data[0], &func))
        return;
    }
  }
}

// static
bool WorldData::TraverseObjectDepthFirst(ezGameObject* pObject, VisitorFunc& func)
{
  if (!func(pObject))
    return false;

  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    if (!TraverseObjectDepthFirst(it, func))
      return false;
  }

  return true;
}

void WorldData::UpdateGlobalTransforms()
{
  struct RootLevel
  {
    EZ_FORCE_INLINE static bool Visit(ezGameObject::TransformationData* pData, void* pUserData)
    {
      WorldData::UpdateGlobalTransform(pData, *static_cast<float*>(pUserData));
      return true;
    }
  };

  struct WithParent
  {
    EZ_FORCE_INLINE static bool Visit(ezGameObject::TransformationData* pData, void* pUserData)
    {
      WorldData::UpdateGlobalTransformWithParent(pData, *static_cast<float*>(pUserData));
      return true;
    }
  };

  float fInvDeltaSeconds = 1.0f / (float)ezClock::Get(ezGlobalClock_GameLogic)->GetTimeDiff().GetSeconds();

  Hierarchy& hierarchy = m_Hierarchies[HierarchyType::Dynamic];
  if (!hierarchy.m_Data.IsEmpty())
  {
    TraverseHierarchyLevel<RootLevel>(*hierarchy.m_Data[0], &fInvDeltaSeconds);

    for (ezUInt32 i = 1; i < hierarchy.m_Data.GetCount(); ++i)
    {
      TraverseHierarchyLevel<WithParent>(*hierarchy.m_Data[i], &fInvDeltaSeconds);
    }
  }
}

}


EZ_STATICLINK_FILE(Core, Core_World_Implementation_WorldData);

