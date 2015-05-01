
// ***** Const Iterator *****

template <typename IdType, typename ValueType>
ezIdTableBase<IdType, ValueType>::ConstIterator::ConstIterator(const ezIdTableBase<IdType, ValueType>& idTable) :
  m_idTable(idTable), m_uiCurrentIndex(0), m_uiCurrentCount(0)
{
  if (m_idTable.IsEmpty())
    return;

  while (m_idTable.m_pEntries[m_uiCurrentIndex].id.m_InstanceIndex != m_uiCurrentIndex)
  {
    ++m_uiCurrentIndex;
  }
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE bool ezIdTableBase<IdType, ValueType>::ConstIterator::IsValid() const
{
  return m_uiCurrentCount < m_idTable.m_uiCount;
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE bool ezIdTableBase<IdType, ValueType>::ConstIterator::operator==(const typename ezIdTableBase<IdType, ValueType>::ConstIterator& it2) const
{
  return m_idTable.m_pEntries == it2.m_idTable.m_pEntries && m_uiCurrentIndex == it2.m_uiCurrentIndex;
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE bool ezIdTableBase<IdType, ValueType>::ConstIterator::operator!=(const typename ezIdTableBase<IdType, ValueType>::ConstIterator& it2) const
{
  return !(*this == it2);
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE IdType ezIdTableBase<IdType, ValueType>::ConstIterator::Id() const
{
  return m_idTable.m_pEntries[m_uiCurrentIndex].id;
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE const ValueType& ezIdTableBase<IdType, ValueType>::ConstIterator::Value() const
{
  return m_idTable.m_pEntries[m_uiCurrentIndex].value;
}

template <typename IdType, typename ValueType>
void ezIdTableBase<IdType, ValueType>::ConstIterator::Next()
{
  ++m_uiCurrentCount;
  if (m_uiCurrentCount == m_idTable.m_uiCount)
    return;

  do
  {
    ++m_uiCurrentIndex;
  }
  while (m_idTable.m_pEntries[m_uiCurrentIndex].id.m_InstanceIndex != m_uiCurrentIndex);
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE void ezIdTableBase<IdType, ValueType>::ConstIterator::operator++()
{
  Next();
}


// ***** Iterator *****

template <typename IdType, typename ValueType>
ezIdTableBase<IdType, ValueType>::Iterator::Iterator(const ezIdTableBase<IdType, ValueType>& idTable) :
  ConstIterator(idTable)
{
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE ValueType& ezIdTableBase<IdType, ValueType>::Iterator::Value()
{
  return this->m_idTable.m_pEntries[this->m_uiCurrentIndex].value;
}


// ***** ezIdTableBase *****

template <typename IdType, typename ValueType>
ezIdTableBase<IdType, ValueType>::ezIdTableBase(ezAllocatorBase* pAllocator)
{
  m_pEntries = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_uiFreelistEnqueue = -1;
  m_uiFreelistDequeue = 0;
  m_pAllocator = pAllocator;
}

template <typename IdType, typename ValueType>
ezIdTableBase<IdType, ValueType>::ezIdTableBase(const ezIdTableBase<IdType, ValueType>& other, ezAllocatorBase* pAllocator)
{
  m_pEntries = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_uiFreelistEnqueue = -1;
  m_uiFreelistDequeue = 0;
  m_pAllocator = pAllocator;

  *this = other;
}

template <typename IdType, typename ValueType>
ezIdTableBase<IdType, ValueType>::~ezIdTableBase()
{
  for (IndexType i = 0; i < m_uiCapacity; ++i)
  {
    if (m_pEntries[i].id.m_InstanceIndex == i)
    {
      ezMemoryUtils::Destruct(&m_pEntries[i].value, 1);
    }
  }

  EZ_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
  m_uiCapacity = 0;
}

template <typename IdType, typename ValueType>
void ezIdTableBase<IdType, ValueType>::operator= (const ezIdTableBase<IdType, ValueType>& rhs)
{
  Clear();
  Reserve(rhs.m_uiCapacity);

  for (IndexType i = 0; i < rhs.m_uiCapacity; ++i)
  {
    Entry& entry = m_pEntries[i];

    entry.id = rhs.m_pEntries[i].id;
    if (entry.id.m_InstanceIndex == i)
    {
      ezMemoryUtils::CopyConstruct(&entry.value, rhs.m_pEntries[i].value, 1);
    }
  }

  m_uiCount = rhs.m_uiCount;
  m_uiFreelistDequeue = rhs.m_uiFreelistDequeue;
}

template <typename IdType, typename ValueType>
void ezIdTableBase<IdType, ValueType>::Reserve(IndexType uiCapacity)
{
  if (m_uiCapacity >= uiCapacity + CAPACITY_ALIGNMENT)
    return;

  IndexType uiNewCapacity = ezMath::Max(m_uiCapacity + (m_uiCapacity / 2), uiCapacity + CAPACITY_ALIGNMENT);
  uiNewCapacity = (uiNewCapacity + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);
  SetCapacity(uiNewCapacity);
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE typename ezIdTableBase<IdType, ValueType>::IndexType ezIdTableBase<IdType, ValueType>::GetCount() const
{
  return m_uiCount;
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE bool ezIdTableBase<IdType, ValueType>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename IdType, typename ValueType>
void ezIdTableBase<IdType, ValueType>::Clear()
{
  for (IndexType i = 0; i < m_uiCapacity; ++i)
  {
    Entry& entry = m_pEntries[i];

    if (entry.id.m_InstanceIndex == i)
    {
      ezMemoryUtils::Destruct(&entry.value, 1);
      ++entry.id.m_Generation;
    }

    entry.id.m_InstanceIndex = i + 1;
  }

  m_uiFreelistDequeue = 0;
  m_uiFreelistEnqueue = m_uiCapacity - 1;
  m_uiCount = 0;
}

template <typename IdType, typename ValueType>
IdType ezIdTableBase<IdType, ValueType>::Insert(const ValueType& value)
{
  Reserve(m_uiCount + 1);

  const IndexType uiNewIndex = m_uiFreelistDequeue;
  Entry& entry = m_pEntries[uiNewIndex];

  m_uiFreelistDequeue = entry.id.m_InstanceIndex;
  entry.id.m_InstanceIndex = uiNewIndex;
  ezMemoryUtils::CopyConstruct(&entry.value, value, 1);

  ++m_uiCount;

  return entry.id;
}

template <typename IdType, typename ValueType>
bool ezIdTableBase<IdType, ValueType>::Remove(const IdType id, ValueType* out_oldValue /*= nullptr*/)
{
  if (m_uiCapacity <= id.m_InstanceIndex)
    return false;

  const IndexType uiIndex = id.m_InstanceIndex;
  Entry& entry = m_pEntries[uiIndex];
  if (!entry.id.IsIndexAndGenerationEqual(id))
    return false;

  if (out_oldValue != nullptr)
    *out_oldValue = m_pEntries[uiIndex].value;

  ezMemoryUtils::Destruct(&entry.value, 1);

  entry.id.m_InstanceIndex = m_pEntries[m_uiFreelistEnqueue].id.m_InstanceIndex;
  ++entry.id.m_Generation;

  m_pEntries[m_uiFreelistEnqueue].id.m_InstanceIndex = uiIndex;
  m_uiFreelistEnqueue = uiIndex;

  --m_uiCount;
  return true;
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE bool ezIdTableBase<IdType, ValueType>::TryGetValue(const IdType id, ValueType& out_value) const
{
  const IndexType index = id.m_InstanceIndex;
  if (index < m_uiCapacity && m_pEntries[index].id.IsIndexAndGenerationEqual(id))
  {
    out_value = m_pEntries[index].value;
    return true;
  }
  return false;
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE bool ezIdTableBase<IdType, ValueType>::TryGetValue(const IdType id, ValueType*& out_pValue) const
{
  const IndexType index = id.m_InstanceIndex;
  if (index < m_uiCapacity && m_pEntries[index].id.IsIndexAndGenerationEqual(id))
  {
    out_pValue = &m_pEntries[index].value;
    return true;
  }
  return false;
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE const ValueType& ezIdTableBase<IdType, ValueType>::operator[](const IdType id) const
{
  EZ_ASSERT_DEV(id.m_InstanceIndex < m_uiCapacity, "Out of bounds access. Table has %i elements, trying to access element at index %i.", m_uiCapacity, id.m_InstanceIndex);
  Entry& entry = m_pEntries[id.m_InstanceIndex];
  EZ_ASSERT_DEV(entry.id.IsIndexAndGenerationEqual(id),
            "Stale access. Trying to access a value (generation: %i) that has been removed and replaced by a new value (generation: %i)", entry.id.m_Generation, id.m_Generation);

  return entry.value;
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE ValueType& ezIdTableBase<IdType, ValueType>::operator[](const IdType id)
{
  EZ_ASSERT_DEV(id.m_InstanceIndex < m_uiCapacity, "Out of bounds access. Table has %i elements, trying to access element at index %i.", m_uiCapacity, id.m_InstanceIndex);
  Entry& entry = m_pEntries[id.m_InstanceIndex];
  EZ_ASSERT_DEV(entry.id.IsIndexAndGenerationEqual(id),
            "Stale access. Trying to access a value (generation: %i) that has been removed and replaced by a new value (generation: %i)", entry.id.m_Generation, id.m_Generation);

  return entry.value;
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE const ValueType& ezIdTableBase<IdType, ValueType>::GetValueUnchecked(const IndexType index) const
{
  EZ_ASSERT_DEV(index < m_uiCapacity, "Out of bounds access. Table has %i elements, trying to access element at index %i.", m_uiCapacity, index);
  return m_pEntries[index].value;
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE ValueType& ezIdTableBase<IdType, ValueType>::GetValueUnchecked(const IndexType index)
{
  EZ_ASSERT_DEV(index < m_uiCapacity, "Out of bounds access. Table has %i elements, trying to access element at index %i.", m_uiCapacity, index);
  return m_pEntries[index].value;
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE bool ezIdTableBase<IdType, ValueType>::Contains(const IdType id) const
{
  const IndexType index = id.m_InstanceIndex;
  return index < m_uiCapacity && m_pEntries[index].id.IsIndexAndGenerationEqual(id);
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE typename ezIdTableBase<IdType, ValueType>::Iterator ezIdTableBase<IdType, ValueType>::GetIterator()
{
  return Iterator(*this);
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE typename ezIdTableBase<IdType, ValueType>::ConstIterator ezIdTableBase<IdType, ValueType>::GetIterator() const
{
  return ConstIterator(*this);
}

template <typename IdType, typename ValueType>
EZ_FORCE_INLINE ezAllocatorBase* ezIdTableBase<IdType, ValueType>::GetAllocator() const
{
  return m_pAllocator;
}

template <typename IdType, typename ValueType>
bool ezIdTableBase<IdType, ValueType>::IsFreelistValid() const
{
  if (m_pEntries == nullptr)
    return true;

  IndexType uiIndex = m_uiFreelistDequeue;
  const Entry* pEntry = m_pEntries + uiIndex;

  while (pEntry->id.m_InstanceIndex < m_uiCapacity)
  {
    uiIndex = pEntry->id.m_InstanceIndex;
    pEntry = m_pEntries + uiIndex;
  }

  return uiIndex == m_uiFreelistEnqueue;
}


// private methods
template <typename IdType, typename ValueType>
void ezIdTableBase<IdType, ValueType>::SetCapacity(IndexType uiCapacity)
{
  Entry* pNewEntries = EZ_NEW_RAW_BUFFER(m_pAllocator, Entry, (size_t) uiCapacity);

  for (IndexType i = 0; i < m_uiCapacity; ++i)
  {
    pNewEntries[i].id = m_pEntries[i].id;

    if (m_pEntries[i].id.m_InstanceIndex == i)
    {
      ezMemoryUtils::RelocateConstruct(&pNewEntries[i].value, &m_pEntries[i].value, 1);
    }
  }

  EZ_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
  m_pEntries = pNewEntries;

  InitializeFreelist(m_uiCapacity, uiCapacity);
  m_uiCapacity = uiCapacity;
}

template <typename IdType, typename ValueType>
inline void ezIdTableBase<IdType, ValueType>::InitializeFreelist(IndexType uiStart, IndexType uiEnd)
{
  for (IndexType i = uiStart; i < uiEnd; ++i)
  {
    IdType& id = m_pEntries[i].id;
    id = IdType(i + 1, 0);
  }

  m_uiFreelistEnqueue = uiEnd - 1;
}


template <typename IdType, typename V, typename A>
ezIdTable<IdType, V, A>::ezIdTable() : ezIdTableBase<IdType, V>(A::GetAllocator())
{
}

template <typename IdType, typename V, typename A>
ezIdTable<IdType, V, A>::ezIdTable(ezAllocatorBase* pAllocator) : ezIdTableBase<IdType, V>(pAllocator)
{
}

template <typename IdType, typename V, typename A>
ezIdTable<IdType, V, A>::ezIdTable(const ezIdTable<IdType, V, A>& other) : ezIdTableBase<IdType, V>(other, A::GetAllocator())
{
}

template <typename IdType, typename V, typename A>
ezIdTable<IdType, V, A>::ezIdTable(const ezIdTableBase<IdType, V>& other) : ezIdTableBase<IdType, V>(other, A::GetAllocator())
{
}

template <typename IdType, typename V, typename A>
void ezIdTable<IdType, V, A>::operator=(const ezIdTable<IdType, V, A>& rhs)
{
  ezIdTableBase<IdType, V>::operator=(rhs);
}

template <typename IdType, typename V, typename A>
void ezIdTable<IdType, V, A>::operator=(const ezIdTableBase<IdType, V>& rhs)
{
  ezIdTableBase<IdType, V>::operator=(rhs);
}

