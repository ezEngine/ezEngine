
#define ezInvalidIndex 0xFFFFFFFF

// ***** Const Iterator *****

template <typename K, typename H>
ezHashSetBase<K, H>::ConstIterator::ConstIterator(const ezHashSetBase<K, H>& hashSet) :
  m_hashSet(hashSet), m_uiCurrentIndex(0), m_uiCurrentCount(0)
{
}

template <typename K, typename H>
void ezHashSetBase<K, H>::ConstIterator::SetToBegin()
{
  if (m_hashSet.IsEmpty())
    return;

  while (!m_hashSet.IsValidEntry(m_uiCurrentIndex))
  {
    ++m_uiCurrentIndex;
  }
}

template <typename K, typename H>
EZ_FORCE_INLINE void ezHashSetBase<K, H>::ConstIterator::SetToEnd()
{
  m_uiCurrentCount = m_hashSet.m_uiCount;
  m_uiCurrentIndex = m_hashSet.m_uiCapacity;

  if (m_hashSet.IsEmpty())
    return;

  while (!m_hashSet.IsValidEntry(m_uiCurrentIndex-1))
  {
    --m_uiCurrentIndex;
  }
}

template <typename K, typename H>
EZ_FORCE_INLINE bool ezHashSetBase<K, H>::ConstIterator::IsValid() const
{
  return m_uiCurrentCount < m_hashSet.m_uiCount;
}

template <typename K, typename H>
EZ_FORCE_INLINE bool ezHashSetBase<K, H>::ConstIterator::operator==(const typename ezHashSetBase<K, H>::ConstIterator& it2) const
{
  return m_hashSet.m_pEntries == it2.m_hashSet.m_pEntries && m_uiCurrentIndex == it2.m_uiCurrentIndex;
}

template <typename K, typename H>
EZ_FORCE_INLINE bool ezHashSetBase<K, H>::ConstIterator::operator!=(const typename ezHashSetBase<K, H>::ConstIterator& it2) const
{
  return !(*this == it2);
}

template <typename K, typename H>
EZ_FORCE_INLINE const K& ezHashSetBase<K, H>::ConstIterator::Key() const
{
  return m_hashSet.m_pEntries[m_uiCurrentIndex];
}

template <typename K, typename H>
void ezHashSetBase<K, H>::ConstIterator::Next()
{
  ++m_uiCurrentCount;
  if (m_uiCurrentCount == m_hashSet.m_uiCount)
    return;

  do
  {
    ++m_uiCurrentIndex;
  }
  while (!m_hashSet.IsValidEntry(m_uiCurrentIndex));
}

template <typename K, typename H>
EZ_FORCE_INLINE void ezHashSetBase<K, H>::ConstIterator::operator++()
{
  Next();
}


// ***** ezHashSetBase *****

template <typename K, typename H>
ezHashSetBase<K, H>::ezHashSetBase(ezAllocatorBase* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;
}

template <typename K, typename H>
ezHashSetBase<K, H>::ezHashSetBase(const ezHashSetBase<K, H>& other, ezAllocatorBase* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;

  *this = other;
}

template <typename K, typename H>
ezHashSetBase<K, H>::ezHashSetBase(ezHashSetBase<K, H>&& other, ezAllocatorBase* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;

  *this = std::move(other);
}

template <typename K, typename H>
ezHashSetBase<K, H>::~ezHashSetBase()
{
  Clear();
  EZ_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
  EZ_DELETE_RAW_BUFFER(m_pAllocator, m_pEntryFlags);
  m_uiCapacity = 0;
}

template <typename K, typename H>
void ezHashSetBase<K, H>::operator= (const ezHashSetBase<K, H>& rhs)
{
  Clear();
  Reserve(rhs.GetCount());

  ezUInt32 uiCopied = 0;
  for (ezUInt32 i = 0; uiCopied < rhs.GetCount(); ++i)
  {
    if (rhs.IsValidEntry(i))
    {
      Insert(rhs.m_pEntries[i]);
      ++uiCopied;
    }
  }
}

template <typename K, typename H>
void ezHashSetBase<K, H>::operator= (ezHashSetBase<K, H>&& rhs)
{
  // Clear any existing data (calls destructors if necessary)
  Clear();

  if (m_pAllocator != rhs.m_pAllocator)
  {
    Reserve(rhs.m_uiCapacity);

    ezUInt32 uiCopied = 0;
    for (ezUInt32 i = 0; uiCopied < rhs.GetCount(); ++i)
    {
      if (rhs.IsValidEntry(i))
      {
        Insert(std::move(rhs.m_pEntries[i]));
        ++uiCopied;
      }
    }

    rhs.Clear();
  }
  else
  {
    EZ_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
    EZ_DELETE_RAW_BUFFER(m_pAllocator, m_pEntryFlags);

    // Move all data over.
    m_pEntries = rhs.m_pEntries;
    m_pEntryFlags = rhs.m_pEntryFlags;
    m_uiCount = rhs.m_uiCount;
    m_uiCapacity = rhs.m_uiCapacity;

    // Temp copy forgets all its state.
    rhs.m_pEntries = nullptr;
    rhs.m_pEntryFlags = nullptr;
    rhs.m_uiCount = 0;
    rhs.m_uiCapacity = 0;
  }
}

template <typename K, typename H>
bool ezHashSetBase<K, H>::operator== (const ezHashSetBase<K, H>& rhs) const
{
  if (m_uiCount != rhs.m_uiCount)
    return false;

  ezUInt32 uiCompared = 0;
  for (ezUInt32 i = 0; uiCompared < m_uiCount; ++i)
  {
    if (IsValidEntry(i))
    {
      if (!rhs.Contains(m_pEntries[i]))
        return false;

      ++uiCompared;
    }
  }

  return true;
}

template <typename K, typename H>
EZ_FORCE_INLINE bool ezHashSetBase<K, H>::operator!= (const ezHashSetBase<K, H>& rhs) const
{
  return !(*this == rhs);
}

template <typename K, typename H>
void ezHashSetBase<K, H>::Reserve(ezUInt32 uiCapacity)
{
  ezUInt32 uiNewCapacity = uiCapacity + (uiCapacity / 3) * 2; // ensure a maximum load of 60%
  if (m_uiCapacity >= uiNewCapacity)
    return;

  uiNewCapacity = ezMath::Max<ezUInt32>(ezMath::PowerOfTwo_Ceil(uiNewCapacity), CAPACITY_ALIGNMENT);
  SetCapacity(uiNewCapacity);
}

template <typename K, typename H>
void ezHashSetBase<K, H>::Compact()
{
  if (IsEmpty())
  {
    // completely deallocate all data, if the table is empty.
    EZ_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
    EZ_DELETE_RAW_BUFFER(m_pAllocator, m_pEntryFlags);
    m_uiCapacity = 0;
  }
  else
  {
    const ezUInt32 uiNewCapacity = (m_uiCount + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);
    if (m_uiCapacity != uiNewCapacity)
      SetCapacity(uiNewCapacity);
  }
}

template <typename K, typename H>
EZ_FORCE_INLINE ezUInt32 ezHashSetBase<K, H>::GetCount() const
{
  return m_uiCount;
}

template <typename K, typename H>
EZ_FORCE_INLINE bool ezHashSetBase<K, H>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename K, typename H>
void ezHashSetBase<K, H>::Clear()
{
  for (ezUInt32 i = 0; i < m_uiCapacity; ++i)
  {
    if (IsValidEntry(i))
    {
      ezMemoryUtils::Destruct(&m_pEntries[i], 1);
    }
  }

  ezMemoryUtils::ZeroFill(m_pEntryFlags, GetFlagsCapacity());
  m_uiCount = 0;
}

template <typename K, typename H>
template <typename CompatibleKeyType>
bool ezHashSetBase<K, H>::Insert(CompatibleKeyType&& key)
{
  Reserve(m_uiCount + 1);

  ezUInt32 uiIndex = H::Hash(key) % m_uiCapacity;
  ezUInt32 uiDeletedIndex = ezInvalidIndex;

  ezUInt32 uiCounter = 0;
  while (!IsFreeEntry(uiIndex) && uiCounter < m_uiCapacity)
  {
    if (IsDeletedEntry(uiIndex))
    {
      if (uiDeletedIndex == ezInvalidIndex)
        uiDeletedIndex = uiIndex;
    }
    else if (H::Equal(m_pEntries[uiIndex], key))
    {
      return true;
    }
    ++uiIndex;
    if (uiIndex == m_uiCapacity)
      uiIndex = 0;

    ++uiCounter;
  }

  // new entry
  uiIndex = uiDeletedIndex != ezInvalidIndex ? uiDeletedIndex : uiIndex;

  // Constructions might either be a move or a copy.
  ezMemoryUtils::CopyOrMoveConstruct(&m_pEntries[uiIndex], std::forward<CompatibleKeyType>(key));

  MarkEntryAsValid(uiIndex);
  ++m_uiCount;

  return false;
}

template <typename K, typename H>
bool ezHashSetBase<K, H>::Remove(const K& key)
{
  ezUInt32 uiIndex = FindEntry(key);
  if (uiIndex != ezInvalidIndex)
  {
    ezMemoryUtils::Destruct(&m_pEntries[uiIndex], 1);

    ezUInt32 uiNextIndex = uiIndex + 1;
    if (uiNextIndex == m_uiCapacity)
      uiNextIndex = 0;

    // if the next entry is free we are at the end of a chain and
    // can immediately mark this entry as free as well
    if (IsFreeEntry(uiNextIndex))
    {
      MarkEntryAsFree(uiIndex);

      // run backwards and free all deleted entries in this chain
      ezUInt32 uiPrevIndex = (uiIndex != 0) ? uiIndex : m_uiCapacity;
      --uiPrevIndex;

      while (IsDeletedEntry(uiPrevIndex))
      {
        MarkEntryAsFree(uiPrevIndex);

        if (uiPrevIndex == 0)
          uiPrevIndex = m_uiCapacity;
        --uiPrevIndex;
      }
    }
    else
    {
      MarkEntryAsDeleted(uiIndex);
    }

    --m_uiCount;
    return true;
  }

  return false;
}

template <typename K, typename H>
EZ_FORCE_INLINE bool ezHashSetBase<K, H>::Contains(const K& key) const
{
  return FindEntry(key) != ezInvalidIndex;
}

template <typename K, typename H>
EZ_FORCE_INLINE typename ezHashSetBase<K, H>::ConstIterator ezHashSetBase<K, H>::GetIterator() const
{
  ConstIterator iterator(*this);
  iterator.SetToBegin();
  return iterator;
}

template <typename K, typename H>
EZ_FORCE_INLINE typename ezHashSetBase<K, H>::ConstIterator ezHashSetBase<K, H>::GetEndIterator() const
{
  ConstIterator iterator(*this);
  iterator.SetToEnd();
  return iterator;
}

template <typename K, typename H>
EZ_FORCE_INLINE ezAllocatorBase* ezHashSetBase<K, H>::GetAllocator() const
{
  return m_pAllocator;
}

template <typename K, typename H>
ezUInt64 ezHashSetBase<K, H>::GetHeapMemoryUsage() const
{
  return ((ezUInt64) m_uiCapacity * sizeof(K)) + (sizeof(ezUInt32) * (ezUInt64) GetFlagsCapacity());
}

// private methods
template <typename K, typename H>
void ezHashSetBase<K, H>::SetCapacity(ezUInt32 uiCapacity)
{
  const ezUInt32 uiOldCapacity = m_uiCapacity;
  m_uiCapacity = uiCapacity;

  K* pOldEntries = m_pEntries;
  ezUInt32* pOldEntryFlags = m_pEntryFlags;

  m_pEntries = EZ_NEW_RAW_BUFFER(m_pAllocator, K, m_uiCapacity);
  m_pEntryFlags = EZ_NEW_RAW_BUFFER(m_pAllocator, ezUInt32, GetFlagsCapacity());
  ezMemoryUtils::ZeroFill(m_pEntryFlags, GetFlagsCapacity());

  m_uiCount = 0;
  for (ezUInt32 i = 0; i < uiOldCapacity; ++i)
  {
    if (GetFlags(pOldEntryFlags, i) == VALID_ENTRY)
    {
      EZ_VERIFY(!Insert(std::move(pOldEntries[i])), "Implementation error");

      ezMemoryUtils::Destruct(&pOldEntries[i], 1);
    }
  }

  EZ_DELETE_RAW_BUFFER(m_pAllocator, pOldEntries);
  EZ_DELETE_RAW_BUFFER(m_pAllocator, pOldEntryFlags);
}

template <typename K, typename H>
EZ_FORCE_INLINE ezUInt32 ezHashSetBase<K, H>::FindEntry(const K& key) const
{
  return FindEntry(H::Hash(key), key);
}

template <typename K, typename H>
inline ezUInt32 ezHashSetBase<K, H>::FindEntry(ezUInt32 uiHash, const K& key) const
{
  if (m_uiCapacity > 0)
  {
    ezUInt32 uiIndex = uiHash % m_uiCapacity;
    ezUInt32 uiCounter = 0;
    while (!IsFreeEntry(uiIndex) && uiCounter < m_uiCapacity)
    {
      if (IsValidEntry(uiIndex) && H::Equal(m_pEntries[uiIndex], key))
        return uiIndex;

      ++uiIndex;
      if (uiIndex == m_uiCapacity)
        uiIndex = 0;

      ++uiCounter;
    }
  }
  // not found
  return ezInvalidIndex;
}

#define EZ_HASHSET_USE_BITFLAGS EZ_ON

template <typename K, typename H>
EZ_FORCE_INLINE ezUInt32 ezHashSetBase<K, H>::GetFlagsCapacity() const
{
#if EZ_ENABLED(EZ_HASHSET_USE_BITFLAGS)
  return (m_uiCapacity + 15) / 16;
#else
  return m_uiCapacity;
#endif
}

template <typename K, typename H>
ezUInt32 ezHashSetBase<K, H>::GetFlags(ezUInt32* pFlags, ezUInt32 uiEntryIndex) const
{
#if EZ_ENABLED(EZ_HASHSET_USE_BITFLAGS)
  const ezUInt32 uiIndex = uiEntryIndex / 16;
  const ezUInt32 uiSubIndex = (uiEntryIndex & 15) * 2;
  return (pFlags[uiIndex] >> uiSubIndex) & FLAGS_MASK;
#else
  return pFlags[uiEntryIndex] & FLAGS_MASK;
#endif
}

template <typename K, typename H>
void ezHashSetBase<K, H>::SetFlags(ezUInt32 uiEntryIndex, ezUInt32 uiFlags)
{
#if EZ_ENABLED(EZ_HASHSET_USE_BITFLAGS)
  const ezUInt32 uiIndex = uiEntryIndex / 16;
  const ezUInt32 uiSubIndex = (uiEntryIndex & 15) * 2;
  EZ_ASSERT_DEV(uiIndex < GetFlagsCapacity(), "Out of bounds access");
  m_pEntryFlags[uiIndex] &= ~(FLAGS_MASK << uiSubIndex);
  m_pEntryFlags[uiIndex] |= (uiFlags << uiSubIndex);
#else
  EZ_ASSERT_DEV(uiEntryIndex < GetFlagsCapacity(), "Out of bounds access");
  m_pEntryFlags[uiEntryIndex] = uiFlags;
#endif
}

template <typename K, typename H>
EZ_FORCE_INLINE bool ezHashSetBase<K, H>::IsFreeEntry(ezUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == FREE_ENTRY;
}

template <typename K, typename H>
EZ_FORCE_INLINE bool ezHashSetBase<K, H>::IsValidEntry(ezUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == VALID_ENTRY;
}

template <typename K, typename H>
EZ_FORCE_INLINE bool ezHashSetBase<K, H>::IsDeletedEntry(ezUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == DELETED_ENTRY;
}

template <typename K, typename H>
EZ_FORCE_INLINE void ezHashSetBase<K, H>::MarkEntryAsFree(ezUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, FREE_ENTRY);
}

template <typename K, typename H>
EZ_FORCE_INLINE void ezHashSetBase<K, H>::MarkEntryAsValid(ezUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, VALID_ENTRY);
}

template <typename K, typename H>
EZ_FORCE_INLINE void ezHashSetBase<K, H>::MarkEntryAsDeleted(ezUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, DELETED_ENTRY);
}


template <typename K, typename H, typename A>
ezHashSet<K, H, A>::ezHashSet() : ezHashSetBase<K, H>(A::GetAllocator())
{
}

template <typename K, typename H, typename A>
ezHashSet<K, H, A>::ezHashSet(ezAllocatorBase* pAllocator) : ezHashSetBase<K, H>(pAllocator)
{
}

template <typename K, typename H, typename A>
ezHashSet<K, H, A>::ezHashSet(const ezHashSet<K, H, A>& other) : ezHashSetBase<K, H>(other, A::GetAllocator())
{
}

template <typename K, typename H, typename A>
ezHashSet<K, H, A>::ezHashSet(const ezHashSetBase<K, H>& other) : ezHashSetBase<K, H>(other, A::GetAllocator())
{
}

template <typename K, typename H, typename A>
ezHashSet<K, H, A>::ezHashSet(ezHashSet<K, H, A>&& other) : ezHashSetBase<K, H>(std::move(other), A::GetAllocator())
{
}

template <typename K, typename H, typename A>
ezHashSet<K, H, A>::ezHashSet(ezHashSetBase<K, H>&& other) : ezHashSetBase<K, H>(std::move(other), A::GetAllocator())
{
}

template <typename K, typename H, typename A>
void ezHashSet<K, H, A>::operator=(const ezHashSet<K, H, A>& rhs)
{
  ezHashSetBase<K, H>::operator=(rhs);
}

template <typename K, typename H, typename A>
void ezHashSet<K, H, A>::operator=(const ezHashSetBase<K, H>& rhs)
{
  ezHashSetBase<K, H>::operator=(rhs);
}

template <typename K, typename H, typename A>
void ezHashSet<K, H, A>::operator=(ezHashSet<K, H, A>&& rhs)
{
  ezHashSetBase<K, H>::operator=(std::move(rhs));
}

template <typename K, typename H, typename A>
void ezHashSet<K, H, A>::operator=(ezHashSetBase<K, H>&& rhs)
{
  ezHashSetBase<K, H>::operator=(std::move(rhs));
}

