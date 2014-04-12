#include <Core/PCH.h>
#include <Core/World/World.h>

namespace ezInternal
{

////////////////////////////////////////////////////////////////////////////////////////////////////

void WorldData::UpdateTask::Execute()
{
  m_Function(m_uiStartIndex, m_uiCount);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
  
WorldData::WorldData(const char* szWorldName) :
  m_Name(szWorldName),
  m_Allocator(m_Name.GetData(), ezFoundation::GetDefaultAllocator()),
  m_AllocatorWrapper(&m_Allocator),
  m_BlockAllocator(m_Name.GetData(), &m_Allocator),
  m_ObjectStorage(&m_BlockAllocator, &m_Allocator),
  m_uiHandledMessageCounter(0),
  m_ThreadID(ezThreadUtils::GetCurrentThreadID()),
  m_bIsInAsyncPhase(false),
  m_pUserData(nullptr)
{
  m_AllocatorWrapper.Reset();

  m_UpdateProfilingID = ezProfilingSystem::CreateId(m_Name.GetData());

  // insert dummy entry to save some checks
  ObjectStorage::Entry entry = { nullptr };
  m_Objects.Insert(entry);

  EZ_CHECK_AT_COMPILETIME(sizeof(ezGameObject::TransformationData) == 128);
  EZ_CHECK_AT_COMPILETIME(sizeof(ezGameObject) == 128);
}

WorldData::~WorldData()
{
  // delete all component manager
  for (ezUInt32 i = 0; i < m_ComponentManagers.GetCount(); ++i)
  {
    EZ_DELETE(&m_Allocator, m_ComponentManagers[i]);
  }

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
}

//static
static EZ_FORCE_INLINE WorldData::HierarchyType::Enum GetHierarchyType(const ezBitflags<ezObjectFlags>& objectFlags)
{
  return objectFlags.IsSet(ezObjectFlags::Dynamic) ? WorldData::HierarchyType::Dynamic : WorldData::HierarchyType::Static;
}

ezUInt32 WorldData::CreateTransformationData(const ezBitflags<ezObjectFlags>& objectFlags, ezUInt32 uiHierarchyLevel,
  ezGameObject::TransformationData*& out_pData)
{
  HierarchyType::Enum hierarchyType = GetHierarchyType(objectFlags);
  Hierarchy& hierarchy = m_Hierarchies[hierarchyType];
  
  if (uiHierarchyLevel >= hierarchy.m_Data.GetCount())
  {
    hierarchy.m_Data.PushBack(EZ_NEW(&m_Allocator, Hierarchy::DataBlockArray)(&m_Allocator));
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
  HierarchyType::Enum hierarchyType = GetHierarchyType(objectFlags);
  Hierarchy& hierarchy = m_Hierarchies[hierarchyType];
  Hierarchy::DataBlockArray& blocks = *hierarchy.m_Data[uiHierarchyLevel];

  Hierarchy::DataBlock& lastBlock = blocks.PeekBack();
  ezGameObject::TransformationData* pLast = lastBlock.PopBack();
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

void WorldData::UpdateWorldTransforms()
{
  struct RootLevel
  {
    EZ_FORCE_INLINE static void Update(ezGameObject::TransformationData* pData)
    {
      WorldData::UpdateWorldTransform(pData);
    }
  };

  struct WithParent
  {
    EZ_FORCE_INLINE static void Update(ezGameObject::TransformationData* pData)
    {
      WorldData::UpdateWorldTransformWithParent(pData);
    }
  };

  Hierarchy& hierarchy = m_Hierarchies[WorldData::HierarchyType::Dynamic];
  UpdateHierarchyLevel<RootLevel>(*hierarchy.m_Data[0]);

  for (ezUInt32 i = 1; i < hierarchy.m_Data.GetCount(); ++i)
  {
    UpdateHierarchyLevel<WithParent>(*hierarchy.m_Data[i]);
  }
}

}


EZ_STATICLINK_FILE(Core, Core_World_Implementation_WorldData);

