#include <Core/PCH.h>
#include <Core/World/World.h>

ezWorld::Hierarchy::Hierarchy()
{
}

ezWorld::Hierarchy::~Hierarchy()
{
  for (ezUInt32 i = m_data.GetCount(); i-- > 0; )
  {
    DataBlockArray* blocks = m_data[i];
    for (ezUInt32 j = blocks->GetCount(); j-- > 0; )
    {
      m_pWorld->m_BlockAllocator.DeallocateBlock((*blocks)[j]);
    }
    EZ_DELETE(&m_pWorld->m_Allocator, blocks);
  }
}

ezUInt32 ezWorld::Hierarchy::ReserveData(ezArrayPtr<ezGameObject::HierarchicalData*> out_data, 
  ezUInt32 uiHierarchyLevel, ezUInt32 uiFirstIndex /*= ezInvalidIndex*/)
{
  if (uiHierarchyLevel >= m_data.GetCount())
  {
    m_data.PushBack(EZ_NEW(&m_pWorld->m_Allocator, DataBlockArray)(&m_pWorld->m_Allocator));
  }

  DataBlockArray& blocks = *m_data[uiHierarchyLevel];
  ezUInt32 uiCount = out_data.GetCount();

  if (uiFirstIndex == ezInvalidIndex)
  {
    ezUInt32 uiOutIndex = 0;
    ezUInt32 uiStartIndex = 0;

    if (!blocks.IsEmpty())
    {
      DataBlock& lastBlock = blocks.PeekBack();
      const ezUInt32 uiDataCountInLastBlock = lastBlock.m_uiCount;
      
      uiStartIndex = uiDataCountInLastBlock + blocks.GetCount() * HIERARCHICAL_DATA_PER_BLOCK;

      const ezUInt32 uiCurrentCount = ezMath::Min<ezUInt32>(uiDataCountInLastBlock + uiCount, HIERARCHICAL_DATA_PER_BLOCK);
      
      for (ezUInt32 i = uiDataCountInLastBlock; i < uiCurrentCount; ++i)
      {
        out_data[uiOutIndex] = lastBlock.ReserveBack();
        ++uiOutIndex;
      }
    }

    uiCount -= uiOutIndex;    
    ReserveWholeBlocks(blocks, uiCount, out_data, uiOutIndex);

    return uiStartIndex;
  }
  else
  {   
    EZ_ASSERT_NOT_IMPLEMENTED;
    return 0;
  }
}

void ezWorld::Hierarchy::ReserveWholeBlocks(DataBlockArray& hierarchyBlocks, ezUInt32 uiCount, 
  ezArrayPtr<ezGameObject::HierarchicalData*> out_data, ezUInt32 uiOutIndex /*= 0*/)
{
  const ezUInt32 uiBlockCount = (uiCount + HIERARCHICAL_DATA_PER_BLOCK - 1) / HIERARCHICAL_DATA_PER_BLOCK;

  for (ezUInt32 uiBlock = 0; uiBlock < uiBlockCount; ++uiBlock)
  {
    const ezUInt32 uiCurrentCount = ezMath::Min<ezUInt32>(uiCount, HIERARCHICAL_DATA_PER_BLOCK);

    DataBlock block = m_pWorld->m_BlockAllocator.AllocateBlock<ezGameObject::HierarchicalData>();
    hierarchyBlocks.PushBack(block);

    uiCount -= HIERARCHICAL_DATA_PER_BLOCK;

    for (ezUInt32 i = 0; i < uiCurrentCount; ++i)
    {
      out_data[uiOutIndex] = block.ReserveBack();
      ++uiOutIndex;
    }
  }
}
    
void ezWorld::Hierarchy::RemoveData(ezUInt32 uiHierarchyLevel, 
  ezUInt32 uiFirstIndex /*= ezInvalidIndex*/, ezUInt32 uiCount /*= 1*/)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

//static
EZ_FORCE_INLINE ezWorld::HierarchyType ezWorld::GetHierarchyType(const ezBitflags<ezGameObjectFlags>& objectFlags)
{
  if (objectFlags.IsSet(ezGameObjectFlags::Active))
  {
    return objectFlags.IsSet(ezGameObjectFlags::Dynamic) ? HIERARCHY_DYNAMIC : HIERARCHY_STATIC;
  }
  else
  {
    return HIERARCHY_INACTIVE;
  }
}
