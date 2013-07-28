#pragma once

#include <Foundation/Memory/LargeBlockAllocator.h>

template <typename T>
class ezBlockStorage
{
public:
  EZ_FORCE_INLINE ezBlockStorage(ezLargeBlockAllocator* pBlockAllocator, ezIAllocator* pAllocator) : 
    m_pBlockAllocator(pBlockAllocator), m_Blocks(pAllocator)
  {
  }

  ~ezBlockStorage()
  {
    for (ezUInt32 i = 0; i < m_Blocks.GetCount(); ++i)
    {
      m_pBlockAllocator->DeallocateBlock(m_Blocks[i]);
    }

    m_Blocks.Clear();
  }

  T* Create()
  {
    ezDataBlock<T>* pBlock = NULL;

    if (m_Blocks.GetCount() > 0)
    {
      pBlock = &m_Blocks.PeekBack();
    }

    if (pBlock == NULL || pBlock->IsFull())
    {
      m_Blocks.PushBack(m_pBlockAllocator->AllocateBlock<T>());
      pBlock = &m_Blocks.PeekBack();
    }

    ++m_uiCount;
    return pBlock->ReserveBack();
  }

  void Delete(T* pValue)
  {
    
  }

  template <typename Functor>
  void Iterate(Functor& functor, ezUInt32 uiStartIndex, ezUInt32 uiCount)
  {
    // TODO:
    const ezUInt32 uiFirstBlock = uiStartIndex / ezDataBlock<T>::CAPACITY;
    ezUInt32 uiCurrentIndex = 0;

    for (ezUInt32 blockIndex = uiFirstBlock; uiCurrentIndex < uiCount; ++blockIndex)
    {
      ezDataBlock<T>& currentBlock = m_Blocks[blockIndex];
      
      for (ezUInt32 i = 0; i < currentBlock.m_uiCount; ++i)
      {
        functor(currentBlock[i]);
        ++uiCurrentIndex;
      }
    }
  }

private:
  ezLargeBlockAllocator* m_pBlockAllocator;

  ezDynamicArray<ezDataBlock<T> > m_Blocks;
  ezUInt32 m_uiCount;
};
