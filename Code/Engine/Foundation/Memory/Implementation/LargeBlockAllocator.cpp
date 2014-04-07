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

ezLargeBlockAllocator::ezLargeBlockAllocator(const char* szName, ezAllocatorBase* pParent, ezBitflags<ezMemoryTrackingFlags> flags) : 
  m_superBlocks(pParent),
  m_freeBlocks(pParent)
{
  m_Id = ezMemoryTracker::RegisterAllocator(szName, flags);
  m_ThreadID = ezThreadUtils::GetCurrentThreadID();

  const ezUInt32 uiPageSize = ezSystemInformation::Get().GetMemoryPageSize();
  EZ_ASSERT(uiPageSize <= BLOCK_SIZE_IN_BYTES, "Memory Page size is bigger than block size.");
}

ezLargeBlockAllocator::~ezLargeBlockAllocator()
{
  EZ_ASSERT_API(m_ThreadID == ezThreadUtils::GetCurrentThreadID(), "Allocator is deleted from another thread");
  ezMemoryTracker::DeregisterAllocator(m_Id);

  for (ezUInt32 i = 0; i < m_superBlocks.GetCount(); ++i)
  {
    ezPageAllocator::DeallocatePage(m_superBlocks[i].m_pBasePtr);
  }
}

const char* ezLargeBlockAllocator::GetName() const
{
  return ezMemoryTracker::GetAllocatorName(m_Id);
}

const ezAllocatorBase::Stats& ezLargeBlockAllocator::GetStats() const
{
  return ezMemoryTracker::GetAllocatorStats(m_Id);
}

void* ezLargeBlockAllocator::Allocate(size_t uiAlign)
{
  EZ_ASSERT_API(ezMath::IsPowerOf2((ezUInt32)uiAlign), "Alignment must be power of two");

  ezLock<ezMutex> lock(m_mutex);

  void* ptr = nullptr;

  if (!m_freeBlocks.IsEmpty())
  {
    // Re-use a super block
    ezUInt32 uiFreeBlockIndex = m_freeBlocks.PeekBack();
    m_freeBlocks.PopBack();

    const ezUInt32 uiSuperBlockIndex = uiFreeBlockIndex / SuperBlock::NUM_BLOCKS;
    const ezUInt32 uiInnerBlockIndex = uiFreeBlockIndex & (SuperBlock::NUM_BLOCKS - 1);
    SuperBlock& superBlock = m_superBlocks[uiSuperBlockIndex];
    ++superBlock.m_uiUsedBlocks;

    ptr = ezMemoryUtils::AddByteOffset(superBlock.m_pBasePtr, uiInnerBlockIndex * BLOCK_SIZE_IN_BYTES);
  }
  else
  {
    // Allocate a new super block
    void* pMemory = ezPageAllocator::AllocatePage(SuperBlock::SIZE_IN_BYTES);
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

    ptr = pMemory;
  }

  ezMemoryTracker::AddAllocation(m_Id, ptr, BLOCK_SIZE_IN_BYTES, uiAlign);

  return ptr;
}

void ezLargeBlockAllocator::Deallocate(void* ptr)
{
  ezLock<ezMutex> lock(m_mutex);

  ezMemoryTracker::RemoveAllocation(m_Id, ptr);

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

  EZ_ASSERT(bFound, "'%p' was not allocated with this allocator", ptr);
  
  SuperBlock& superBlock = m_superBlocks[uiSuperBlockIndex];
  --superBlock.m_uiUsedBlocks;

  if (superBlock.m_uiUsedBlocks == 0 && m_freeBlocks.GetCount() > SuperBlock::NUM_BLOCKS * 4)
  {
    // give memory back
    ezPageAllocator::DeallocatePage(superBlock.m_pBasePtr);

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


EZ_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_LargeBlockAllocator);

