#pragma once

template <typename KEY, typename VALUE>
inline ezArrayMapBase<KEY, VALUE>::ezArrayMapBase(ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_bSorted = true;
}

template <typename KEY, typename VALUE>
inline ezArrayMapBase<KEY, VALUE>::ezArrayMapBase(const ezArrayMapBase& rhs, ezAllocatorBase* pAllocator)
  : m_bSorted(rhs.m_bSorted)
  , m_Data(pAllocator)
{
  m_Data = rhs.m_Data;
}

template <typename KEY, typename VALUE>
inline void ezArrayMapBase<KEY, VALUE>::operator=(const ezArrayMapBase& rhs)
{
  m_bSorted = rhs.m_bSorted;
  m_Data = rhs.m_Data;
}

template <typename KEY, typename VALUE>
EZ_ALWAYS_INLINE ezUInt32 ezArrayMapBase<KEY, VALUE>::GetCount() const
{
  return m_Data.GetCount();
}

template <typename KEY, typename VALUE>
EZ_ALWAYS_INLINE bool ezArrayMapBase<KEY, VALUE>::IsEmpty() const
{
  return m_Data.IsEmpty();
}

template <typename KEY, typename VALUE>
inline void ezArrayMapBase<KEY, VALUE>::Clear()
{
  m_bSorted = true;
  m_Data.Clear();
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType, typename CompatibleValueType>
inline ezUInt32 ezArrayMapBase<KEY, VALUE>::Insert(CompatibleKeyType&& key, CompatibleValueType&& value)
{
  Pair& ref = m_Data.ExpandAndGetRef();
  ref.key = std::forward<CompatibleKeyType>(key);
  ref.value = std::forward<CompatibleValueType>(value);
  m_bSorted = false;
  return m_Data.GetCount() - 1;
}

template <typename KEY, typename VALUE>
inline void ezArrayMapBase<KEY, VALUE>::Sort() const
{
  if (m_bSorted)
    return;

  m_bSorted = true;
  m_Data.Sort();
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
ezUInt32 ezArrayMapBase<KEY, VALUE>::Find(const CompatibleKeyType& key) const
{
  if (!m_bSorted)
  {
    m_Data.Sort();
  }

  ezUInt32 lb = 0;
  ezUInt32 ub = m_Data.GetCount();

  while (lb < ub)
  {
    const ezUInt32 middle = lb + ((ub - lb) >> 1);

    if (m_Data[middle].key < key)
    {
      lb = middle + 1;
    }
    else if (key < m_Data[middle].key)
    {
      ub = middle;
    }
    else // equal
    {
      return middle;
    }
  }

  return ezInvalidIndex;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
ezUInt32 ezArrayMapBase<KEY, VALUE>::LowerBound(const CompatibleKeyType& key) const
{
  if (!m_bSorted)
  {
    m_Data.Sort();
  }

  ezUInt32 lb = 0;
  ezUInt32 ub = m_Data.GetCount();

  while (lb < ub)
  {
    const ezUInt32 middle = lb + ((ub - lb) >> 1);

    if (m_Data[middle].key < key)
    {
      lb = middle + 1;
    }
    else
    {
      ub = middle;
    }
  }

  if (lb == m_Data.GetCount())
    return ezInvalidIndex;

  return lb;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
ezUInt32 ezArrayMapBase<KEY, VALUE>::UpperBound(const CompatibleKeyType& key) const
{
  if (!m_bSorted)
  {
    m_Data.Sort();
  }

  ezUInt32 lb = 0;
  ezUInt32 ub = m_Data.GetCount();

  while (lb < ub)
  {
    const ezUInt32 middle = lb + ((ub - lb) >> 1);

    if (key < m_Data[middle].key)
    {
      ub = middle;
    }
    else
    {
      lb = middle + 1;
    }
  }

  if (ub == m_Data.GetCount())
    return ezInvalidIndex;

  return ub;
}

template <typename KEY, typename VALUE>
EZ_ALWAYS_INLINE const KEY& ezArrayMapBase<KEY, VALUE>::GetKey(ezUInt32 uiIndex) const
{
  return m_Data[uiIndex].key;
}

template <typename KEY, typename VALUE>
EZ_ALWAYS_INLINE const VALUE& ezArrayMapBase<KEY, VALUE>::GetValue(ezUInt32 uiIndex) const
{
  return m_Data[uiIndex].value;
}

template <typename KEY, typename VALUE>
VALUE& ezArrayMapBase<KEY, VALUE>::GetValue(ezUInt32 uiIndex)
{
  return m_Data[uiIndex].value;
}

template <typename KEY, typename VALUE>
EZ_ALWAYS_INLINE ezDynamicArray<typename ezArrayMapBase<KEY, VALUE>::Pair>& ezArrayMapBase<KEY, VALUE>::GetData()
{
  m_bSorted = false;
  return m_Data;
}

template <typename KEY, typename VALUE>
EZ_ALWAYS_INLINE const ezDynamicArray<typename ezArrayMapBase<KEY, VALUE>::Pair>& ezArrayMapBase<KEY, VALUE>::GetData() const
{
  return m_Data;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
VALUE& ezArrayMapBase<KEY, VALUE>::FindOrAdd(const CompatibleKeyType& key, bool* out_pExisted)
{
  ezUInt32 index = Find<CompatibleKeyType>(key);

  if (out_pExisted)
    *out_pExisted = index != ezInvalidIndex;

  if (index == ezInvalidIndex)
  {
    index = Insert(key, VALUE());
  }

  return GetValue(index);
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
EZ_ALWAYS_INLINE VALUE& ezArrayMapBase<KEY, VALUE>::operator[](const CompatibleKeyType& key)
{
  return FindOrAdd(key);
}

template <typename KEY, typename VALUE>
EZ_ALWAYS_INLINE const typename ezArrayMapBase<KEY, VALUE>::Pair& ezArrayMapBase<KEY, VALUE>::GetPair(ezUInt32 uiIndex) const
{
  return m_Data[uiIndex];
}

template <typename KEY, typename VALUE>
void ezArrayMapBase<KEY, VALUE>::RemoveAtAndCopy(ezUInt32 uiIndex, bool bKeepSorted)
{
  if (bKeepSorted && m_bSorted)
  {
    m_Data.RemoveAtAndCopy(uiIndex);
  }
  else
  {
    m_Data.RemoveAtAndSwap(uiIndex);
    m_bSorted = false;
  }
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
bool ezArrayMapBase<KEY, VALUE>::RemoveAndCopy(const CompatibleKeyType& key, bool bKeepSorted)
{
  const ezUInt32 uiIndex = Find(key);

  if (uiIndex == ezInvalidIndex)
    return false;

  RemoveAtAndCopy(uiIndex, bKeepSorted);
  return true;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
EZ_ALWAYS_INLINE bool ezArrayMapBase<KEY, VALUE>::Contains(const CompatibleKeyType& key) const
{
  return Find(key) != ezInvalidIndex;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
bool ezArrayMapBase<KEY, VALUE>::Contains(const CompatibleKeyType& key, const VALUE& value) const
{
  ezUInt32 atpos = LowerBound(key);

  if (atpos == ezInvalidIndex)
    return false;

  while (atpos < m_Data.GetCount())
  {
    if (m_Data[atpos].key != key)
      return false;

    if (m_Data[atpos].value == value)
      return true;

    ++atpos;
  }

  return false;
}


template <typename KEY, typename VALUE>
EZ_ALWAYS_INLINE void ezArrayMapBase<KEY, VALUE>::Reserve(ezUInt32 uiSize)
{
  m_Data.Reserve(uiSize);
}

template <typename KEY, typename VALUE>
EZ_ALWAYS_INLINE void ezArrayMapBase<KEY, VALUE>::Compact()
{
  m_Data.Compact();
}

template <typename KEY, typename VALUE>
bool ezArrayMapBase<KEY, VALUE>::operator==(const ezArrayMapBase<KEY, VALUE>& rhs) const
{
  Sort();
  rhs.Sort();

  return m_Data == rhs.m_Data;
}

template <typename KEY, typename VALUE, typename A>
ezArrayMap<KEY, VALUE, A>::ezArrayMap()
  : ezArrayMapBase<KEY, VALUE>(A::GetAllocator())
{
}

template <typename KEY, typename VALUE, typename A>
ezArrayMap<KEY, VALUE, A>::ezArrayMap(ezAllocatorBase* pAllocator)
  : ezArrayMapBase<KEY, VALUE>(pAllocator)
{
}

template <typename KEY, typename VALUE, typename A>
ezArrayMap<KEY, VALUE, A>::ezArrayMap(const ezArrayMap<KEY, VALUE, A>& rhs)
  : ezArrayMapBase<KEY, VALUE>(rhs, A::GetAllocator())
{
}

template <typename KEY, typename VALUE, typename A>
ezArrayMap<KEY, VALUE, A>::ezArrayMap(const ezArrayMapBase<KEY, VALUE>& rhs)
  : ezArrayMapBase<KEY, VALUE>(rhs, A::GetAllocator())
{
}

template <typename KEY, typename VALUE, typename A>
void ezArrayMap<KEY, VALUE, A>::operator=(const ezArrayMap<KEY, VALUE, A>& rhs)
{
  ezArrayMapBase<KEY, VALUE>::operator=(rhs);
}

template <typename KEY, typename VALUE, typename A>
void ezArrayMap<KEY, VALUE, A>::operator=(const ezArrayMapBase<KEY, VALUE>& rhs)
{
  ezArrayMapBase<KEY, VALUE>::operator=(rhs);
}
