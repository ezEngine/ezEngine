
template <typename T, ezUInt16 Size>
ezSmallArrayBase<T, Size>::ezSmallArrayBase() = default;

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE ezSmallArrayBase<T, Size>::ezSmallArrayBase(const ezSmallArrayBase<T, Size>& other, ezAllocator* pAllocator)
{
  CopyFrom((ezArrayPtr<const T>)other, pAllocator);
  m_uiUserData = other.m_uiUserData;
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE ezSmallArrayBase<T, Size>::ezSmallArrayBase(const ezArrayPtr<const T>& other, ezAllocator* pAllocator)
{
  CopyFrom(other, pAllocator);
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE ezSmallArrayBase<T, Size>::ezSmallArrayBase(ezSmallArrayBase<T, Size>&& other, ezAllocator* pAllocator)
{
  MoveFrom(std::move(other), pAllocator);
}

template <typename T, ezUInt16 Size>
EZ_FORCE_INLINE ezSmallArrayBase<T, Size>::~ezSmallArrayBase()
{
  EZ_ASSERT_DEBUG(m_uiCount == 0, "The derived class did not destruct all objects. Count is {0}.", m_uiCount);
  EZ_ASSERT_DEBUG(m_pElements == nullptr, "The derived class did not free its memory.");
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::CopyFrom(const ezArrayPtr<const T>& other, ezAllocator* pAllocator)
{
  EZ_ASSERT_DEV(other.GetCount() <= ezSmallInvalidIndex, "Can't copy {} elements to small array. Maximum count is {}", other.GetCount(), ezSmallInvalidIndex);

  if (GetData() == other.GetPtr())
  {
    if (m_uiCount == other.GetCount())
      return;

    EZ_ASSERT_DEV(m_uiCount > other.GetCount(), "Dangling array pointer. The given array pointer points to invalid memory.");
    T* pElements = GetElementsPtr();
    ezMemoryUtils::Destruct(pElements + other.GetCount(), m_uiCount - other.GetCount());
    m_uiCount = static_cast<ezUInt16>(other.GetCount());
    return;
  }

  const ezUInt32 uiOldCount = m_uiCount;
  const ezUInt32 uiNewCount = other.GetCount();

  if (uiNewCount > uiOldCount)
  {
    Reserve(static_cast<ezUInt16>(uiNewCount), pAllocator);
    T* pElements = GetElementsPtr();
    ezMemoryUtils::Copy(pElements, other.GetPtr(), uiOldCount);
    ezMemoryUtils::CopyConstructArray(pElements + uiOldCount, other.GetPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else
  {
    T* pElements = GetElementsPtr();
    ezMemoryUtils::Copy(pElements, other.GetPtr(), uiNewCount);
    ezMemoryUtils::Destruct(pElements + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = static_cast<ezUInt16>(uiNewCount);
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::MoveFrom(ezSmallArrayBase<T, Size>&& other, ezAllocator* pAllocator)
{
  Clear();

  if (other.m_uiCapacity > Size)
  {
    if (m_uiCapacity > Size)
    {
      // only delete our own external storage
      EZ_DELETE_RAW_BUFFER(pAllocator, m_pElements);
    }

    m_uiCapacity = other.m_uiCapacity;
    m_pElements = other.m_pElements;
  }
  else
  {
    ezMemoryUtils::RelocateConstruct(GetElementsPtr(), other.GetElementsPtr(), other.m_uiCount);
  }

  m_uiCount = other.m_uiCount;
  m_uiUserData = other.m_uiUserData;

  // reset the other array to not reference the data anymore
  other.m_pElements = nullptr;
  other.m_uiCount = 0;
  other.m_uiCapacity = 0;
}

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
EZ_ALWAYS_INLINE bool ezSmallArrayBase<T, Size>::operator==(const ezSmallArrayBase<T, Size>& rhs) const
{
  return *this == rhs.GetArrayPtr();
}

#if EZ_DISABLED(EZ_USE_CPP20_OPERATORS)
template <typename T, ezUInt16 Size>
bool ezSmallArrayBase<T, Size>::operator==(const ezArrayPtr<const T>& rhs) const
{
  if (m_uiCount != rhs.GetCount())
    return false;

  return ezMemoryUtils::IsEqual(GetElementsPtr(), rhs.GetPtr(), m_uiCount);
}
#endif

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE bool ezSmallArrayBase<T, Size>::operator<(const ezSmallArrayBase<T, Size>& rhs) const
{
  return GetArrayPtr() < rhs.GetArrayPtr();
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE bool ezSmallArrayBase<T, Size>::operator<(const ezArrayPtr<const T>& rhs) const
{
  return GetArrayPtr() < rhs;
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE const T& ezSmallArrayBase<T, Size>::operator[](const ezUInt32 uiIndex) const
{
  EZ_ASSERT_DEBUG(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return GetElementsPtr()[uiIndex];
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE T& ezSmallArrayBase<T, Size>::operator[](const ezUInt32 uiIndex)
{
  EZ_ASSERT_DEBUG(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return GetElementsPtr()[uiIndex];
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::SetCount(ezUInt16 uiCount, ezAllocator* pAllocator)
{
  const ezUInt32 uiOldCount = m_uiCount;
  const ezUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    Reserve(static_cast<ezUInt16>(uiNewCount), pAllocator);
    ezMemoryUtils::Construct<ConstructAll>(GetElementsPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    ezMemoryUtils::Destruct(GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::SetCount(ezUInt16 uiCount, const T& fillValue, ezAllocator* pAllocator)
{
  const ezUInt32 uiOldCount = m_uiCount;
  const ezUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    Reserve(uiCount, pAllocator);
    ezMemoryUtils::CopyConstruct(GetElementsPtr() + uiOldCount, fillValue, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    ezMemoryUtils::Destruct(GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::EnsureCount(ezUInt16 uiCount, ezAllocator* pAllocator)
{
  if (uiCount > m_uiCount)
  {
    SetCount(uiCount, pAllocator);
  }
}

template <typename T, ezUInt16 Size>
template <typename> // Second template needed so that the compiler does only instantiate it when called. Otherwise the static_assert would trigger early.
void ezSmallArrayBase<T, Size>::SetCountUninitialized(ezUInt16 uiCount, ezAllocator* pAllocator)
{
  static_assert(ezIsPodType<T>::value == ezTypeIsPod::value, "SetCountUninitialized is only supported for POD types. See EZ_DEFINE_AS_POD_TYPE() and EZ_DECLARE_POD_TYPE().");
  const ezUInt16 uiOldCount = m_uiCount;
  const ezUInt16 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    Reserve(uiNewCount, pAllocator);
    ezMemoryUtils::Construct<SkipTrivialTypes>(GetElementsPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    ezMemoryUtils::Destruct(GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

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

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::Insert(const T& value, ezUInt32 uiIndex, ezAllocator* pAllocator)
{
  EZ_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  Reserve(m_uiCount + 1, pAllocator);

  ezMemoryUtils::Prepend(GetElementsPtr() + uiIndex, value, m_uiCount - uiIndex);
  m_uiCount++;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::Insert(T&& value, ezUInt32 uiIndex, ezAllocator* pAllocator)
{
  EZ_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  Reserve(m_uiCount + 1, pAllocator);

  ezMemoryUtils::Prepend(GetElementsPtr() + uiIndex, std::move(value), m_uiCount - uiIndex);
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
void ezSmallArrayBase<T, Size>::RemoveAtAndCopy(ezUInt32 uiIndex, ezUInt16 uiNumElements /*= 1*/)
{
  EZ_ASSERT_DEV(uiIndex + uiNumElements <= m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount, uiIndex + uiNumElements - 1);

  T* pElements = GetElementsPtr();

  m_uiCount -= uiNumElements;
  ezMemoryUtils::RelocateOverlapped(pElements + uiIndex, pElements + uiIndex + uiNumElements, m_uiCount - uiIndex);
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::RemoveAtAndSwap(ezUInt32 uiIndex, ezUInt16 uiNumElements /*= 1*/)
{
  EZ_ASSERT_DEV(uiIndex + uiNumElements <= m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount, uiIndex + uiNumElements - 1);

  T* pElements = GetElementsPtr();

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

template <typename T, ezUInt16 Size>
ezUInt32 ezSmallArrayBase<T, Size>::IndexOf(const T& value, ezUInt32 uiStartIndex) const
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
ezUInt32 ezSmallArrayBase<T, Size>::LastIndexOf(const T& value, ezUInt32 uiStartIndex) const
{
  const T* pElements = GetElementsPtr();

  for (ezUInt32 i = ezMath::Min<ezUInt32>(uiStartIndex, m_uiCount); i-- > 0;)
  {
    if (ezMemoryUtils::IsEqual(pElements + i, &value))
      return i;
  }
  return ezInvalidIndex;
}

template <typename T, ezUInt16 Size>
T& ezSmallArrayBase<T, Size>::ExpandAndGetRef(ezAllocator* pAllocator)
{
  Reserve(m_uiCount + 1, pAllocator);

  T* pElements = GetElementsPtr();

  ezMemoryUtils::Construct<SkipTrivialTypes>(pElements + m_uiCount, 1);

  T& ReturnRef = *(pElements + m_uiCount);

  m_uiCount++;

  return ReturnRef;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::PushBack(const T& value, ezAllocator* pAllocator)
{
  Reserve(m_uiCount + 1, pAllocator);

  ezMemoryUtils::CopyConstruct(GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::PushBack(T&& value, ezAllocator* pAllocator)
{
  Reserve(m_uiCount + 1, pAllocator);

  ezMemoryUtils::MoveConstruct<T>(GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::PushBackUnchecked(const T& value)
{
  EZ_ASSERT_DEBUG(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  ezMemoryUtils::CopyConstruct(GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::PushBackUnchecked(T&& value)
{
  EZ_ASSERT_DEBUG(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  ezMemoryUtils::MoveConstruct<T>(GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::PushBackRange(const ezArrayPtr<const T>& range, ezAllocator* pAllocator)
{
  const ezUInt32 uiRangeCount = range.GetCount();
  Reserve(m_uiCount + uiRangeCount, pAllocator);

  ezMemoryUtils::CopyConstructArray(GetElementsPtr() + m_uiCount, range.GetPtr(), uiRangeCount);
  m_uiCount += uiRangeCount;
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::PopBack(ezUInt32 uiCountToRemove /* = 1 */)
{
  EZ_ASSERT_DEBUG(m_uiCount >= uiCountToRemove, "Out of bounds access. Array has {0} elements, trying to pop {1} elements.", m_uiCount, uiCountToRemove);

  m_uiCount -= uiCountToRemove;
  ezMemoryUtils::Destruct(GetElementsPtr() + m_uiCount, uiCountToRemove);
}

template <typename T, ezUInt16 Size>
EZ_FORCE_INLINE T& ezSmallArrayBase<T, Size>::PeekBack()
{
  EZ_ASSERT_DEBUG(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");
  return GetElementsPtr()[m_uiCount - 1];
}

template <typename T, ezUInt16 Size>
EZ_FORCE_INLINE const T& ezSmallArrayBase<T, Size>::PeekBack() const
{
  EZ_ASSERT_DEBUG(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");
  return GetElementsPtr()[m_uiCount - 1];
}

template <typename T, ezUInt16 Size>
template <typename Comparer>
void ezSmallArrayBase<T, Size>::Sort(const Comparer& comparer)
{
  if (m_uiCount > 1)
  {
    ezArrayPtr<T> ar = GetArrayPtr();
    ezSorting::QuickSort(ar, comparer);
  }
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::Sort()
{
  if (m_uiCount > 1)
  {
    ezArrayPtr<T> ar = GetArrayPtr();
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
void ezSmallArrayBase<T, Size>::Reserve(ezUInt16 uiCapacity, ezAllocator* pAllocator)
{
  if (m_uiCapacity >= uiCapacity)
    return;

  const ezUInt32 uiCurCap = static_cast<ezUInt32>(m_uiCapacity);
  ezUInt32 uiNewCapacity = uiCurCap + (uiCurCap / 2);

  uiNewCapacity = ezMath::Max<ezUInt32>(uiNewCapacity, uiCapacity);
  uiNewCapacity = ezMemoryUtils::AlignSize<ezUInt32>(uiNewCapacity, CAPACITY_ALIGNMENT);
  uiNewCapacity = ezMath::Min<ezUInt32>(uiNewCapacity, 0xFFFFu);

  SetCapacity(static_cast<ezUInt16>(uiNewCapacity), pAllocator);
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::Compact(ezAllocator* pAllocator)
{
  if (IsEmpty())
  {
    if (m_uiCapacity > Size)
    {
      // completely deallocate all data, if the array is empty.
      EZ_DELETE_RAW_BUFFER(pAllocator, m_pElements);
    }

    m_uiCapacity = Size;
    m_pElements = nullptr;
  }
  else if (m_uiCapacity > Size)
  {
    ezUInt32 uiNewCapacity = ezMemoryUtils::AlignSize<ezUInt32>(m_uiCount, CAPACITY_ALIGNMENT);
    uiNewCapacity = ezMath::Min<ezUInt32>(uiNewCapacity, 0xFFFFu);

    if (m_uiCapacity != uiNewCapacity)
      SetCapacity(static_cast<ezUInt16>(uiNewCapacity), pAllocator);
  }
}

template <typename T, ezUInt16 Size>
EZ_ALWAYS_INLINE ezUInt64 ezSmallArrayBase<T, Size>::GetHeapMemoryUsage() const
{
  return m_uiCapacity <= Size ? 0 : m_uiCapacity * sizeof(T);
}

template <typename T, ezUInt16 Size>
template <typename U>
EZ_ALWAYS_INLINE const U& ezSmallArrayBase<T, Size>::GetUserData() const
{
  static_assert(sizeof(U) <= sizeof(ezUInt32));
  return reinterpret_cast<const U&>(m_uiUserData);
}

template <typename T, ezUInt16 Size>
template <typename U>
EZ_ALWAYS_INLINE U& ezSmallArrayBase<T, Size>::GetUserData()
{
  static_assert(sizeof(U) <= sizeof(ezUInt32));
  return reinterpret_cast<U&>(m_uiUserData);
}

template <typename T, ezUInt16 Size>
void ezSmallArrayBase<T, Size>::SetCapacity(ezUInt16 uiCapacity, ezAllocator* pAllocator)
{
  if (m_uiCapacity > Size && uiCapacity > m_uiCapacity)
  {
    m_pElements = EZ_EXTEND_RAW_BUFFER(pAllocator, m_pElements, m_uiCount, uiCapacity);
    m_uiCapacity = uiCapacity;
  }
  else
  {
    // special case when migrating from in-place to external storage or shrinking
    T* pOldElements = GetElementsPtr();

    const ezUInt32 uiOldCapacity = m_uiCapacity;
    const ezUInt32 uiNewCapacity = uiCapacity;
    m_uiCapacity = ezMath::Max(uiCapacity, Size);

    if (uiNewCapacity > Size)
    {
      // new external storage
      T* pNewElements = EZ_NEW_RAW_BUFFER(pAllocator, T, uiCapacity);
      ezMemoryUtils::RelocateConstruct(pNewElements, pOldElements, m_uiCount);
      m_pElements = pNewElements;
    }
    else
    {
      // Re-use inplace storage
      ezMemoryUtils::RelocateConstruct(GetElementsPtr(), pOldElements, m_uiCount);
    }

    if (uiOldCapacity > Size)
    {
      EZ_DELETE_RAW_BUFFER(pAllocator, pOldElements);
    }
  }
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

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE ezSmallArray<T, Size, AllocatorWrapper>::ezSmallArray(const ezSmallArray<T, Size, AllocatorWrapper>& other)
  : SUPER(other, AllocatorWrapper::GetAllocator())
{
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE ezSmallArray<T, Size, AllocatorWrapper>::ezSmallArray(const ezArrayPtr<const T>& other)
  : SUPER(other, AllocatorWrapper::GetAllocator())
{
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE ezSmallArray<T, Size, AllocatorWrapper>::ezSmallArray(ezSmallArray<T, Size, AllocatorWrapper>&& other)
  : SUPER(static_cast<SUPER&&>(other), AllocatorWrapper::GetAllocator())
{
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
ezSmallArray<T, Size, AllocatorWrapper>::~ezSmallArray()
{
  SUPER::Clear();
  SUPER::Compact(AllocatorWrapper::GetAllocator());
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE void ezSmallArray<T, Size, AllocatorWrapper>::operator=(const ezSmallArray<T, Size, AllocatorWrapper>& rhs)
{
  *this = ((ezArrayPtr<const T>)rhs); // redirect this to the ezArrayPtr version
  this->m_uiUserData = rhs.m_uiUserData;
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE void ezSmallArray<T, Size, AllocatorWrapper>::operator=(const ezArrayPtr<const T>& rhs)
{
  SUPER::CopyFrom(rhs, AllocatorWrapper::GetAllocator());
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE void ezSmallArray<T, Size, AllocatorWrapper>::operator=(ezSmallArray<T, Size, AllocatorWrapper>&& rhs) noexcept
{
  SUPER::MoveFrom(std::move(rhs), AllocatorWrapper::GetAllocator());
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE void ezSmallArray<T, Size, AllocatorWrapper>::SetCount(ezUInt16 uiCount)
{
  SUPER::SetCount(uiCount, AllocatorWrapper::GetAllocator());
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE void ezSmallArray<T, Size, AllocatorWrapper>::SetCount(ezUInt16 uiCount, const T& fillValue)
{
  SUPER::SetCount(uiCount, fillValue, AllocatorWrapper::GetAllocator());
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE void ezSmallArray<T, Size, AllocatorWrapper>::EnsureCount(ezUInt16 uiCount)
{
  SUPER::EnsureCount(uiCount, AllocatorWrapper::GetAllocator());
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
template <typename> // Second template needed so that the compiler does only instantiate it when called. Otherwise the static_assert would trigger early.
EZ_ALWAYS_INLINE void ezSmallArray<T, Size, AllocatorWrapper>::SetCountUninitialized(ezUInt16 uiCount)
{
  SUPER::SetCountUninitialized(uiCount, AllocatorWrapper::GetAllocator());
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE void ezSmallArray<T, Size, AllocatorWrapper>::InsertAt(ezUInt32 uiIndex, const T& value)
{
  SUPER::Insert(value, uiIndex, AllocatorWrapper::GetAllocator());
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE void ezSmallArray<T, Size, AllocatorWrapper>::InsertAt(ezUInt32 uiIndex, T&& value)
{
  SUPER::Insert(value, uiIndex, AllocatorWrapper::GetAllocator());
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE T& ezSmallArray<T, Size, AllocatorWrapper>::ExpandAndGetRef()
{
  return SUPER::ExpandAndGetRef(AllocatorWrapper::GetAllocator());
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE void ezSmallArray<T, Size, AllocatorWrapper>::PushBack(const T& value)
{
  SUPER::PushBack(value, AllocatorWrapper::GetAllocator());
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE void ezSmallArray<T, Size, AllocatorWrapper>::PushBack(T&& value)
{
  SUPER::PushBack(std::move(value), AllocatorWrapper::GetAllocator());
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE void ezSmallArray<T, Size, AllocatorWrapper>::PushBackRange(const ezArrayPtr<const T>& range)
{
  SUPER::PushBackRange(range, AllocatorWrapper::GetAllocator());
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE void ezSmallArray<T, Size, AllocatorWrapper>::Reserve(ezUInt16 uiCapacity)
{
  SUPER::Reserve(uiCapacity, AllocatorWrapper::GetAllocator());
}

template <typename T, ezUInt16 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE void ezSmallArray<T, Size, AllocatorWrapper>::Compact()
{
  SUPER::Compact(AllocatorWrapper::GetAllocator());
}

//////////////////////////////////////////////////////////////////////////

template <typename T, ezUInt16 Size>
typename ezSmallArrayBase<T, Size>::iterator begin(ezSmallArrayBase<T, Size>& ref_container)
{
  return ref_container.GetData();
}

template <typename T, ezUInt16 Size>
typename ezSmallArrayBase<T, Size>::const_iterator begin(const ezSmallArrayBase<T, Size>& container)
{
  return container.GetData();
}

template <typename T, ezUInt16 Size>
typename ezSmallArrayBase<T, Size>::const_iterator cbegin(const ezSmallArrayBase<T, Size>& container)
{
  return container.GetData();
}

template <typename T, ezUInt16 Size>
typename ezSmallArrayBase<T, Size>::reverse_iterator rbegin(ezSmallArrayBase<T, Size>& ref_container)
{
  return typename ezSmallArrayBase<T, Size>::reverse_iterator(ref_container.GetData() + ref_container.GetCount() - 1);
}

template <typename T, ezUInt16 Size>
typename ezSmallArrayBase<T, Size>::const_reverse_iterator rbegin(const ezSmallArrayBase<T, Size>& container)
{
  return typename ezSmallArrayBase<T, Size>::const_reverse_iterator(container.GetData() + container.GetCount() - 1);
}

template <typename T, ezUInt16 Size>
typename ezSmallArrayBase<T, Size>::const_reverse_iterator crbegin(const ezSmallArrayBase<T, Size>& container)
{
  return typename ezSmallArrayBase<T, Size>::const_reverse_iterator(container.GetData() + container.GetCount() - 1);
}

template <typename T, ezUInt16 Size>
typename ezSmallArrayBase<T, Size>::iterator end(ezSmallArrayBase<T, Size>& ref_container)
{
  return ref_container.GetData() + ref_container.GetCount();
}

template <typename T, ezUInt16 Size>
typename ezSmallArrayBase<T, Size>::const_iterator end(const ezSmallArrayBase<T, Size>& container)
{
  return container.GetData() + container.GetCount();
}

template <typename T, ezUInt16 Size>
typename ezSmallArrayBase<T, Size>::const_iterator cend(const ezSmallArrayBase<T, Size>& container)
{
  return container.GetData() + container.GetCount();
}

template <typename T, ezUInt16 Size>
typename ezSmallArrayBase<T, Size>::reverse_iterator rend(ezSmallArrayBase<T, Size>& ref_container)
{
  return typename ezSmallArrayBase<T, Size>::reverse_iterator(ref_container.GetData() - 1);
}

template <typename T, ezUInt16 Size>
typename ezSmallArrayBase<T, Size>::const_reverse_iterator rend(const ezSmallArrayBase<T, Size>& container)
{
  return typename ezSmallArrayBase<T, Size>::const_reverse_iterator(container.GetData() - 1);
}

template <typename T, ezUInt16 Size>
typename ezSmallArrayBase<T, Size>::const_reverse_iterator crend(const ezSmallArrayBase<T, Size>& container)
{
  return typename ezSmallArrayBase<T, Size>::const_reverse_iterator(container.GetData() - 1);
}
