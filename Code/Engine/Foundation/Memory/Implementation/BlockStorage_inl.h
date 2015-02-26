
template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE ezBlockStorage<T, BlockSize, CompactStorage>::ConstIterator::ConstIterator(const ezBlockStorage<T, BlockSize, CompactStorage>& storage, ezUInt32 uiStartIndex, ezUInt32 uiCount) :
  m_Storage(storage)
{
  m_uiCurrentIndex = uiStartIndex;
  m_uiEndIndex = ezMath::Max(uiStartIndex + uiCount, uiCount);

  if (!CompactStorage)
  {
    ezUInt32 uiEndIndex = ezMath::Min(m_uiEndIndex, m_Storage.m_uiCount);
    while (m_uiCurrentIndex < uiEndIndex && !m_Storage.m_UsedEntries.IsSet(m_uiCurrentIndex))
    {
      ++m_uiCurrentIndex;
    }
  }
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE T& ezBlockStorage<T, BlockSize, CompactStorage>::ConstIterator::CurrentElement() const
{
  const ezUInt32 uiBlockIndex = m_uiCurrentIndex / ezDataBlock<T, BlockSize>::CAPACITY;
  const ezUInt32 uiInnerIndex = m_uiCurrentIndex - uiBlockIndex * ezDataBlock<T, BlockSize>::CAPACITY;
  return m_Storage.m_Blocks[uiBlockIndex][uiInnerIndex];
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE const T& ezBlockStorage<T, BlockSize, CompactStorage>::ConstIterator::operator*() const
{
  return CurrentElement();
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE const T* ezBlockStorage<T, BlockSize, CompactStorage>::ConstIterator::operator->() const
{
  return &CurrentElement();
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE ezBlockStorage<T, BlockSize, CompactStorage>::ConstIterator::operator const T*() const
{
  return &CurrentElement();
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE void ezBlockStorage<T, BlockSize, CompactStorage>::ConstIterator::Next()
{
  ++m_uiCurrentIndex;

  if (!CompactStorage)
  {
    ezUInt32 uiEndIndex = ezMath::Min(m_uiEndIndex, m_Storage.m_uiCount);
    while (m_uiCurrentIndex < uiEndIndex && !m_Storage.m_UsedEntries.IsSet(m_uiCurrentIndex))
    {
      ++m_uiCurrentIndex;
    }
  }
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE bool ezBlockStorage<T, BlockSize, CompactStorage>::ConstIterator::IsValid() const
{
  return m_uiCurrentIndex < ezMath::Min(m_uiEndIndex, m_Storage.m_uiCount);
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE void ezBlockStorage<T, BlockSize, CompactStorage>::ConstIterator::operator++()
{
  Next();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE ezBlockStorage<T, BlockSize, CompactStorage>::Iterator::Iterator(const ezBlockStorage<T, BlockSize, CompactStorage>& storage, ezUInt32 uiStartIndex, ezUInt32 uiCount) :
  ConstIterator(storage, uiStartIndex, uiCount)
{
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE T& ezBlockStorage<T, BlockSize, CompactStorage>::Iterator::operator*()
{
  return this->CurrentElement();
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE T* ezBlockStorage<T, BlockSize, CompactStorage>::Iterator::operator->()
{
  return &(this->CurrentElement());
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE ezBlockStorage<T, BlockSize, CompactStorage>::Iterator::operator T*()
{
  return &(this->CurrentElement());
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE bool ezBlockStorage<T, BlockSize, CompactStorage>::Entry::operator<(const Entry& rhs) const
{
  return m_uiIndex < rhs.m_uiIndex;
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE bool ezBlockStorage<T, BlockSize, CompactStorage>::Entry::operator>(const Entry& rhs) const
{
  return m_uiIndex > rhs.m_uiIndex;
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE bool ezBlockStorage<T, BlockSize, CompactStorage>::Entry::operator==(const Entry& rhs) const
{
  return m_uiIndex == rhs.m_uiIndex;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE ezBlockStorage<T, BlockSize, CompactStorage>::ezBlockStorage(ezLargeBlockAllocator<BlockSize>* pBlockAllocator, ezAllocatorBase* pAllocator)
  : m_pBlockAllocator(pBlockAllocator)
  , m_Blocks(pAllocator)
  , m_uiCount(0)
  , m_uiFreelistStart(ezInvalidIndex)
{
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
ezBlockStorage<T, BlockSize, CompactStorage>::~ezBlockStorage()
{
  for (ezUInt32 uiBlockIndex = 0; uiBlockIndex < m_Blocks.GetCount(); ++uiBlockIndex)
  {
    ezDataBlock<T, BlockSize>& block = m_Blocks[uiBlockIndex];

    if (CompactStorage)
    {
      ezMemoryUtils::Destruct(block.m_pData, block.m_uiCount);
    }
    else
    {
      for (ezUInt32 uiInnerIndex = 0; uiInnerIndex < block.m_uiCount; ++uiInnerIndex)
      {
        ezUInt32 uiIndex = uiBlockIndex * ezDataBlock<T, BlockSize>::CAPACITY + uiInnerIndex;
        if (m_UsedEntries.IsSet(uiIndex))
        {
          ezMemoryUtils::Destruct(&block.m_pData[uiInnerIndex], 1);
        }
      }
    }
    
    m_pBlockAllocator->DeallocateBlock(block);
  }

  m_Blocks.Clear();
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
typename ezBlockStorage<T, BlockSize, CompactStorage>::Entry ezBlockStorage<T, BlockSize, CompactStorage>::Create()
{
  Entry entry;

  if (!CompactStorage && m_uiFreelistStart != ezInvalidIndex)
  {
    entry.m_uiIndex = m_uiFreelistStart;

    const ezUInt32 uiBlockIndex = entry.m_uiIndex / ezDataBlock<T, BlockSize>::CAPACITY;
    const ezUInt32 uiInnerIndex = entry.m_uiIndex - uiBlockIndex * ezDataBlock<T, BlockSize>::CAPACITY;

    entry.m_Ptr = &(m_Blocks[uiBlockIndex][uiInnerIndex]);

    m_uiFreelistStart = *reinterpret_cast<ezUInt32*>(entry.m_Ptr);
  }
  else
  {
    ezDataBlock<T, BlockSize>* pBlock = nullptr;

    if (m_Blocks.GetCount() > 0)
    {
      pBlock = &m_Blocks.PeekBack();
    }

    if (pBlock == nullptr || pBlock->IsFull())
    {
      m_Blocks.PushBack(m_pBlockAllocator->template AllocateBlock<T>());
      pBlock = &m_Blocks.PeekBack();
    }

    entry.m_Ptr = pBlock->ReserveBack();
    entry.m_uiIndex = m_uiCount;

    ++m_uiCount;
  }

  ezMemoryUtils::Construct(entry.m_Ptr, 1);

  if (!CompactStorage)
  {
    m_UsedEntries.SetCount(m_uiCount);
    m_UsedEntries.SetBit(entry.m_uiIndex);
  }

  return entry;
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE void ezBlockStorage<T, BlockSize, CompactStorage>::Delete(Entry entry)
{
  T* pDummy;
  Delete(entry, pDummy);
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
void ezBlockStorage<T, BlockSize, CompactStorage>::Delete(Entry entry, T*& out_pMovedObject)
{
  EZ_ASSERT_DEV(entry.m_uiIndex < m_uiCount, "Out of bounds access. Block storage has %i objects, trying to remove object at index %i.", 
    m_uiCount, entry.m_uiIndex);

  const ezUInt32 uiBlockIndex = entry.m_uiIndex / ezDataBlock<T, BlockSize>::CAPACITY;
  const ezUInt32 uiInnerIndex = entry.m_uiIndex - uiBlockIndex * ezDataBlock<T, BlockSize>::CAPACITY;
  EZ_ASSERT_DEV(&(m_Blocks[uiBlockIndex][uiInnerIndex]) == entry.m_Ptr, "Memory Corruption");

  if (!CompactStorage)
  {
    m_UsedEntries.ClearBit(entry.m_uiIndex);

    out_pMovedObject = entry.m_Ptr;
    ezMemoryUtils::Destruct(entry.m_Ptr, 1);

    *reinterpret_cast<ezUInt32*>(entry.m_Ptr) = m_uiFreelistStart;
    m_uiFreelistStart = entry.m_uiIndex;
  }
  else
  {
    ezDataBlock<T, BlockSize>& lastBlock = m_Blocks.PeekBack();
    T* pLast = lastBlock.PopBack();

    --m_uiCount;
    if (m_uiCount != entry.m_uiIndex)
    {
      ezMemoryUtils::Copy(entry.m_Ptr, pLast, 1);
    }

    out_pMovedObject = pLast;
    ezMemoryUtils::Destruct(pLast, 1);

    if (lastBlock.IsEmpty())
    {
      m_pBlockAllocator->DeallocateBlock(lastBlock);
      m_Blocks.PopBack();
    }
  }
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE ezUInt32 ezBlockStorage<T, BlockSize, CompactStorage>::GetCount() const
{
  return m_uiCount;
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE typename ezBlockStorage<T, BlockSize, CompactStorage>::Iterator ezBlockStorage<T, BlockSize, CompactStorage>::GetIterator(ezUInt32 uiStartIndex /*= 0*/, ezUInt32 uiCount /*= ezInvalidIndex*/)
{
  return Iterator(*this, uiStartIndex, uiCount);
}

template <typename T, ezUInt32 BlockSize, bool CompactStorage>
EZ_FORCE_INLINE typename ezBlockStorage<T, BlockSize, CompactStorage>::ConstIterator ezBlockStorage<T, BlockSize, CompactStorage>::GetIterator(ezUInt32 uiStartIndex /*= 0*/, ezUInt32 uiCount /*= ezInvalidIndex*/) const
{
  return ConstIterator(*this, uiStartIndex, uiCount);
}

