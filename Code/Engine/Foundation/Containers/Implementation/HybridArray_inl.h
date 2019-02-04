
template <typename T, ezUInt32 Size>
ezHybridArrayBase<T, Size>::ezHybridArrayBase(ezAllocatorBase* pAllocator)
{
  this->m_pElements = nullptr; // if m_pElements is null the static array is used
  this->m_uiCapacity = Size;
  this->m_pAllocator = pAllocator;
}

template <typename T, ezUInt32 Size>
ezHybridArrayBase<T, Size>::ezHybridArrayBase(const ezHybridArrayBase<T, Size>& other, ezAllocatorBase* pAllocator)
{
  this->m_pElements = nullptr; // if m_pElements is null the static array is used
  this->m_uiCapacity = Size;
  this->m_pAllocator = pAllocator;

  ezArrayBase<T, ezHybridArrayBase<T, Size>>::operator=((ezArrayPtr<const T>)other); // redirect this to the ezArrayPtr version
}

template <typename T, ezUInt32 Size>
ezHybridArrayBase<T, Size>::ezHybridArrayBase(ezHybridArrayBase<T, Size>&& other, ezAllocatorBase* pAllocator)
{
  this->m_pElements = nullptr; // if m_pElements is null the static array is used
  this->m_uiCapacity = Size;
  this->m_pAllocator = pAllocator;

  *this = std::move(other);
}

template <typename T, ezUInt32 Size>
ezHybridArrayBase<T, Size>::ezHybridArrayBase(const ezArrayPtr<const T>& other, ezAllocatorBase* pAllocator)
{
  this->m_pElements = nullptr; // if m_pElements is null the static array is used
  this->m_uiCapacity = Size;
  this->m_pAllocator = pAllocator;

  ezArrayBase<T, ezHybridArrayBase<T, Size>>::operator=((ezArrayPtr<const T>)other);
}

template <typename T, ezUInt32 Size>
ezHybridArrayBase<T, Size>::~ezHybridArrayBase()
{
  this->Clear();

  if (this->m_pElements != nullptr)
    EZ_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);

  this->m_pElements = nullptr;
}

template <typename T, ezUInt32 Size>
EZ_ALWAYS_INLINE T* ezHybridArrayBase<T, Size>::GetStaticArray()
{
  return reinterpret_cast<T*>(m_StaticData);
}

template <typename T, ezUInt32 Size>
EZ_ALWAYS_INLINE const T* ezHybridArrayBase<T, Size>::GetStaticArray() const
{
  return reinterpret_cast<const T*>(m_StaticData);
}


template <typename T, ezUInt32 Size>
EZ_ALWAYS_INLINE void ezHybridArrayBase<T, Size>::operator=(const ezHybridArrayBase<T, Size>& rhs)
{
  ezArrayBase<T, ezHybridArrayBase<T, Size>>::operator=((ezArrayPtr<const T>)rhs); // redirect this to the ezArrayPtr version
}

template <typename T, ezUInt32 Size>
EZ_FORCE_INLINE void ezHybridArrayBase<T, Size>::operator=(ezHybridArrayBase<T, Size>&& rhs)
{
  // Clear any existing data (calls destructors if necessary)
  this->Clear();

  if (rhs.m_pElements == nullptr || (m_pAllocator != rhs.m_pAllocator))
  {
    // Ensure we have enough data.
    this->Reserve(rhs.m_uiCount);

    ezMemoryUtils::RelocateConstruct(this->GetElementsPtr(), rhs.GetElementsPtr(), rhs.m_uiCount);

    this->m_uiCount = rhs.m_uiCount;
    rhs.m_uiCount = 0;
  }
  else
  {
    // Move semantics !
    // We cannot do this when the allocators of this and rhs are different,
    // because the memory will be freed by this and not by rhs anymore.

    if (this->m_pElements != nullptr)
      EZ_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);

    this->m_pElements = rhs.m_pElements;
    this->m_uiCapacity = rhs.m_uiCapacity;
    this->m_uiCount = rhs.m_uiCount;

    rhs.m_uiCount = 0;
    rhs.m_pElements = nullptr;
    rhs.m_uiCapacity = Size;
  }
}

template <typename T, ezUInt32 Size>
void ezHybridArrayBase<T, Size>::SetCapacity(ezUInt32 uiCapacity)
{
  T* pNewData = nullptr;

  // if the static buffer is sufficient, use that
  if (uiCapacity <= Size)
  {
    this->m_uiCapacity = Size;

    // if we already use that static buffer, no reallocation is needed
    if (this->m_pElements == nullptr)
      return;

    pNewData = GetStaticArray();
  }
  else
  {
    // otherwise allocate a new buffer
    this->m_uiCapacity = uiCapacity;
    pNewData = EZ_NEW_RAW_BUFFER(m_pAllocator, T, this->m_uiCapacity);
  }

  ezMemoryUtils::RelocateConstruct(pNewData, this->GetElementsPtr(), this->m_uiCount);

  // if the previous buffer is not the static array, deallocate it
  if (this->m_pElements != nullptr)
    EZ_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);

  this->m_pElements = (pNewData == GetStaticArray()) ? nullptr : pNewData;
}

template <typename T, ezUInt32 Size>
void ezHybridArrayBase<T, Size>::Reserve(ezUInt32 uiCapacity)
{
  // m_uiCapacity will always be at least Size
  if (this->m_uiCapacity >= uiCapacity)
    return;

  ezUInt32 uiNewCapacity = ezMath::Max(this->m_uiCapacity + (this->m_uiCapacity / 2), uiCapacity);
  uiNewCapacity = (uiNewCapacity + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);

  SetCapacity(uiNewCapacity);
}

template <typename T, ezUInt32 Size>
void ezHybridArrayBase<T, Size>::Compact()
{
  if (this->IsEmpty())
  {
    // completely deallocate all data, if the array is empty.
    if (this->m_pElements != nullptr)
      EZ_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);

    this->m_uiCapacity = Size;
    this->m_pElements = nullptr;
  }
  else
  {
    const ezUInt32 uiNewCapacity = (this->m_uiCount + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);
    if (this->m_uiCapacity != uiNewCapacity)
      SetCapacity(uiNewCapacity);
  }
}

template <typename T, ezUInt32 Size>
ezUInt64 ezHybridArrayBase<T, Size>::GetHeapMemoryUsage() const
{
  if (this->m_uiCapacity <= Size)
    return 0;

  return (ezUInt64)sizeof(T) * (ezUInt64)this->m_uiCapacity;
}

template <typename T, ezUInt32 Size>
void ezHybridArrayBase<T, Size>::Swap(ezHybridArrayBase<T, Size>& other)
{
  struct Tmp : ezAligned<EZ_ALIGNMENT_OF(T)>
  {
    ezUInt8 m_StaticData[Size * sizeof(T)];
  };

  // If there is stuff in the local array we need to swap it
  const ezUInt32 localSize = this->m_uiCapacity <= Size ? this->m_uiCount : 0;
  const ezUInt32 otherLocalSize = other.m_uiCapacity <= Size ? other.m_uiCount : 0;
  if (localSize != 0 || otherLocalSize != 0)
  {
    Tmp tmp;
    ezMemoryUtils::RelocateConstruct(reinterpret_cast<T*>(tmp.m_StaticData), this->GetStaticArray(), localSize);
    ezMemoryUtils::RelocateConstruct(this->GetStaticArray(), other.GetStaticArray(), otherLocalSize);
    ezMemoryUtils::RelocateConstruct(other.GetStaticArray(), reinterpret_cast<T*>(tmp.m_StaticData), localSize);
  }

  ezMath::Swap(this->m_pAllocator, other.m_pAllocator);
  this->DoSwap(other);

  // Fixup pElements pointer after swap
  if (this->m_uiCapacity <= Size)
  {
    this->m_pElements = nullptr;
  }

  if (other.m_uiCapacity <= Size)
  {
    other.m_pElements = nullptr;
  }
}

template <typename T, ezUInt32 Size>
EZ_ALWAYS_INLINE T* ezHybridArrayBase<T, Size>::GetElementsPtr()
{
  if (this->m_pElements == nullptr)
    return GetStaticArray();
  return this->m_pElements;
}

template <typename T, ezUInt32 Size>
EZ_ALWAYS_INLINE const T* ezHybridArrayBase<T, Size>::GetElementsPtr() const
{
  if (this->m_pElements == nullptr)
    return GetStaticArray();
  return this->m_pElements;
}

template <typename T, ezUInt32 Size, typename A>
ezHybridArray<T, Size, A>::ezHybridArray() : ezHybridArrayBase<T, Size>(A::GetAllocator())
{
}

template <typename T, ezUInt32 Size, typename A>
ezHybridArray<T, Size, A>::ezHybridArray(ezAllocatorBase* pAllocator) : ezHybridArrayBase<T, Size>(pAllocator)
{
}

template <typename T, ezUInt32 Size, typename A>
ezHybridArray<T, Size, A>::ezHybridArray(const ezHybridArray<T, Size, A>& other) : ezHybridArrayBase<T, Size>(other, A::GetAllocator())
{
}

template <typename T, ezUInt32 Size, typename A>
ezHybridArray<T, Size, A>::ezHybridArray(const ezHybridArrayBase<T, Size>& other) : ezHybridArrayBase<T, Size>(other, A::GetAllocator())
{
}

template <typename T, ezUInt32 Size, typename A>
ezHybridArray<T, Size, A>::ezHybridArray(ezHybridArray<T, Size, A>&& other) : ezHybridArrayBase<T, Size>(std::move(other), A::GetAllocator())
{
}

template <typename T, ezUInt32 Size, typename A>
ezHybridArray<T, Size, A>::ezHybridArray(ezHybridArrayBase<T, Size>&& other) : ezHybridArrayBase<T, Size>(std::move(other), A::GetAllocator())
{
}

template <typename T, ezUInt32 Size, typename A>
ezHybridArray<T, Size, A>::ezHybridArray(const ezArrayPtr<const T>& other) : ezHybridArrayBase<T, Size>(other, A::GetAllocator())
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
void ezHybridArray<T, Size, A>::operator=(ezHybridArray<T, Size, A>&& rhs)
{
  ezHybridArrayBase<T, Size>::operator=(std::move(rhs));
}

template <typename T, ezUInt32 Size, typename A>
void ezHybridArray<T, Size, A>::operator=(ezHybridArrayBase<T, Size>&& rhs)
{
  ezHybridArrayBase<T, Size>::operator=(std::move(rhs));
}

template <typename T, ezUInt32 Size, typename A>
void ezHybridArray<T, Size, A>::operator=(const ezArrayPtr<const T>& rhs)
{
  ezArrayBase<T, ezHybridArrayBase<T, Size>>::operator=((ezArrayPtr<const T>)rhs);
}

