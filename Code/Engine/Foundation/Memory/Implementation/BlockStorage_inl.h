
template <typename T>
EZ_FORCE_INLINE ezBlockStorage<T>::Iterator::Iterator(ezBlockStorage<T>& storage, ezUInt32 uiStartIndex, ezUInt32 uiCount) : 
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
EZ_FORCE_INLINE ezBlockStorage<T>::ezBlockStorage(ezLargeBlockAllocator* pBlockAllocator, ezIAllocator* pAllocator) : 
  m_pBlockAllocator(pBlockAllocator), m_Blocks(pAllocator), m_uiCount(0)
{
}

template <typename T>
ezBlockStorage<T>::~ezBlockStorage()
{
  for (ezUInt32 i = 0; i < m_Blocks.GetCount(); ++i)
  {
    m_pBlockAllocator->DeallocateBlock(m_Blocks[i]);
  }

  m_Blocks.Clear();
}

template <typename T>
T* ezBlockStorage<T>::Create()
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

template <typename T>
void ezBlockStorage<T>::Delete(ezUInt32 uiIndex)
{

  const ezUInt32 uiBlockIndex = uiIndex / ezDataBlock<T>::CAPACITY;
  const ezUInt32 uiInnerIndex = uiIndex - uiBlockIndex * ezDataBlock<T>::CAPACITY;


}

template <typename T>
typename ezBlockStorage<T>::Iterator ezBlockStorage<T>::GetIterator(ezUInt32 uiStartIndex /*= 0*/, ezUInt32 uiCount /*= ezInvalidIndex*/)
{
  return Iterator(*this, uiStartIndex, uiCount);
}
