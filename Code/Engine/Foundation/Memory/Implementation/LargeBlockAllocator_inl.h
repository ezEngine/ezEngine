
template <typename T, ezUInt32 SizeInBytes>
EZ_FORCE_INLINE ezDataBlock<T, SizeInBytes>::ezDataBlock(T* pData, ezUInt32 uiCount)
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
EZ_FORCE_INLINE bool ezDataBlock<T, SizeInBytes>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, ezUInt32 SizeInBytes>
EZ_FORCE_INLINE bool ezDataBlock<T, SizeInBytes>::IsFull() const
{
  return m_uiCount == CAPACITY;
}

template <typename T, ezUInt32 SizeInBytes>
EZ_FORCE_INLINE T& ezDataBlock<T, SizeInBytes>::operator[](ezUInt32 uiIndex) const
{
  EZ_ASSERT_DEV(uiIndex < m_uiCount, "Out of bounds access. Data block has %i elements, trying to access element at index %i.", m_uiCount, uiIndex);
  return m_pData[uiIndex];
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template <ezUInt32 BlockSize>
ezLargeBlockAllocator<BlockSize>::ezLargeBlockAllocator(const char* szName, ezAllocatorBase* pParent, ezBitflags<ezMemoryTrackingFlags> flags) : 
  m_superBlocks(pParent),
  m_freeBlocks(pParent)
{
  EZ_CHECK_AT_COMPILETIME_MSG(BlockSize >= 4096, "Block size must be 4096 or bigger");

  m_Id = ezMemoryTracker::RegisterAllocator(szName, flags);
  m_ThreadID = ezThreadUtils::GetCurrentThreadID();

  const ezUInt32 uiPageSize = ezSystemInformation::Get().GetMemoryPageSize();
  EZ_IGNORE_UNUSED(uiPageSize);
  EZ_ASSERT_DEV(uiPageSize <= BlockSize, "Memory Page size is bigger than block size.");
  EZ_ASSERT_DEV(BlockSize % uiPageSize == 0, "Blocksize (%u) must be a multiple of page size (%u)", BlockSize, uiPageSize);
}

template <ezUInt32 BlockSize>
ezLargeBlockAllocator<BlockSize>::~ezLargeBlockAllocator()
{
  EZ_ASSERT_RELEASE(m_ThreadID == ezThreadUtils::GetCurrentThreadID(), "Allocator is deleted from another thread");
  ezMemoryTracker::DeregisterAllocator(m_Id);

  for (ezUInt32 i = 0; i < m_superBlocks.GetCount(); ++i)
  {
    ezPageAllocator::DeallocatePage(m_superBlocks[i].m_pBasePtr);
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

  EZ_CHECK_AT_COMPILETIME_MSG(Helper::BLOCK_CAPACITY >= 1,
    "Type is too big for block allocation. Consider using regular heap allocation instead or increase the block size.");

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
EZ_FORCE_INLINE const char* ezLargeBlockAllocator<BlockSize>::GetName() const
{
  return ezMemoryTracker::GetAllocatorName(m_Id);
}

template <ezUInt32 BlockSize>
EZ_FORCE_INLINE ezAllocatorId ezLargeBlockAllocator<BlockSize>::GetId() const
{
  return m_Id;
}

template <ezUInt32 BlockSize>
EZ_FORCE_INLINE const ezAllocatorBase::Stats& ezLargeBlockAllocator<BlockSize>::GetStats() const
{
  return ezMemoryTracker::GetAllocatorStats(m_Id);
}

template <ezUInt32 BlockSize>
void* ezLargeBlockAllocator<BlockSize>::Allocate(size_t uiAlign)
{
  EZ_ASSERT_RELEASE(ezMath::IsPowerOf2((ezUInt32)uiAlign), "Alignment must be power of two");

  EZ_LOCK(m_mutex);

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

    m_superBlocks.PushBack(superBlock);
    
    const ezUInt32 uiBlockBaseIndex = (m_superBlocks.GetCount() - 1) * SuperBlock::NUM_BLOCKS;
    for (ezUInt32 i = SuperBlock::NUM_BLOCKS - 1; i > 0; --i)
    {
      m_freeBlocks.PushBack(uiBlockBaseIndex + i);
    }

    ptr = pMemory;
  }

  ezMemoryTracker::AddAllocation(m_Id, ptr, BlockSize, uiAlign);

  return ptr;
}

template <ezUInt32 BlockSize>
void ezLargeBlockAllocator<BlockSize>::Deallocate(void* ptr)
{
  EZ_LOCK(m_mutex);

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

  EZ_ASSERT_DEV(bFound, "'%p' was not allocated with this allocator", ptr);
  
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
    const ezUInt32 uiInnerBlockIndex = (ezUInt32)(diff / BlockSize);
    m_freeBlocks.PushBack(uiSuperBlockIndex * SuperBlock::NUM_BLOCKS + uiInnerBlockIndex);
  }
}

