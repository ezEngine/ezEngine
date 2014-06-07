
template <typename T>
EZ_FORCE_INLINE ezBlockStorage<T>::Iterator::Iterator(const ezBlockStorage<T>& storage, ezUInt32 uiStartIndex, ezUInt32 uiCount) : 
  m_Storage(storage)
{
  m_uiCurrentIndex = uiStartIndex;
  m_uiEndIndex = ezMath::Max(uiStartIndex + uiCount, uiCount);
}

template <typename T>
EZ_FORCE_INLINE T& ezBlockStorage<T>::Iterator::operator*() const
{
  const ezUInt32 uiBlockIndex = m_uiCurrentIndex / ezDataBlock<T>::CAPACITY;
  const ezUInt32 uiInnerIndex = m_uiCurrentIndex - uiBlockIndex * ezDataBlock<T>::CAPACITY;
  return m_Storage.m_Blocks[uiBlockIndex][uiInnerIndex];
}

template <typename T>
EZ_FORCE_INLINE T* ezBlockStorage<T>::Iterator::operator->() const
{
  const ezUInt32 uiBlockIndex = m_uiCurrentIndex / ezDataBlock<T>::CAPACITY;
  const ezUInt32 uiInnerIndex = m_uiCurrentIndex - uiBlockIndex * ezDataBlock<T>::CAPACITY;  
  return m_Storage.m_Blocks[uiBlockIndex].m_pData + uiInnerIndex;
}

template <typename T>
EZ_FORCE_INLINE ezBlockStorage<T>::Iterator::operator T*() const
{
  const ezUInt32 uiBlockIndex = m_uiCurrentIndex / ezDataBlock<T>::CAPACITY;
  const ezUInt32 uiInnerIndex = m_uiCurrentIndex - uiBlockIndex * ezDataBlock<T>::CAPACITY;
  return m_Storage.m_Blocks[uiBlockIndex].m_pData + uiInnerIndex;
}

template <typename T>
EZ_FORCE_INLINE void ezBlockStorage<T>::Iterator::Next()
{
  ++m_uiCurrentIndex;
}

template <typename T>
EZ_FORCE_INLINE bool ezBlockStorage<T>::Iterator::IsValid() const
{
  return m_uiCurrentIndex < ezMath::Min(m_uiEndIndex, m_Storage.m_uiCount);
}

template <typename T>
EZ_FORCE_INLINE void ezBlockStorage<T>::Iterator::operator++()
{
  Next();
}


template <typename T>
EZ_FORCE_INLINE bool ezBlockStorage<T>::Entry::operator<(const Entry& rhs) const
{
  return m_uiIndex < rhs.m_uiIndex;
}

template <typename T>
EZ_FORCE_INLINE bool ezBlockStorage<T>::Entry::operator>(const Entry& rhs) const
{
  return m_uiIndex > rhs.m_uiIndex;
}

template <typename T>
EZ_FORCE_INLINE bool ezBlockStorage<T>::Entry::operator==(const Entry& rhs) const
{
  return m_uiIndex == rhs.m_uiIndex;
}



template <typename T>
EZ_FORCE_INLINE ezBlockStorage<T>::ezBlockStorage(ezLargeBlockAllocator* pBlockAllocator, ezAllocatorBase* pAllocator) : 
  m_pBlockAllocator(pBlockAllocator), m_Blocks(pAllocator), m_uiCount(0)
{
}

template <typename T>
ezBlockStorage<T>::~ezBlockStorage()
{
  for (ezUInt32 i = 0; i < m_Blocks.GetCount(); ++i)
  {
    ezDataBlock<T>& block = m_Blocks[i];

    ezMemoryUtils::Destruct(block.m_pData, block.m_uiCount);
    
    m_pBlockAllocator->DeallocateBlock(block);
  }

  m_Blocks.Clear();
}

template <typename T>
typename ezBlockStorage<T>::Entry ezBlockStorage<T>::Create()
{
  ezDataBlock<T>* pBlock = nullptr;

  if (m_Blocks.GetCount() > 0)
  {
    pBlock = &m_Blocks.PeekBack();
  }

  if (pBlock == nullptr || pBlock->IsFull())
  {
    m_Blocks.PushBack(m_pBlockAllocator->AllocateBlock<T>());
    pBlock = &m_Blocks.PeekBack();
  }

  Entry entry;
  entry.m_Ptr = pBlock->ReserveBack();
  entry.m_uiIndex = m_uiCount;

  ezMemoryUtils::Construct(entry.m_Ptr, 1);

  ++m_uiCount;
  return entry;
}

template <typename T>
EZ_FORCE_INLINE void ezBlockStorage<T>::Delete(Entry entry)
{
  T* pDummy;
  Delete(entry, pDummy);
}

template <typename T>
void ezBlockStorage<T>::Delete(Entry entry, T*& out_pMovedObject)
{
  EZ_ASSERT(entry.m_uiIndex < m_uiCount, "Out of bounds access. Block storage has %i objects, trying to remove object at index %i.", 
    m_uiCount, entry.m_uiIndex);

  ezDataBlock<T>& lastBlock = m_Blocks.PeekBack();
  T* pLast = lastBlock.PopBack();

  --m_uiCount;
  if (m_uiCount != entry.m_uiIndex)
  {
    const ezUInt32 uiBlockIndex = entry.m_uiIndex / ezDataBlock<T>::CAPACITY;
    const ezUInt32 uiInnerIndex = entry.m_uiIndex - uiBlockIndex * ezDataBlock<T>::CAPACITY;

    ezDataBlock<T>& block = m_Blocks[uiBlockIndex];
    EZ_ASSERT(&block[uiInnerIndex] == entry.m_Ptr, "Memory Corruption");

    ezMemoryUtils::Copy(&block[uiInnerIndex], pLast, 1);
  }

  out_pMovedObject = pLast;
  ezMemoryUtils::Destruct(pLast, 1);
  
  if (lastBlock.IsEmpty())
  {
    m_pBlockAllocator->DeallocateBlock(lastBlock);
    m_Blocks.PopBack();
  }
}

template <typename T>
EZ_FORCE_INLINE ezUInt32 ezBlockStorage<T>::GetCount() const
{
  return m_uiCount;
}

template <typename T>
EZ_FORCE_INLINE typename ezBlockStorage<T>::Iterator ezBlockStorage<T>::GetIterator(ezUInt32 uiStartIndex /*= 0*/, ezUInt32 uiCount /*= ezInvalidIndex*/) const
{
  return Iterator(*this, uiStartIndex, uiCount);
}

