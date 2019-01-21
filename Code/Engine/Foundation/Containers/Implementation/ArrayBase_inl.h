
template <typename T, typename Derived>
ezArrayBase<T, Derived>::ezArrayBase()
{
  m_pElements = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
}

template <typename T, typename Derived>
ezArrayBase<T, Derived>::~ezArrayBase()
{
  EZ_ASSERT_DEBUG(m_uiCount == 0, "The derived class did not destruct all objects. Count is {0}.", m_uiCount);
  EZ_ASSERT_DEBUG(m_pElements == nullptr, "The derived class did not free its memory.");
}

template <typename T, typename Derived>
void ezArrayBase<T, Derived>::operator=(const ezArrayPtr<const T>& rhs)
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

template <typename T, typename Derived>
EZ_ALWAYS_INLINE ezArrayBase<T, Derived>::operator ezArrayPtr<const T>() const
{
  return ezArrayPtr<const T>(static_cast<const Derived*>(this)->GetElementsPtr(), m_uiCount);
}

template <typename T, typename Derived>
EZ_ALWAYS_INLINE ezArrayBase<T, Derived>::operator ezArrayPtr<T>()
{
  return ezArrayPtr<T>(static_cast<Derived*>(this)->GetElementsPtr(), m_uiCount);
}

template <typename T, typename Derived>
bool ezArrayBase<T, Derived>::operator==(const ezArrayPtr<const T>& rhs) const
{
  if (m_uiCount != rhs.GetCount())
    return false;

  return ezMemoryUtils::IsEqual(static_cast<const Derived*>(this)->GetElementsPtr(), rhs.GetPtr(), m_uiCount);
}

template <typename T, typename Derived>
EZ_ALWAYS_INLINE bool ezArrayBase<T, Derived>::operator!=(const ezArrayPtr<const T>& rhs) const
{
  return !(*this == rhs);
}

template <typename T, typename Derived>
EZ_ALWAYS_INLINE const T& ezArrayBase<T, Derived>::operator[](const ezUInt32 uiIndex) const
{
  EZ_ASSERT_DEV(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount,
                uiIndex);
  return static_cast<const Derived*>(this)->GetElementsPtr()[uiIndex];
}

template <typename T, typename Derived>
EZ_ALWAYS_INLINE T& ezArrayBase<T, Derived>::operator[](const ezUInt32 uiIndex)
{
  EZ_ASSERT_DEV(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount,
                uiIndex);
  return static_cast<Derived*>(this)->GetElementsPtr()[uiIndex];
}

template <typename T, typename Derived>
void ezArrayBase<T, Derived>::SetCount(ezUInt32 uiCount)
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

template <typename T, typename Derived>
void ezArrayBase<T, Derived>::SetCount(ezUInt32 uiCount, const T& FillValue)
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

template <typename T, typename Derived>
void ezArrayBase<T, Derived>::EnsureCount(ezUInt32 uiCount)
{
  if (uiCount > m_uiCount)
  {
    SetCount(uiCount);
  }
}

template <typename T, typename Derived>
template <typename> // second template needed so that the compiler does only instantiate it when called. Otherwise the static_assert would
                    // trigger early.
                    void ezArrayBase<T, Derived>::SetCountUninitialized(ezUInt32 uiCount)
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

template <typename T, typename Derived>
EZ_ALWAYS_INLINE ezUInt32 ezArrayBase<T, Derived>::GetCount() const
{
  return m_uiCount;
}

template <typename T, typename Derived>
EZ_ALWAYS_INLINE bool ezArrayBase<T, Derived>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, typename Derived>
void ezArrayBase<T, Derived>::Clear()
{
  ezMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr(), m_uiCount);
  m_uiCount = 0;
}

template <typename T, typename Derived>
bool ezArrayBase<T, Derived>::Contains(const T& value) const
{
  return IndexOf(value) != ezInvalidIndex;
}

template <typename T, typename Derived>
void ezArrayBase<T, Derived>::Insert(const T& value, ezUInt32 uiIndex)
{
  EZ_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  ezMemoryUtils::Prepend(static_cast<Derived*>(this)->GetElementsPtr() + uiIndex, value, m_uiCount - uiIndex);
  m_uiCount++;
}

template <typename T, typename Derived>
void ezArrayBase<T, Derived>::Insert(T&& value, ezUInt32 uiIndex)
{
  EZ_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  ezMemoryUtils::Prepend(static_cast<Derived*>(this)->GetElementsPtr() + uiIndex, std::move(value), m_uiCount - uiIndex);
  m_uiCount++;
}

template <typename T, typename Derived>
bool ezArrayBase<T, Derived>::RemoveAndCopy(const T& value)
{
  ezUInt32 uiIndex = IndexOf(value);

  if (uiIndex == ezInvalidIndex)
    return false;

  RemoveAtAndCopy(uiIndex);
  return true;
}

template <typename T, typename Derived>
bool ezArrayBase<T, Derived>::RemoveAndSwap(const T& value)
{
  ezUInt32 uiIndex = IndexOf(value);

  if (uiIndex == ezInvalidIndex)
    return false;

  RemoveAtAndSwap(uiIndex);
  return true;
}

template <typename T, typename Derived>
void ezArrayBase<T, Derived>::RemoveAtAndCopy(ezUInt32 uiIndex)
{
  EZ_ASSERT_DEV(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount,
                uiIndex);

  T* pElements = static_cast<Derived*>(this)->GetElementsPtr();

  m_uiCount--;
  ezMemoryUtils::RelocateOverlapped(pElements + uiIndex, pElements + uiIndex + 1, m_uiCount - uiIndex);
}

template <typename T, typename Derived>
void ezArrayBase<T, Derived>::RemoveAtAndSwap(ezUInt32 uiIndex)
{
  EZ_ASSERT_DEV(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount,
                uiIndex);

  T* pElements = static_cast<Derived*>(this)->GetElementsPtr();

  m_uiCount--;
  if (m_uiCount != uiIndex)
  {
    pElements[uiIndex] = std::move(pElements[m_uiCount]);
  }
  ezMemoryUtils::Destruct(pElements + m_uiCount, 1);
}

template <typename T, typename Derived>
ezUInt32 ezArrayBase<T, Derived>::IndexOf(const T& value, ezUInt32 uiStartIndex) const
{
  const T* pElements = static_cast<const Derived*>(this)->GetElementsPtr();

  for (ezUInt32 i = uiStartIndex; i < m_uiCount; i++)
  {
    if (ezMemoryUtils::IsEqual(pElements + i, &value))
      return i;
  }
  return ezInvalidIndex;
}

template <typename T, typename Derived>
ezUInt32 ezArrayBase<T, Derived>::LastIndexOf(const T& value, ezUInt32 uiStartIndex) const
{
  const T* pElements = static_cast<const Derived*>(this)->GetElementsPtr();

  for (ezUInt32 i = ezMath::Min(uiStartIndex, m_uiCount); i-- > 0;)
  {
    if (ezMemoryUtils::IsEqual(pElements + i, &value))
      return i;
  }
  return ezInvalidIndex;
}

template <typename T, typename Derived>
T& ezArrayBase<T, Derived>::ExpandAndGetRef()
{
  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  T* pElements = static_cast<Derived*>(this)->GetElementsPtr();

  ezMemoryUtils::Construct(pElements + m_uiCount, 1);

  T& ReturnRef = *(pElements + m_uiCount);

  m_uiCount++;

  return ReturnRef;
}

template <typename T, typename Derived>
void ezArrayBase<T, Derived>::PushBack(const T& value)
{
  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  ezMemoryUtils::CopyConstruct(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, typename Derived>
void ezArrayBase<T, Derived>::PushBack(T&& value)
{
  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  ezMemoryUtils::MoveConstruct<T>(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, typename Derived>
void ezArrayBase<T, Derived>::PushBackUnchecked(const T& value)
{
  EZ_ASSERT_DEV(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  ezMemoryUtils::CopyConstruct(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, typename Derived>
void ezArrayBase<T, Derived>::PushBackUnchecked(T&& value)
{
  EZ_ASSERT_DEV(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  ezMemoryUtils::MoveConstruct<T>(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, typename Derived>
void ezArrayBase<T, Derived>::PushBackRange(const ezArrayPtr<const T>& range)
{
  const ezUInt32 uiRangeCount = range.GetCount();
  static_cast<Derived*>(this)->Reserve(m_uiCount + uiRangeCount);

  ezMemoryUtils::CopyConstructArray(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, range.GetPtr(), uiRangeCount);
  m_uiCount += uiRangeCount;
}

template <typename T, typename Derived>
void ezArrayBase<T, Derived>::PopBack(ezUInt32 uiCountToRemove /* = 1 */)
{
  EZ_ASSERT_DEV(m_uiCount >= uiCountToRemove, "Out of bounds access. Array has {0} elements, trying to pop {1} elements.", m_uiCount,
                uiCountToRemove);

  m_uiCount -= uiCountToRemove;
  ezMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, uiCountToRemove);
}

template <typename T, typename Derived>
EZ_FORCE_INLINE T& ezArrayBase<T, Derived>::PeekBack()
{
  EZ_ASSERT_DEV(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");
  return static_cast<Derived*>(this)->GetElementsPtr()[m_uiCount - 1];
}

template <typename T, typename Derived>
EZ_FORCE_INLINE const T& ezArrayBase<T, Derived>::PeekBack() const
{
  EZ_ASSERT_DEV(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");
  return static_cast<const Derived*>(this)->GetElementsPtr()[m_uiCount - 1];
}

template <typename T, typename Derived>
template <typename Comparer>
void ezArrayBase<T, Derived>::Sort(const Comparer& comparer)
{
  if (m_uiCount > 1)
  {
    ezArrayPtr<T> ar = *this;
    ezSorting::QuickSort(ar, comparer);
  }
}

template <typename T, typename Derived>
void ezArrayBase<T, Derived>::Sort()
{
  if (m_uiCount > 1)
  {
    ezArrayPtr<T> ar = *this;
    ezSorting::QuickSort(ar, ezCompareHelper<T>());
  }
}

template <typename T, typename Derived>
EZ_ALWAYS_INLINE T* ezArrayBase<T, Derived>::GetData()
{
  if (IsEmpty())
    return nullptr;

  return static_cast<Derived*>(this)->GetElementsPtr();
}

template <typename T, typename Derived>
EZ_ALWAYS_INLINE const T* ezArrayBase<T, Derived>::GetData() const
{
  if (IsEmpty())
    return nullptr;

  return static_cast<const Derived*>(this)->GetElementsPtr();
}

template <typename T, typename Derived>
EZ_ALWAYS_INLINE ezArrayPtr<T> ezArrayBase<T, Derived>::GetArrayPtr()
{
  return ezArrayPtr<T>(GetData(), GetCount());
}

template <typename T, typename Derived>
EZ_ALWAYS_INLINE ezArrayPtr<const T> ezArrayBase<T, Derived>::GetArrayPtr() const
{
  return ezArrayPtr<const T>(GetData(), GetCount());
}

template <typename T, typename Derived>
EZ_ALWAYS_INLINE ezArrayPtr<typename ezArrayPtr<T>::ByteType> ezArrayBase<T, Derived>::GetByteArrayPtr()
{
  return GetArrayPtr().ToByteArray();
}

template <typename T, typename Derived>
EZ_ALWAYS_INLINE ezArrayPtr<typename ezArrayPtr<const T>::ByteType> ezArrayBase<T, Derived>::GetByteArrayPtr() const
{
  return GetArrayPtr().ToByteArray();
}

template <typename T, typename Derived>
void ezArrayBase<T, Derived>::DoSwap(ezArrayBase<T, Derived>& other)
{
  ezMath::Swap(this->m_pElements, other.m_pElements);
  ezMath::Swap(this->m_uiCapacity, other.m_uiCapacity);
  ezMath::Swap(this->m_uiCount, other.m_uiCount);
}
