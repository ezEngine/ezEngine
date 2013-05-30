#include <Foundation/PCH.h>
#include <Foundation/Memory/LargeBlockAllocator.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/ThreadUtils.h>

namespace
{
  enum
  {
    BLOCK_SIZE_IN_BYTES = ezDataBlock<int>::SIZE_IN_BYTES
  };
}

ezLargeBlockAllocator::ezLargeBlockAllocator(const char* szName, ezIAllocator* pParent) : 
  ezIAllocator(szName),
  m_threadHandle(ezThreadUtils::GetCurrentThreadHandle()),
  m_uiNumAllocations(0),
  m_uiNumDeallocations(0),
  m_superBlocks(pParent),
  m_freeBlocks(pParent)
{
  const ezUInt32 uiPageSize = ezSystemInformation::Get().GetMemoryPageSize();
  EZ_ASSERT(uiPageSize <= BLOCK_SIZE_IN_BYTES, "Memory Page size is bigger than block size.");
}

ezLargeBlockAllocator::~ezLargeBlockAllocator()
{
  EZ_ASSERT_API(m_threadHandle == ezThreadUtils::GetCurrentThreadHandle(), "Allocator is deleted from another thread");

  DumpMemoryLeaks();

  for (ezUInt32 i = 0; i < m_superBlocks.GetCount(); ++i)
  {
    m_allocator.PageDeallocate(m_superBlocks[i].m_pBasePtr);
  }
}

void* ezLargeBlockAllocator::Allocate(size_t uiSize, size_t uiAlign)
{
  EZ_ASSERT_API(uiSize == BLOCK_SIZE_IN_BYTES, "size must be %d. Got %d", BLOCK_SIZE_IN_BYTES, uiSize);
  EZ_ASSERT_API(ezMath::IsPowerOf2((ezUInt32)uiAlign), "Alignment must be power of two");

  ezLock<ezMutex> lock(m_mutex);

  ++m_uiNumAllocations;

  if (!m_freeBlocks.IsEmpty())
  {
    // Re-use a super block
    ezUInt32 uiFreeBlockIndex = m_freeBlocks.PeekBack();
    m_freeBlocks.PopBack();

    const ezUInt32 uiSuperBlockIndex = uiFreeBlockIndex / SuperBlock::NUM_BLOCKS;
    const ezUInt32 uiInnerBlockIndex = uiFreeBlockIndex & (SuperBlock::NUM_BLOCKS - 1);
    SuperBlock& superBlock = m_superBlocks[uiSuperBlockIndex];
    ++superBlock.m_uiUsedBlocks;

    return ezMemoryUtils::AddByteOffset(superBlock.m_pBasePtr, uiInnerBlockIndex * BLOCK_SIZE_IN_BYTES);
  }
  else
  {
    // Allocate a new super block
    void* pMemory = m_allocator.PageAllocate(SuperBlock::SIZE_IN_BYTES);
    EZ_CHECK_ALIGNMENT(pMemory, uiAlign);

    SuperBlock superBlock;
    superBlock.m_pBasePtr = pMemory;
    superBlock.m_uiUsedBlocks = 1;

    m_superBlocks.PushBack(superBlock);
    
    const ezUInt32 uiBlockBaseIndex = (m_superBlocks.GetCount() - 1) * SuperBlock::NUM_BLOCKS;
    for (ezUInt32 i = SuperBlock::NUM_BLOCKS - 1; i > 0; --i)
    {
      m_freeBlocks.PushBack(uiBlockBaseIndex + i);
    }

    return pMemory;
  }
}

void ezLargeBlockAllocator::Deallocate(void* ptr)
{
  ezLock<ezMutex> lock(m_mutex);

  ++m_uiNumDeallocations;

  // find super block
  bool bFound = false;
  ezUInt32 uiSuperBlockIndex = m_superBlocks.GetCount();
  ptrdiff_t diff = 0;

  for (; uiSuperBlockIndex-- > 0; )
  {
    diff = (char*)ptr - (char*)m_superBlocks[uiSuperBlockIndex].m_pBasePtr;
    if (diff >= 0 && diff < SuperBlock::SIZE_IN_BYTES)
    {
      bFound = true;
      break;
    }
  }

  if (!bFound)
  {
    EZ_REPORT_FAILURE("'%x' was not allocated with this allocator", ptr);
    return;
  }

  SuperBlock& superBlock = m_superBlocks[uiSuperBlockIndex];
  --superBlock.m_uiUsedBlocks;

  if (superBlock.m_uiUsedBlocks == 0 && m_freeBlocks.GetCount() > SuperBlock::NUM_BLOCKS * 5)
  {
    // give memory back
    m_allocator.PageDeallocate(superBlock.m_pBasePtr);

    m_superBlocks.RemoveAtSwap(uiSuperBlockIndex);
    const ezUInt32 uiLastSuperBlockIndex = m_superBlocks.GetCount();

    // patch free list
    for (ezUInt32 i = 0; i < m_freeBlocks.GetCount(); ++i)
    {
      const ezUInt32 uiIndex = m_freeBlocks[i];
      const ezUInt32 uiSBIndex = uiIndex / SuperBlock::NUM_BLOCKS;
      
      if (uiSBIndex == uiSuperBlockIndex)
      {
        // points to the block we just removed
        m_freeBlocks.RemoveAtSwap(i);
        --i;
      }
      else if (uiSBIndex == uiLastSuperBlockIndex)
      {
        // points to the block we just swapped
        m_freeBlocks[i] = uiSuperBlockIndex * SuperBlock::NUM_BLOCKS + (uiIndex & (SuperBlock::NUM_BLOCKS - 1));
      }
    }
  }
  else
  {
    // add block to free list
    const ezUInt32 uiInnerBlockIndex = (ezUInt32)(diff / BLOCK_SIZE_IN_BYTES);
    m_freeBlocks.PushBack(uiSuperBlockIndex * SuperBlock::NUM_BLOCKS + uiInnerBlockIndex);
  }
}

size_t ezLargeBlockAllocator::AllocatedSize(const void* ptr)
{
  return BLOCK_SIZE_IN_BYTES;
}

size_t ezLargeBlockAllocator::UsedMemorySize(const void* ptr)
{
  return BLOCK_SIZE_IN_BYTES;
}

void ezLargeBlockAllocator::GetStats(Stats& stats) const
{
  stats.m_uiNumAllocations = m_uiNumAllocations;
  stats.m_uiNumDeallocations = m_uiNumDeallocations;
  stats.m_uiNumLiveAllocations = m_superBlocks.GetCount() * SuperBlock::NUM_BLOCKS - m_freeBlocks.GetCount();
  stats.m_uiAllocationSize = stats.m_uiNumLiveAllocations * BLOCK_SIZE_IN_BYTES;
  stats.m_uiUsedMemorySize = m_superBlocks.GetCount() * SuperBlock::SIZE_IN_BYTES;
}
