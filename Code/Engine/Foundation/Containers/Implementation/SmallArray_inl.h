
template <typename T, ezUInt16 Size>
ezSmallArrayBase<T, Size>::ezSmallArrayBase() = default;

template <typename T, ezUInt16 Size>
ezSmallArrayBase<T, Size>::~ezSmallArrayBase() = default;

#if 0
template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::operator=(const ezArrayPtr<const T>& rhs)
{
  if (this->GetData() == rhs.GetPtr())
  {
    if (m_uiCount == rhs.GetCount())
      return;

    EZ_ASSERT_DEV(m_uiCount > rhs.GetCount(), "Dangling array pointer. The given array pointer points to invalid memory.");
    T* pElements = static_cast<Derived*>(this)->GetElementsPtr();
    ezMemoryUtils::Destruct(pElements + rhs.GetCount(), m_uiCount - rhs.GetCount());
    m_uiCount = rhs.GetCount();
    return;
  }

  const ezUInt32 uiOldCount = m_uiCount;
  const ezUInt32 uiNewCount = rhs.GetCount();

  if (uiNewCount > uiOldCount)
  {
    static_cast<Derived*>(this)->Reserve(uiNewCount);
    T* pElements = static_cast<Derived*>(this)->GetElementsPtr();
    ezMemoryUtils::Copy(pElements, rhs.GetPtr(), uiOldCount);
    ezMemoryUtils::CopyConstructArray(pElements + uiOldCount, rhs.GetPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else
  {
    T* pElements = static_cast<Derived*>(this)->GetElementsPtr();
    ezMemoryUtils::Copy(pElements, rhs.GetPtr(), uiNewCount);
    ezMemoryUtils::Destruct(pElements + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiNewCount;
}

#endif

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE ezSmallArrayBase<T, Size>::operator ezArrayPtr<const T>() const
{
  return ezArrayPtr<const T>(GetElementsPtr(), m_uiCount);
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE ezSmallArrayBase<T, Size>::operator ezArrayPtr<T>()
{
  return ezArrayPtr<T>(GetElementsPtr(), m_uiCount);
}

template <typename T, ezUInt16 Size>
bool ezSmallArrayBase<T, Size>::operator==(const ezArrayPtr<const T>& rhs) const
{
  if (m_uiCount != rhs.GetCount())
    return false;

  return ezMemoryUtils::IsEqual(GetElementsPtr(), rhs.GetPtr(), m_uiCount);
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE bool ezSmallArrayBase<T, Size>::operator!=(const ezArrayPtr<const T>& rhs) const
{
  return !(*this == rhs);
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE const T& ezSmallArrayBase<T, Size>::operator[](const ezUInt16 uiIndex) const
{
  EZ_ASSERT_DEV(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return GetElementsPtr()[uiIndex];
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE T& ezSmallArrayBase<T, Size>::operator[](const ezUInt16 uiIndex)
{
  EZ_ASSERT_DEV(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return GetElementsPtr()[uiIndex];
}

#if 0

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::SetCount(ezUInt32 uiCount)
{
  const ezUInt32 uiOldCount = m_uiCount;
  const ezUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    static_cast<Derived*>(this)->Reserve(uiNewCount);
    ezMemoryUtils::DefaultConstruct(static_cast<Derived*>(this)->GetElementsPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    ezMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::SetCount(ezUInt32 uiCount, const T& FillValue)
{
  const ezUInt32 uiOldCount = m_uiCount;
  const ezUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    static_cast<Derived*>(this)->Reserve(uiNewCount);
    ezMemoryUtils::CopyConstruct(static_cast<Derived*>(this)->GetElementsPtr() + uiOldCount, FillValue, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    ezMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::EnsureCount(ezUInt32 uiCount)
{
  if (uiCount > m_uiCount)
  {
    SetCount(uiCount);
  }
}

template <typename T, ezUInt16 Size>
template <typename> // Second template needed so that the compiler does only instantiate it when called. Otherwise the static_assert would trigger
                    // early.
                    void ezSmallArrayBase<T, Size>::SetCountUninitialized(ezUInt32 uiCount)
{
  static_assert(ezIsPodType<T>::value == ezTypeIsPod::value, "SetCountUninitialized is only supported for POD types.");
  const ezUInt32 uiOldCount = m_uiCount;
  const ezUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    static_cast<Derived*>(this)->Reserve(uiNewCount);
    ezMemoryUtils::Construct(static_cast<Derived*>(this)->GetElementsPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    ezMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

#endif

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE ezUInt32 ezSmallArrayBase<T, Size>::GetCount() const
{
  return m_uiCount;
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE bool ezSmallArrayBase<T, Size>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::Clear()
{
  ezMemoryUtils::Destruct(GetElementsPtr(), m_uiCount);
  m_uiCount = 0;
}

template <typename T, ezUInt16 Size>
bool ezSmallArrayBase<T, Size>::Contains(const T& value) const
{
  return IndexOf(value) != ezInvalidIndex;
}

#if 0

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::Insert(const T& value, ezUInt32 uiIndex)
{
  EZ_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  ezMemoryUtils::Prepend(static_cast<Derived*>(this)->GetElementsPtr() + uiIndex, value, m_uiCount - uiIndex);
  m_uiCount++;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::Insert(T&& value, ezUInt32 uiIndex)
{
  EZ_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  ezMemoryUtils::Prepend(static_cast<Derived*>(this)->GetElementsPtr() + uiIndex, std::move(value), m_uiCount - uiIndex);
  m_uiCount++;
}

template <typename T, ezUInt16 Size>
bool ezSmallArrayBase<T, Size>::RemoveAndCopy(const T& value)
{
  ezUInt32 uiIndex = IndexOf(value);

  if (uiIndex == ezInvalidIndex)
    return false;

  RemoveAtAndCopy(uiIndex);
  return true;
}

template <typename T, ezUInt16 Size>
bool ezSmallArrayBase<T, Size>::RemoveAndSwap(const T& value)
{
  ezUInt32 uiIndex = IndexOf(value);

  if (uiIndex == ezInvalidIndex)
    return false;

  RemoveAtAndSwap(uiIndex);
  return true;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::RemoveAtAndCopy(ezUInt32 uiIndex, ezUInt32 uiNumElements /*= 1*/)
{
  EZ_ASSERT_DEV(uiIndex + uiNumElements <= m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.",
    m_uiCount, uiIndex + uiNumElements - 1);

  T* pElements = static_cast<Derived*>(this)->GetElementsPtr();

  m_uiCount -= uiNumElements;
  ezMemoryUtils::RelocateOverlapped(pElements + uiIndex, pElements + uiIndex + uiNumElements, m_uiCount - uiIndex);
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::RemoveAtAndSwap(ezUInt32 uiIndex, ezUInt32 uiNumElements /*= 1*/)
{
  EZ_ASSERT_DEV(uiIndex + uiNumElements <= m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.",
    m_uiCount, uiIndex + uiNumElements - 1);

  T* pElements = static_cast<Derived*>(this)->GetElementsPtr();

  for (ezUInt32 i = 0; i < uiNumElements; ++i)
  {
    m_uiCount--;

    if (m_uiCount != uiIndex)
    {
      pElements[uiIndex] = std::move(pElements[m_uiCount]);
    }
    ezMemoryUtils::Destruct(pElements + m_uiCount, 1);
    ++uiIndex;
  }
}

#endif

template <typename T, ezUInt16 Size>
ezUInt32 ezSmallArrayBase<T, Size>::IndexOf(const T& value, ezUInt16 uiStartIndex) const
{
  const T* pElements = GetElementsPtr();

  for (ezUInt32 i = uiStartIndex; i < m_uiCount; i++)
  {
    if (ezMemoryUtils::IsEqual(pElements + i, &value))
      return i;
  }
  return ezInvalidIndex;
}

template <typename T, ezUInt16 Size>
ezUInt32 ezSmallArrayBase<T, Size>::LastIndexOf(const T& value, ezUInt16 uiStartIndex) const
{
  const T* pElements = GetElementsPtr();

  for (ezUInt32 i = ezMath::Min(uiStartIndex, m_uiCount); i-- > 0;)
  {
    if (ezMemoryUtils::IsEqual(pElements + i, &value))
      return i;
  }
  return ezInvalidIndex;
}

template <typename T, ezUInt16 Size>
T& ezSmallArrayBase<T, Size>::ExpandAndGetRef()
{
  Reserve(m_uiCount + 1);

  T* pElements = GetElementsPtr();

  ezMemoryUtils::Construct(pElements + m_uiCount, 1);

  T& ReturnRef = *(pElements + m_uiCount);

  m_uiCount++;

  return ReturnRef;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::PushBack(const T& value)
{
  Reserve(m_uiCount + 1);

  ezMemoryUtils::CopyConstruct(GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::PushBack(T&& value)
{
  Reserve(m_uiCount + 1);

  ezMemoryUtils::MoveConstruct<T>(GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::PushBackUnchecked(const T& value)
{
  EZ_ASSERT_DEV(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  ezMemoryUtils::CopyConstruct(GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::PushBackUnchecked(T&& value)
{
  EZ_ASSERT_DEV(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  ezMemoryUtils::MoveConstruct<T>(GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::PushBackRange(const ezArrayPtr<const T>& range)
{
  const ezUInt32 uiRangeCount = range.GetCount();
  Reserve(m_uiCount + uiRangeCount);

  ezMemoryUtils::CopyConstructArray(GetElementsPtr() + m_uiCount, range.GetPtr(), uiRangeCount);
  m_uiCount += uiRangeCount;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::PopBack(ezUInt32 uiCountToRemove /* = 1 */)
{
  EZ_ASSERT_DEV(m_uiCount >= uiCountToRemove, "Out of bounds access. Array has {0} elements, trying to pop {1} elements.", m_uiCount, uiCountToRemove);

  m_uiCount -= uiCountToRemove;
  ezMemoryUtils::Destruct(GetElementsPtr() + m_uiCount, uiCountToRemove);
}

template <typename T, ezUInt16 Size>
EZ_FORCE_INLINE T& ezSmallArrayBase<T, Size>::PeekBack()
{
  EZ_ASSERT_DEV(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");
  return GetElementsPtr()[m_uiCount - 1];
}

template <typename T, ezUInt16 Size>
EZ_FORCE_INLINE const T& ezSmallArrayBase<T, Size>::PeekBack() const
{
  EZ_ASSERT_DEV(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");
  return GetElementsPtr()[m_uiCount - 1];
}

template <typename T, ezUInt16 Size>
template <typename Comparer>
void ezSmallArrayBase<T, Size>::Sort(const Comparer& comparer)
{
  if (m_uiCount > 1)
  {
    ezArrayPtr<T> ar = *this;
    ezSorting::QuickSort(ar, comparer);
  }
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::Sort()
{
  if (m_uiCount > 1)
  {
    ezArrayPtr<T> ar = *this;
    ezSorting::QuickSort(ar, ezCompareHelper<T>());
  }
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE T* ezSmallArrayBase<T, Size>::GetData()
{
  if (IsEmpty())
    return nullptr;

  return GetElementsPtr();
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE const T* ezSmallArrayBase<T, Size>::GetData() const
{
  if (IsEmpty())
    return nullptr;

  return GetElementsPtr();
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE ezArrayPtr<T> ezSmallArrayBase<T, Size>::GetArrayPtr()
{
  return ezArrayPtr<T>(GetData(), GetCount());
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE ezArrayPtr<const T> ezSmallArrayBase<T, Size>::GetArrayPtr() const
{
  return ezArrayPtr<const T>(GetData(), GetCount());
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE ezArrayPtr<typename ezArrayPtr<T>::ByteType> ezSmallArrayBase<T, Size>::GetByteArrayPtr()
{
  return GetArrayPtr().ToByteArray();
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE ezArrayPtr<typename ezArrayPtr<const T>::ByteType> ezSmallArrayBase<T, Size>::GetByteArrayPtr() const
{
  return GetArrayPtr().ToByteArray();
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE T* ezSmallArrayBase<T, Size>::GetElementsPtr()
{
  return m_uiCapacity <= Size ? reinterpret_cast<T*>(m_StaticData) : m_pElements;
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE const T* ezSmallArrayBase<T, Size>::GetElementsPtr() const
{
  return m_uiCapacity <= Size ? reinterpret_cast<const T*>(m_StaticData) : m_pElements;
}

//////////////////////////////////////////////////////////////////////////

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
ezSmallArray<T, Size, AllocatorWrapper>::ezSmallArray() = default;

#if 0
template <typename T, typename A>
ezDynamicArray<T, A>::ezDynamicArray(const ezDynamicArray<T, A>& other)
  : ezDynamicArrayBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
ezDynamicArray<T, A>::ezDynamicArray(const ezDynamicArrayBase<T>& other)
  : ezDynamicArrayBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
ezDynamicArray<T, A>::ezDynamicArray(const ezArrayPtr<const T>& other)
  : ezDynamicArrayBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
ezDynamicArray<T, A>::ezDynamicArray(ezDynamicArray<T, A>&& other)
  : ezDynamicArrayBase<T>(std::move(other), other.GetAllocator())
{
}

template <typename T, typename A>
ezDynamicArray<T, A>::ezDynamicArray(ezDynamicArrayBase<T>&& other)
  : ezDynamicArrayBase<T>(std::move(other), other.GetAllocator())
{
}

template <typename T, typename A>
void ezDynamicArray<T, A>::operator=(const ezDynamicArray<T, A>& rhs)
{
  ezDynamicArrayBase<T>::operator=(rhs);
}

template <typename T, typename A>
void ezDynamicArray<T, A>::operator=(const ezDynamicArrayBase<T>& rhs)
{
  ezDynamicArrayBase<T>::operator=(rhs);
}

template <typename T, typename A>
void ezDynamicArray<T, A>::operator=(const ezArrayPtr<const T>& rhs)
{
  ezArrayBase<T, ezDynamicArrayBase<T>>::operator=(rhs);
}

template <typename T, typename A>
void ezDynamicArray<T, A>::operator=(ezDynamicArray<T, A>&& rhs) noexcept
{
  ezDynamicArrayBase<T>::operator=(std::move(rhs));
}

template <typename T, typename A>
void ezDynamicArray<T, A>::operator=(ezDynamicArrayBase<T>&& rhs) noexcept
{
  ezDynamicArrayBase<T>::operator=(std::move(rhs));
}
#endif
