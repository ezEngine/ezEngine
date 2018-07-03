
template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE ezBlockStorage<T, BlockSize, StorageType>::ConstIterator::ConstIterator(const ezBlockStorage<T, BlockSize, StorageType>& storage, ezUInt32 uiStartIndex, ezUInt32 uiCount)
    : m_Storage(storage)
{
  m_uiCurrentIndex = uiStartIndex;
  m_uiEndIndex = ezMath::Max(uiStartIndex + uiCount, uiCount);

  if (StorageType == ezBlockStorageType::FreeList)
  {
    ezUInt32 uiEndIndex = ezMath::Min(m_uiEndIndex, m_Storage.m_uiCount);
    while (m_uiCurrentIndex < uiEndIndex && !m_Storage.m_UsedEntries.IsSet(m_uiCurrentIndex))
    {
      ++m_uiCurrentIndex;
    }
  }
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE T& ezBlockStorage<T, BlockSize, StorageType>::ConstIterator::CurrentElement() const
{
  const ezUInt32 uiBlockIndex = m_uiCurrentIndex / ezDataBlock<T, BlockSize>::CAPACITY;
  const ezUInt32 uiInnerIndex = m_uiCurrentIndex - uiBlockIndex * ezDataBlock<T, BlockSize>::CAPACITY;
  return m_Storage.m_Blocks[uiBlockIndex][uiInnerIndex];
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_ALWAYS_INLINE const T& ezBlockStorage<T, BlockSize, StorageType>::ConstIterator::operator*() const
{
  return CurrentElement();
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_ALWAYS_INLINE const T* ezBlockStorage<T, BlockSize, StorageType>::ConstIterator::operator->() const
{
  return &CurrentElement();
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_ALWAYS_INLINE ezBlockStorage<T, BlockSize, StorageType>::ConstIterator::operator const T*() const
{
  return &CurrentElement();
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE void ezBlockStorage<T, BlockSize, StorageType>::ConstIterator::Next()
{
  ++m_uiCurrentIndex;

  if (StorageType == ezBlockStorageType::FreeList)
  {
    ezUInt32 uiEndIndex = ezMath::Min(m_uiEndIndex, m_Storage.m_uiCount);
    while (m_uiCurrentIndex < uiEndIndex && !m_Storage.m_UsedEntries.IsSet(m_uiCurrentIndex))
    {
      ++m_uiCurrentIndex;
    }
  }
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE bool ezBlockStorage<T, BlockSize, StorageType>::ConstIterator::IsValid() const
{
  return m_uiCurrentIndex < ezMath::Min(m_uiEndIndex, m_Storage.m_uiCount);
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_ALWAYS_INLINE void ezBlockStorage<T, BlockSize, StorageType>::ConstIterator::operator++()
{
  Next();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE ezBlockStorage<T, BlockSize, StorageType>::Iterator::Iterator(const ezBlockStorage<T, BlockSize, StorageType>& storage, ezUInt32 uiStartIndex, ezUInt32 uiCount)
    : ConstIterator(storage, uiStartIndex, uiCount)
{
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_ALWAYS_INLINE T& ezBlockStorage<T, BlockSize, StorageType>::Iterator::operator*()
{
  return this->CurrentElement();
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_ALWAYS_INLINE T* ezBlockStorage<T, BlockSize, StorageType>::Iterator::operator->()
{
  return &(this->CurrentElement());
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_ALWAYS_INLINE ezBlockStorage<T, BlockSize, StorageType>::Iterator::operator T*()
{
  return &(this->CurrentElement());
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE ezBlockStorage<T, BlockSize, StorageType>::ezBlockStorage(ezLargeBlockAllocator<BlockSize>* pBlockAllocator, ezAllocatorBase* pAllocator)
    : m_pBlockAllocator(pBlockAllocator)
    , m_Blocks(pAllocator)
    , m_uiCount(0)
    , m_uiFreelistStart(ezInvalidIndex)
{
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
ezBlockStorage<T, BlockSize, StorageType>::~ezBlockStorage()
{
  for (ezUInt32 uiBlockIndex = 0; uiBlockIndex < m_Blocks.GetCount(); ++uiBlockIndex)
  {
    ezDataBlock<T, BlockSize>& block = m_Blocks[uiBlockIndex];

    if (StorageType == ezBlockStorageType::Compact)
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

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
T* ezBlockStorage<T, BlockSize, StorageType>::Create()
{
  T* pNewObject = nullptr;
  ezUInt32 uiNewIndex = ezInvalidIndex;

  if (StorageType == ezBlockStorageType::FreeList && m_uiFreelistStart != ezInvalidIndex)
  {
    uiNewIndex = m_uiFreelistStart;

    const ezUInt32 uiBlockIndex = uiNewIndex / ezDataBlock<T, BlockSize>::CAPACITY;
    const ezUInt32 uiInnerIndex = uiNewIndex - uiBlockIndex * ezDataBlock<T, BlockSize>::CAPACITY;

    pNewObject = &(m_Blocks[uiBlockIndex][uiInnerIndex]);

    m_uiFreelistStart = *reinterpret_cast<ezUInt32*>(pNewObject);
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

    pNewObject = pBlock->ReserveBack();
    uiNewIndex = m_uiCount;

    ++m_uiCount;
  }

  ezMemoryUtils::Construct(pNewObject, 1);

  if (StorageType == ezBlockStorageType::FreeList)
  {
    m_UsedEntries.SetCount(m_uiCount);
    m_UsedEntries.SetBit(uiNewIndex);
  }

  return pNewObject;
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE void ezBlockStorage<T, BlockSize, StorageType>::Delete(T* pObject)
{
  T* pDummy;
  Delete(pObject, pDummy);
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
void ezBlockStorage<T, BlockSize, StorageType>::Delete(T* pObject, T*& out_pMovedObject)
{
  Delete(pObject, out_pMovedObject, ezTraitInt<StorageType>());
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_ALWAYS_INLINE ezUInt32 ezBlockStorage<T, BlockSize, StorageType>::GetCount() const
{
  return m_uiCount;
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_ALWAYS_INLINE typename ezBlockStorage<T, BlockSize, StorageType>::Iterator ezBlockStorage<T, BlockSize, StorageType>::GetIterator(ezUInt32 uiStartIndex /*= 0*/, ezUInt32 uiCount /*= ezInvalidIndex*/)
{
  return Iterator(*this, uiStartIndex, uiCount);
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_ALWAYS_INLINE typename ezBlockStorage<T, BlockSize, StorageType>::ConstIterator ezBlockStorage<T, BlockSize, StorageType>::GetIterator(ezUInt32 uiStartIndex /*= 0*/, ezUInt32 uiCount /*= ezInvalidIndex*/) const
{
  return ConstIterator(*this, uiStartIndex, uiCount);
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE void ezBlockStorage<T, BlockSize, StorageType>::Delete(T* pObject, T*& out_pMovedObject, ezTraitInt<ezBlockStorageType::Compact>)
{
  ezDataBlock<T, BlockSize>& lastBlock = m_Blocks.PeekBack();
  T* pLast = lastBlock.PopBack();

  --m_uiCount;
  if (pObject != pLast)
  {
    ezMemoryUtils::Relocate(pObject, pLast, 1);
  }
  else
  {
    ezMemoryUtils::Destruct(pLast, 1);
  }

  out_pMovedObject = pLast;

  if (lastBlock.IsEmpty())
  {
    m_pBlockAllocator->DeallocateBlock(lastBlock);
    m_Blocks.PopBack();
  }
}

template <typename T, ezUInt32 BlockSize, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE void ezBlockStorage<T, BlockSize, StorageType>::Delete(T* pObject, T*& out_pMovedObject, ezTraitInt<ezBlockStorageType::FreeList>)
{
  ezUInt32 uiIndex = ezInvalidIndex;
  for (ezUInt32 uiBlockIndex = 0; uiBlockIndex < m_Blocks.GetCount(); ++uiBlockIndex)
  {
    ptrdiff_t diff = pObject - m_Blocks[uiBlockIndex].m_pData;
    if (diff >= 0 && diff < ezDataBlock<T, BlockSize>::CAPACITY)
    {
      uiIndex = uiBlockIndex * ezDataBlock<T, BlockSize>::CAPACITY + (ezInt32)diff;
      break;
    }
  }

  EZ_ASSERT_DEV(uiIndex != ezInvalidIndex, "Invalid object {0} was not found in block storage.", ezArgP(pObject));

  m_UsedEntries.ClearBit(uiIndex);

  out_pMovedObject = pObject;
  ezMemoryUtils::Destruct(pObject, 1);

  *reinterpret_cast<ezUInt32*>(pObject) = m_uiFreelistStart;
  m_uiFreelistStart = uiIndex;
}
