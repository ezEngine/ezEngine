
template <typename T>
ezDynamicArrayBase<T>::ezDynamicArrayBase(ezAllocatorBase* pAllocator)
{
  this->m_uiCapacity = 0;
  m_pAllocator = pAllocator;
}

template <typename T>
ezDynamicArrayBase<T>::ezDynamicArrayBase(const ezDynamicArrayBase<T>& other, ezAllocatorBase* pAllocator)
{
  this->m_uiCapacity = 0;
  m_pAllocator = pAllocator;

  ezArrayBase<T, ezDynamicArrayBase<T>>::operator=((ezArrayPtr<const T>)other); // redirect this to the ezArrayPtr version
}

template <typename T>
ezDynamicArrayBase<T>::ezDynamicArrayBase(ezDynamicArrayBase<T>&& other, ezAllocatorBase* pAllocator)
{
  this->m_uiCapacity = 0;
  m_pAllocator = pAllocator;

  *this = std::move(other);
}

template <typename T>
ezDynamicArrayBase<T>::ezDynamicArrayBase(const ezArrayPtr<const T>& other, ezAllocatorBase* pAllocator)
{
  this->m_uiCapacity = 0;
  m_pAllocator = pAllocator;

  ezArrayBase<T, ezDynamicArrayBase<T>>::operator=((ezArrayPtr<const T>)other);
}

template <typename T>
ezDynamicArrayBase<T>::~ezDynamicArrayBase()
{
  this->Clear();
  EZ_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);
  this->m_uiCapacity = 0;
}

template <typename T>
EZ_FORCE_INLINE void ezDynamicArrayBase<T>::operator= (const ezDynamicArrayBase<T>& rhs)
{
  ezArrayBase<T, ezDynamicArrayBase<T>>::operator=((ezArrayPtr<const T>)rhs); // redirect this to the ezArrayPtr version
}

template <typename T>
EZ_FORCE_INLINE void ezDynamicArrayBase<T>::operator= (ezDynamicArrayBase<T>&& rhs)
{
  if (this->m_pAllocator == rhs.m_pAllocator)
  {
    // clear any existing data
    this->Clear();
    EZ_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);

    // move the data over from the other array
    this->m_uiCount = rhs.m_uiCount;
    this->m_uiCapacity = rhs.m_uiCapacity;
    this->m_pElements = rhs.m_pElements;

    // reset the other array to not reference the data anymore
    rhs.m_pElements = nullptr;
    rhs.m_uiCount = 0;
    rhs.m_uiCapacity = 0;
  }
  else
  {
    ezArrayBase<T, ezDynamicArrayBase<T>>::operator=((ezArrayPtr<const T>)rhs); // redirect this to the ezArrayPtr version
    rhs.Clear();
  }
}

template <typename T>
void ezDynamicArrayBase<T>::SetCapacity(ezUInt32 uiCapacity)
{
  this->m_uiCapacity = uiCapacity;
  this->m_pElements = EZ_EXTEND_RAW_BUFFER(this->m_pAllocator, this->m_pElements, this->m_uiCount, this->m_uiCapacity);
}

template <typename T>
void ezDynamicArrayBase<T>::Reserve(ezUInt32 uiCapacity)
{
  if (this->m_uiCapacity >= uiCapacity)
    return;

  ezUInt32 uiNewCapacity = ezMath::Max(this->m_uiCapacity + (this->m_uiCapacity / 2), uiCapacity);
  uiNewCapacity = (uiNewCapacity + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);
  SetCapacity(uiNewCapacity);
}

template <typename T>
void ezDynamicArrayBase<T>::Compact()
{
  if (this->IsEmpty())
  {
    // completely deallocate all data, if the array is empty.
    EZ_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);
    this->m_uiCapacity = 0;
  }
  else
  {
    const ezUInt32 uiNewCapacity = (this->m_uiCount + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);
    if (this->m_uiCapacity != uiNewCapacity)
      SetCapacity(uiNewCapacity);
  }
}

template <typename T>
ezUInt64 ezDynamicArrayBase<T>::GetHeapMemoryUsage() const
{
  return (ezUInt64) this->m_uiCapacity * (ezUInt64) sizeof(T);
}

template <typename T, typename A>
ezDynamicArray<T, A>::ezDynamicArray() : ezDynamicArrayBase<T>(A::GetAllocator())
{
}

template <typename T, typename A>
ezDynamicArray<T, A>::ezDynamicArray(ezAllocatorBase* pAllocator) : ezDynamicArrayBase<T>(pAllocator)
{
}

template <typename T, typename A>
ezDynamicArray<T, A>::ezDynamicArray(const ezDynamicArray<T, A>& other) : ezDynamicArrayBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
ezDynamicArray<T, A>::ezDynamicArray(const ezDynamicArrayBase<T>& other) : ezDynamicArrayBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
ezDynamicArray<T, A>::ezDynamicArray(const ezArrayPtr<const T>& other) : ezDynamicArrayBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
ezDynamicArray<T, A>::ezDynamicArray(ezDynamicArray<T, A>&& other) : ezDynamicArrayBase<T>(std::move(other), A::GetAllocator())
{
}

template <typename T, typename A>
ezDynamicArray<T, A>::ezDynamicArray(ezDynamicArrayBase<T>&& other) : ezDynamicArrayBase<T>(std::move(other), A::GetAllocator())
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
void ezDynamicArray<T, A>::operator=(ezDynamicArray<T, A>&& rhs)
{
  ezDynamicArrayBase<T>::operator=(std::move(rhs));
}

template <typename T, typename A>
void ezDynamicArray<T, A>::operator=(ezDynamicArrayBase<T>&& rhs)
{
  ezDynamicArrayBase<T>::operator=(std::move(rhs));
}

