
template <typename T, ezUInt32 Size>
ezHybridArrayBase<T, Size>::ezHybridArrayBase(ezIAllocator* pAllocator)
{
  m_pElements = GetStaticArray();
  m_uiCapacity = Size;
  m_pAllocator = pAllocator;
}

template <typename T, ezUInt32 Size>
ezHybridArrayBase<T, Size>::ezHybridArrayBase(const ezHybridArrayBase<T, Size>& other, ezIAllocator* pAllocator)
{
  m_pElements = GetStaticArray();
  m_uiCapacity = Size;
  m_pAllocator = pAllocator;

  *this = (ezArrayPtr<T>) other; // redirect this to the ezArrayPtr version
}

template <typename T, ezUInt32 Size>
ezHybridArrayBase<T, Size>::ezHybridArrayBase(const ezArrayPtr<T>& other, ezIAllocator* pAllocator)
{
  m_pElements = GetStaticArray();
  m_uiCapacity = Size;
  m_pAllocator = pAllocator;

  *this = other;
}

template <typename T, ezUInt32 Size>
ezHybridArrayBase<T, Size>::~ezHybridArrayBase()
{
  Clear();

  if (m_pElements != GetStaticArray())
    EZ_DELETE_RAW_BUFFER(m_pAllocator, m_pElements);

  m_pElements = NULL;
}

template <typename T, ezUInt32 Size>
EZ_FORCE_INLINE T* ezHybridArrayBase<T, Size>::GetStaticArray()
{
  return reinterpret_cast<T*>(m_StaticData);
}

template <typename T, ezUInt32 Size>
EZ_FORCE_INLINE void ezHybridArrayBase<T, Size>::operator= (const ezHybridArrayBase<T, Size>& rhs)
{
  *this = (ezArrayPtr<T>) rhs; // redirect this to the ezArrayPtr version
}

template <typename T, ezUInt32 Size>
void ezHybridArrayBase<T, Size>::operator= (const ezArrayPtr<T>& rhs)
{
  SetCount(rhs.GetCount());
  ezMemoryUtils::Copy(m_pElements, rhs.GetPtr(), rhs.GetCount());
}

template <typename T, ezUInt32 Size>
void ezHybridArrayBase<T, Size>::SetCapacity(ezUInt32 uiCapacity)
{
  T* pNewData = NULL;

  // if the static buffer is sufficient, use that
  if (uiCapacity <= Size)
  {
    m_uiCapacity = Size;

    // if we already use that static buffer, no reallocation is needed
    if (m_pElements == GetStaticArray())
      return;

    pNewData = GetStaticArray();
  }
  else
  {
    // otherwise allocate a new buffer
    m_uiCapacity = uiCapacity;
    pNewData = EZ_NEW_RAW_BUFFER(m_pAllocator, T, m_uiCapacity);
  }

  ezMemoryUtils::Construct(pNewData, m_pElements, m_uiCount);
  ezMemoryUtils::Destruct(m_pElements, m_uiCount);

  // if the previous buffer is not the static array, deallocate it
  if (m_pElements != GetStaticArray())
    EZ_DELETE_RAW_BUFFER(m_pAllocator, m_pElements);

  m_pElements = pNewData;
}

template <typename T, ezUInt32 Size>
void ezHybridArrayBase<T, Size>::Reserve(ezUInt32 uiCapacity)
{
  // m_uiCapacity will always be at least Size
  if (m_uiCapacity >= uiCapacity)
    return;

  ezUInt32 uiNewCapacity = ezMath::Max(m_uiCapacity + m_uiCapacity / 2, uiCapacity);
  uiNewCapacity = (uiNewCapacity + (CAPACITY_ALIGNMENT-1)) & ~(CAPACITY_ALIGNMENT-1);

  SetCapacity(uiNewCapacity);
}

template <typename T, ezUInt32 Size>
void ezHybridArrayBase<T, Size>::Compact()
{
  if (IsEmpty())
  {
    // completely deallocate all data, if the array is empty.
    if (m_pElements != GetStaticArray())
      EZ_DELETE_RAW_BUFFER(m_pAllocator, m_pElements);

    m_uiCapacity = Size;
    m_pElements = GetStaticArray();
  }
  else
  {
    const ezUInt32 uiNewCapacity = (m_uiCount + (CAPACITY_ALIGNMENT-1)) & ~(CAPACITY_ALIGNMENT-1);
    if (m_uiCapacity != uiNewCapacity)
      SetCapacity(uiNewCapacity);
  }
}

template <typename T, ezUInt32 Size>
void ezHybridArrayBase<T, Size>::SetCount(ezUInt32 uiCount)
{
  const ezUInt32 uiOldCount = m_uiCount;
  const ezUInt32 uiNewCount = uiCount;
  if (uiNewCount > uiOldCount)
  {
    Reserve(uiNewCount);
    ezMemoryUtils::Construct(m_pElements + uiOldCount, uiNewCount - uiOldCount);  
  }
  else if (uiNewCount < uiOldCount)
  {
    ezMemoryUtils::Destruct(m_pElements + uiNewCount, uiOldCount - uiNewCount);
  }
  m_uiCount = uiCount;
}


template <typename T, ezUInt32 Size, typename A>
ezHybridArray<T, Size, A>::ezHybridArray() : ezHybridArrayBase(A::GetAllocator())
{
}

template <typename T, ezUInt32 Size, typename A>
ezHybridArray<T, Size, A>:: ezHybridArray(ezIAllocator* pAllocator) : ezHybridArrayBase(pAllocator)
{
}

template <typename T, ezUInt32 Size, typename A>
ezHybridArray<T, Size, A>::ezHybridArray(const ezHybridArray<T, Size, A>& other) : ezHybridArrayBase(other, A::GetAllocator())
{
}

template <typename T, ezUInt32 Size, typename A>
ezHybridArray<T, Size, A>:: ezHybridArray(const ezHybridArrayBase<T, Size>& other) : ezHybridArrayBase(other, A::GetAllocator())
{
}

template <typename T, ezUInt32 Size, typename A>
ezHybridArray<T, Size, A>::ezHybridArray(const ezArrayPtr<T>& other) : ezHybridArrayBase(other, A::GetAllocator())
{
}

template <typename T, ezUInt32 Size, typename A>
void ezHybridArray<T, Size, A>::operator=(const ezHybridArray<T, Size, A>& rhs)
{
  ezHybridArrayBase<T, Size>::operator=(rhs);
}

template <typename T, ezUInt32 Size, typename A>
void ezHybridArray<T, Size, A>::operator=(const ezHybridArrayBase<T, Size>& rhs)
{
  ezHybridArrayBase<T, Size>::operator=(rhs);
}

template <typename T, ezUInt32 Size, typename A>
void ezHybridArray<T, Size, A>::operator=(const ezArrayPtr<T>& rhs)
{
  ezHybridArrayBase<T, Size>::operator=(rhs);
}

