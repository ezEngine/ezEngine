
template <typename T>
ezDynamicArrayBase<T>::ezDynamicArrayBase(ezAllocator* pAllocator)
  : m_pAllocator(pAllocator)
{
}

template <typename T>
ezDynamicArrayBase<T>::ezDynamicArrayBase(T* pInplaceStorage, ezUInt32 uiCapacity, ezAllocator* pAllocator)
  : m_pAllocator(pAllocator)
{
  m_pAllocator.SetFlags(Storage::External);
  this->m_uiCapacity = uiCapacity;
  this->m_pElements = pInplaceStorage;
}

template <typename T>
ezDynamicArrayBase<T>::ezDynamicArrayBase(const ezDynamicArrayBase<T>& other, ezAllocator* pAllocator)
  : m_pAllocator(pAllocator)
{
  ezArrayBase<T, ezDynamicArrayBase<T>>::operator=((ezArrayPtr<const T>)other); // redirect this to the ezArrayPtr version
}

template <typename T>
ezDynamicArrayBase<T>::ezDynamicArrayBase(ezDynamicArrayBase<T>&& other, ezAllocator* pAllocator)
  : m_pAllocator(pAllocator)
{
  *this = std::move(other);
}

template <typename T>
ezDynamicArrayBase<T>::ezDynamicArrayBase(const ezArrayPtr<const T>& other, ezAllocator* pAllocator)
  : m_pAllocator(pAllocator)
{
  ezArrayBase<T, ezDynamicArrayBase<T>>::operator=(other);
}

template <typename T>
ezDynamicArrayBase<T>::~ezDynamicArrayBase()
{
  this->Clear();

  if (m_pAllocator.GetFlags() == Storage::Owned)
  {
    // only delete our storage, if we own it
    EZ_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);
  }

  this->m_uiCapacity = 0;
  this->m_pElements = nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE void ezDynamicArrayBase<T>::operator=(const ezDynamicArrayBase<T>& rhs)
{
  ezArrayBase<T, ezDynamicArrayBase<T>>::operator=((ezArrayPtr<const T>)rhs); // redirect this to the ezArrayPtr version
}

template <typename T>
inline void ezDynamicArrayBase<T>::operator=(ezDynamicArrayBase<T>&& rhs) noexcept
{
  // Clear any existing data (calls destructors if necessary)
  this->Clear();

  if (this->m_pAllocator.GetPtr() == rhs.m_pAllocator.GetPtr() && rhs.m_pAllocator.GetFlags() == Storage::Owned) // only move the storage of rhs, if it owns it
  {
    if (this->m_pAllocator.GetFlags() == Storage::Owned)
    {
      // only delete our storage, if we own it
      EZ_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);
    }

    // we now own this storage
    this->m_pAllocator.SetFlags(Storage::Owned);

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
    // Ensure we have enough data.
    this->Reserve(rhs.m_uiCount);
    this->m_uiCount = rhs.m_uiCount;

    ezMemoryUtils::RelocateConstruct(
      this->GetElementsPtr(), rhs.GetElementsPtr() /* vital to remap rhs.m_pElements to absolute ptr */, rhs.m_uiCount);

    rhs.m_uiCount = 0;
  }
}

template <typename T>
void ezDynamicArrayBase<T>::Swap(ezDynamicArrayBase<T>& other)
{
  if (this->m_pAllocator.GetFlags() == Storage::External && other.m_pAllocator.GetFlags() == Storage::External)
  {
    constexpr ezUInt32 InplaceStorageSize = 64;

    struct alignas(alignof(T)) Tmp
    {
      ezUInt8 m_StaticData[InplaceStorageSize * sizeof(T)];
    };

    const ezUInt32 localSize = this->m_uiCount;
    const ezUInt32 otherLocalSize = other.m_uiCount;

    if (localSize <= InplaceStorageSize && otherLocalSize <= InplaceStorageSize && localSize <= other.m_uiCapacity &&
        otherLocalSize <= this->m_uiCapacity)
    {

      Tmp tmp;
      ezMemoryUtils::RelocateConstruct(reinterpret_cast<T*>(tmp.m_StaticData), this->GetElementsPtr(), localSize);
      ezMemoryUtils::RelocateConstruct(this->GetElementsPtr(), other.GetElementsPtr(), otherLocalSize);
      ezMemoryUtils::RelocateConstruct(other.GetElementsPtr(), reinterpret_cast<T*>(tmp.m_StaticData), localSize);

      ezMath::Swap(this->m_pAllocator, other.m_pAllocator);
      ezMath::Swap(this->m_uiCount, other.m_uiCount);

      return; // successfully swapped in place
    }

    // temp buffer was insufficient -> fallthrough
  }

  if (this->m_pAllocator.GetFlags() == Storage::External)
  {
    // enforce using own storage
    this->Reserve(this->m_uiCapacity + 1);
  }

  if (other.m_pAllocator.GetFlags() == Storage::External)
  {
    // enforce using own storage
    other.Reserve(other.m_uiCapacity + 1);
  }

  // no external storage involved -> swap pointers
  ezMath::Swap(this->m_pAllocator, other.m_pAllocator);
  this->DoSwap(other);
}

template <typename T>
void ezDynamicArrayBase<T>::SetCapacity(ezUInt32 uiCapacity)
{
  // do NOT early out here, it is vital that this function does its thing even if the old capacity would be sufficient

  if (this->m_pAllocator.GetFlags() == Storage::Owned && uiCapacity > this->m_uiCapacity)
  {
    this->m_pElements = EZ_EXTEND_RAW_BUFFER(this->m_pAllocator, this->m_pElements, this->m_uiCount, uiCapacity);
  }
  else
  {
    T* pOldElements = GetElementsPtr();

    T* pNewElements = EZ_NEW_RAW_BUFFER(this->m_pAllocator, T, uiCapacity);
    ezMemoryUtils::RelocateConstruct(pNewElements, pOldElements, this->m_uiCount);

    if (this->m_pAllocator.GetFlags() == Storage::Owned)
    {
      EZ_DELETE_RAW_BUFFER(this->m_pAllocator, pOldElements);
    }

    // after any resize, we definitely own the storage
    this->m_pAllocator.SetFlags(Storage::Owned);
    this->m_pElements = pNewElements;
  }

  this->m_uiCapacity = uiCapacity;
}

template <typename T>
void ezDynamicArrayBase<T>::Reserve(ezUInt32 uiCapacity)
{
  if (this->m_uiCapacity >= uiCapacity)
    return;

  const ezUInt64 uiCurCap64 = static_cast<ezUInt64>(this->m_uiCapacity);
  ezUInt64 uiNewCapacity64 = uiCurCap64 + (uiCurCap64 / 2);

  uiNewCapacity64 = ezMath::Max<ezUInt64>(uiNewCapacity64, uiCapacity);

  constexpr ezUInt64 uiMaxCapacity = 0xFFFFFFFFllu - (CAPACITY_ALIGNMENT - 1);

  // the maximum value must leave room for the capacity alignment computation below (without overflowing the 32 bit range)
  uiNewCapacity64 = ezMath::Min<ezUInt64>(uiNewCapacity64, uiMaxCapacity);

  uiNewCapacity64 = (uiNewCapacity64 + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);

  EZ_ASSERT_DEV(uiCapacity <= uiNewCapacity64, "The requested capacity of {} elements exceeds the maximum possible capacity of {} elements.", uiCapacity, uiMaxCapacity);

  SetCapacity(static_cast<ezUInt32>(uiNewCapacity64 & 0xFFFFFFFF));
}

template <typename T>
void ezDynamicArrayBase<T>::Compact()
{
  if (m_pAllocator.GetFlags() == Storage::External)
    return;

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
EZ_ALWAYS_INLINE T* ezDynamicArrayBase<T>::GetElementsPtr()
{
  return this->m_pElements;
}

template <typename T>
EZ_ALWAYS_INLINE const T* ezDynamicArrayBase<T>::GetElementsPtr() const
{
  return this->m_pElements;
}

template <typename T>
ezUInt64 ezDynamicArrayBase<T>::GetHeapMemoryUsage() const
{
  if (this->m_pAllocator.GetFlags() == Storage::External)
    return 0;

  return (ezUInt64)this->m_uiCapacity * (ezUInt64)sizeof(T);
}

template <typename T, typename A>
ezDynamicArray<T, A>::ezDynamicArray()
  : ezDynamicArrayBase<T>(A::GetAllocator())
{
}

template <typename T, typename A>
ezDynamicArray<T, A>::ezDynamicArray(ezAllocator* pAllocator)
  : ezDynamicArrayBase<T>(pAllocator)
{
}

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

template <typename T, typename AllocatorWrapper>
ezArrayPtr<const T* const> ezMakeArrayPtr(const ezDynamicArray<T*, AllocatorWrapper>& dynArray)
{
  return ezArrayPtr<const T* const>(dynArray.GetData(), dynArray.GetCount());
}

template <typename T, typename AllocatorWrapper>
ezArrayPtr<const T> ezMakeArrayPtr(const ezDynamicArray<T, AllocatorWrapper>& dynArray)
{
  return ezArrayPtr<const T>(dynArray.GetData(), dynArray.GetCount());
}

template <typename T, typename AllocatorWrapper>
ezArrayPtr<T> ezMakeArrayPtr(ezDynamicArray<T, AllocatorWrapper>& in_dynArray)
{
  return ezArrayPtr<T>(in_dynArray.GetData(), in_dynArray.GetCount());
}
