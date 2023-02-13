
template <typename T, ezUInt32 SizeInBytes>
EZ_ALWAYS_INLINE ezDataBlock<T, SizeInBytes>::ezDataBlock(T* pData, ezUInt32 uiCount)
{
  m_pData = pData;
  m_uiCount = uiCount;
}

template <typename T, ezUInt32 SizeInBytes>
EZ_FORCE_INLINE T* ezDataBlock<T, SizeInBytes>::ReserveBack()
{
  EZ_ASSERT_DEV(m_uiCount < CAPACITY, "Block is full.");
  return m_pData + m_uiCount++;
}

template <typename T, ezUInt32 SizeInBytes>
EZ_FORCE_INLINE T* ezDataBlock<T, SizeInBytes>::PopBack()
{
  EZ_ASSERT_DEV(m_uiCount > 0, "Block is empty");
  --m_uiCount;
  return m_pData + m_uiCount;
}

template <typename T, ezUInt32 SizeInBytes>
EZ_ALWAYS_INLINE bool ezDataBlock<T, SizeInBytes>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, ezUInt32 SizeInBytes>
EZ_ALWAYS_INLINE bool ezDataBlock<T, SizeInBytes>::IsFull() const
{
  return m_uiCount == CAPACITY;
}

template <typename T, ezUInt32 SizeInBytes>
EZ_FORCE_INLINE T& ezDataBlock<T, SizeInBytes>::operator[](ezUInt32 uiIndex) const
{
  EZ_ASSERT_DEV(uiIndex < m_uiCount, "Out of bounds access. Data block has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return m_pData[uiIndex];
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template <ezUInt32 BlockSize>
ezLargeBlockAllocator<BlockSize>::ezLargeBlockAllocator(const char* szName, ezAllocatorBase* pParent, ezBitflags<ezMemoryTrackingFlags> flags)
  : m_TrackingFlags(flags)
  , m_SuperBlocks(pParent)
  , m_FreeBlocks(pParent)
{
  EZ_CHECK_AT_COMPILETIME_MSG(BlockSize >= 4096, "Block size must be 4096 or bigger");

  m_Id = ezMemoryTracker::RegisterAllocator(szName, flags, ezPageAllocator::GetId());
  m_ThreadID = ezThreadUtils::GetCurrentThreadID();

  const ezUInt32 uiPageSize = ezSystemInformation::Get().GetMemoryPageSize();
  EZ_IGNORE_UNUSED(uiPageSize);
  EZ_ASSERT_DEV(uiPageSize <= BlockSize, "Memory Page size is bigger than block size.");
  EZ_ASSERT_DEV(BlockSize % uiPageSize == 0, "Blocksize ({0}) must be a multiple of page size ({1})", BlockSize, uiPageSize);
}

template <ezUInt32 BlockSize>
ezLargeBlockAllocator<BlockSize>::~ezLargeBlockAllocator()
{
  EZ_ASSERT_RELEASE(m_ThreadID == ezThreadUtils::GetCurrentThreadID(), "Allocator is deleted from another thread");
  ezMemoryTracker::DeregisterAllocator(m_Id);

  for (ezUInt32 i = 0; i < m_SuperBlocks.GetCount(); ++i)
  {
    ezPageAllocator::DeallocatePage(m_SuperBlocks[i].m_pBasePtr);
  }
}

template <ezUInt32 BlockSize>
template <typename T>
EZ_FORCE_INLINE ezDataBlock<T, BlockSize> ezLargeBlockAllocator<BlockSize>::AllocateBlock()
{
  struct Helper
  {
    enum
    {
      BLOCK_CAPACITY = ezDataBlock<T, BlockSize>::CAPACITY
    };
  };

  EZ_CHECK_AT_COMPILETIME_MSG(
    Helper::BLOCK_CAPACITY >= 1, "Type is too big for block allocation. Consider using regular heap allocation instead or increase the block size.");

  ezDataBlock<T, BlockSize> block(static_cast<T*>(Allocate(EZ_ALIGNMENT_OF(T))), 0);
  return block;
}

template <ezUInt32 BlockSize>
template <typename T>
EZ_FORCE_INLINE void ezLargeBlockAllocator<BlockSize>::DeallocateBlock(ezDataBlock<T, BlockSize>& block)
{
  Deallocate(block.m_pData);
  block.m_pData = nullptr;
  block.m_uiCount = 0;
}

template <ezUInt32 BlockSize>
EZ_ALWAYS_INLINE const char* ezLargeBlockAllocator<BlockSize>::GetName() const
{
  return ezMemoryTracker::GetAllocatorName(m_Id);
}

template <ezUInt32 BlockSize>
EZ_ALWAYS_INLINE ezAllocatorId ezLargeBlockAllocator<BlockSize>::GetId() const
{
  return m_Id;
}

template <ezUInt32 BlockSize>
EZ_ALWAYS_INLINE const ezAllocatorBase::Stats& ezLargeBlockAllocator<BlockSize>::GetStats() const
{
  return ezMemoryTracker::GetAllocatorStats(m_Id);
}

template <ezUInt32 BlockSize>
void* ezLargeBlockAllocator<BlockSize>::Allocate(size_t uiAlign)
{
  EZ_ASSERT_RELEASE(ezMath::IsPowerOf2((ezUInt32)uiAlign), "Alignment must be power of two");

  ezTime fAllocationTime = ezTime::Now();

  EZ_LOCK(m_Mutex);

  void* ptr = nullptr;

  if (!m_FreeBlocks.IsEmpty())
  {
    // Re-use a super block
    ezUInt32 uiFreeBlockIndex = m_FreeBlocks.PeekBack();
    m_FreeBlocks.PopBack();

    const ezUInt32 uiSuperBlockIndex = uiFreeBlockIndex / SuperBlock::NUM_BLOCKS;
    const ezUInt32 uiInnerBlockIndex = uiFreeBlockIndex & (SuperBlock::NUM_BLOCKS - 1);
    SuperBlock& superBlock = m_SuperBlocks[uiSuperBlockIndex];
    ++superBlock.m_uiUsedBlocks;

    ptr = ezMemoryUtils::AddByteOffset(superBlock.m_pBasePtr, uiInnerBlockIndex * BlockSize);
  }
  else
  {
    // Allocate a new super block
    void* pMemory = ezPageAllocator::AllocatePage(SuperBlock::SIZE_IN_BYTES);
    EZ_CHECK_ALIGNMENT(pMemory, uiAlign);

    SuperBlock superBlock;
    superBlock.m_pBasePtr = pMemory;
    superBlock.m_uiUsedBlocks = 1;

    m_SuperBlocks.PushBack(superBlock);

    const ezUInt32 uiBlockBaseIndex = (m_SuperBlocks.GetCount() - 1) * SuperBlock::NUM_BLOCKS;
    for (ezUInt32 i = SuperBlock::NUM_BLOCKS - 1; i > 0; --i)
    {
      m_FreeBlocks.PushBack(uiBlockBaseIndex + i);
    }

    ptr = pMemory;
  }

  if ((m_TrackingFlags & ezMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    ezMemoryTracker::AddAllocation(m_Id, m_TrackingFlags, ptr, BlockSize, uiAlign, ezTime::Now() - fAllocationTime);
  }

  return ptr;
}

template <ezUInt32 BlockSize>
void ezLargeBlockAllocator<BlockSize>::Deallocate(void* ptr)
{
  EZ_LOCK(m_Mutex);

  if ((m_TrackingFlags & ezMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    ezMemoryTracker::RemoveAllocation(m_Id, ptr);
  }

  // find super block
  bool bFound = false;
  ezUInt32 uiSuperBlockIndex = m_SuperBlocks.GetCount();
  ptrdiff_t diff = 0;

  for (; uiSuperBlockIndex-- > 0;)
  {
    diff = (char*)ptr - (char*)m_SuperBlocks[uiSuperBlockIndex].m_pBasePtr;
    if (diff >= 0 && diff < SuperBlock::SIZE_IN_BYTES)
    {
      bFound = true;
      break;
    }
  }

  EZ_ASSERT_DEV(bFound, "'{0}' was not allocated with this allocator", ezArgP(ptr));

  SuperBlock& superBlock = m_SuperBlocks[uiSuperBlockIndex];
  --superBlock.m_uiUsedBlocks;

  if (superBlock.m_uiUsedBlocks == 0 && m_FreeBlocks.GetCount() > SuperBlock::NUM_BLOCKS * 4)
  {
    // give memory back
    ezPageAllocator::DeallocatePage(superBlock.m_pBasePtr);

    m_SuperBlocks.RemoveAtAndSwap(uiSuperBlockIndex);
    const ezUInt32 uiLastSuperBlockIndex = m_SuperBlocks.GetCount();

    // patch free list
    for (ezUInt32 i = 0; i < m_FreeBlocks.GetCount(); ++i)
    {
      const ezUInt32 uiIndex = m_FreeBlocks[i];
      const ezUInt32 uiSBIndex = uiIndex / SuperBlock::NUM_BLOCKS;

      if (uiSBIndex == uiSuperBlockIndex)
      {
        // points to the block we just removed
        m_FreeBlocks.RemoveAtAndSwap(i);
        --i;
      }
      else if (uiSBIndex == uiLastSuperBlockIndex)
      {
        // points to the block we just swapped
        m_FreeBlocks[i] = uiSuperBlockIndex * SuperBlock::NUM_BLOCKS + (uiIndex & (SuperBlock::NUM_BLOCKS - 1));
      }
    }
  }
  else
  {
    // add block to free list
    const ezUInt32 uiInnerBlockIndex = (ezUInt32)(diff / BlockSize);
    m_FreeBlocks.PushBack(uiSuperBlockIndex * SuperBlock::NUM_BLOCKS + uiInnerBlockIndex);
  }
}
