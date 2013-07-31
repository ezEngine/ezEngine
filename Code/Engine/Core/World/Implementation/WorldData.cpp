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
  m_ObjectStorage(&m_BlockAllocator, &m_Allocator)
{
  m_AllocatorWrapper.Reset();

  EZ_CHECK_AT_COMPILETIME(sizeof(ezGameObject::HierarchicalData) == 128);
  EZ_CHECK_AT_COMPILETIME(sizeof(ezGameObject) == 128);
}

WorldData::~WorldData()
{
  // delete all component manager
  for (ezUInt32 i = 0; i < m_ComponentManagers.GetCount(); ++i)
  {
    EZ_DELETE(&m_Allocator, m_ComponentManagers[i]);
  }

  // delete all hierarchical data
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
  if (objectFlags.IsSet(ezObjectFlags::Active))
  {
    return objectFlags.IsSet(ezObjectFlags::Dynamic) ? WorldData::HierarchyType::Dynamic : WorldData::HierarchyType::Static;
  }
  else
  {
    return WorldData::HierarchyType::Inactive;
  }
}

ezUInt32 WorldData::CreateHierarchicalData(const ezBitflags<ezObjectFlags>& objectFlags, ezUInt32 uiHierarchyLevel,
  ezArrayPtr<ezGameObject::HierarchicalData*> out_data)
{
  HierarchyType::Enum hierarchyType = GetHierarchyType(objectFlags);
  Hierarchy& hierarchy = m_Hierarchies[hierarchyType];
  
  if (hierarchyType == HierarchyType::Inactive)
    uiHierarchyLevel = 0;

  if (uiHierarchyLevel >= hierarchy.m_Data.GetCount())
  {
    hierarchy.m_Data.PushBack(EZ_NEW(&m_Allocator, Hierarchy::DataBlockArray)(&m_Allocator));
  }

  Hierarchy::DataBlockArray& blocks = *hierarchy.m_Data[uiHierarchyLevel];
  ezUInt32 uiCount = out_data.GetCount();
  ezUInt32 uiStartIndex = 0;
  ezUInt32 uiOutIndex = 0;

  if (!blocks.IsEmpty())
  {
    Hierarchy::DataBlock& lastBlock = blocks.PeekBack();
    const ezUInt32 uiDataCountInLastBlock = lastBlock.m_uiCount;

    uiStartIndex = uiDataCountInLastBlock + (blocks.GetCount() - 1) * HIERARCHICAL_DATA_PER_BLOCK;

    const ezUInt32 uiCurrentCount = ezMath::Min<ezUInt32>(uiCount, HIERARCHICAL_DATA_PER_BLOCK - uiDataCountInLastBlock);      
    for (ezUInt32 i = 0; i < uiCurrentCount; ++i)
    {
      out_data[uiOutIndex] = lastBlock.ReserveBack();
      ++uiOutIndex;
    }
  }

  uiCount -= uiOutIndex;
  const ezUInt32 uiBlockCount = (uiCount + HIERARCHICAL_DATA_PER_BLOCK - 1) / HIERARCHICAL_DATA_PER_BLOCK;

  for (ezUInt32 uiBlock = 0; uiBlock < uiBlockCount; ++uiBlock)
  {
    const ezUInt32 uiCurrentCount = ezMath::Min<ezUInt32>(uiCount, HIERARCHICAL_DATA_PER_BLOCK);

    blocks.PushBack(m_BlockAllocator.AllocateBlock<ezGameObject::HierarchicalData>());
    Hierarchy::DataBlock& block = blocks.PeekBack();

    uiCount -= HIERARCHICAL_DATA_PER_BLOCK;

    for (ezUInt32 i = 0; i < uiCurrentCount; ++i)
    {
      out_data[uiOutIndex] = block.ReserveBack();
      ++uiOutIndex;
    }
  }

  return uiStartIndex;
}

void WorldData::DeleteHierarchicalData(const ezBitflags<ezObjectFlags>& objectFlags, ezUInt32 uiHierarchyLevel, 
  ezUInt32 uiIndex, ezUInt32 uiCount /*= 1*/)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

}
