
template <typename T, ezUInt32 C>
ezStaticArray<T, C>::ezStaticArray()
{
  this->m_pElements = GetStaticArray();
}

template <typename T, ezUInt32 C>
ezStaticArray<T, C>::ezStaticArray(const ezStaticArray<T, C>& rhs)
{
  this->m_pElements = GetStaticArray();

  *this = (ezArrayPtr<T>) rhs; // redirect this to the ezArrayPtr version
}

template <typename T, ezUInt32 C>
template <ezUInt32 OtherCapacity>
ezStaticArray<T, C>::ezStaticArray(const ezStaticArray<T, OtherCapacity>& rhs)
{
  this->m_pElements = GetStaticArray();

  *this = (ezArrayPtr<T>) rhs; // redirect this to the ezArrayPtr version
}

template <typename T, ezUInt32 C>
ezStaticArray<T, C>::ezStaticArray(const ezArrayPtr<T>& rhs)
{
  this->m_pElements = GetStaticArray();

  *this = rhs;
}

template <typename T, ezUInt32 C>
ezStaticArray<T, C>::~ezStaticArray()
{
  this->Clear();

  this->m_pElements = NULL;
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE T* ezStaticArray<T, C>::GetStaticArray()
{
  return reinterpret_cast<T*>(m_Data);
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE void ezStaticArray<T, C>::Reserve(ezUInt32 uiCapacity)
{
  EZ_ASSERT(uiCapacity <= C, "The static array has a fixed capacity of %i, cannot reserve more elements than that.", C);
  // Nothing to do here
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE void ezStaticArray<T, C>::operator= (const ezStaticArray<T, C>& rhs)
{
  *this = (ezArrayPtr<T>) rhs; // redirect this to the ezArrayPtr version
}

template <typename T, ezUInt32 C>
template <ezUInt32 OtherCapacity>
EZ_FORCE_INLINE void ezStaticArray<T, C>::operator= (const ezStaticArray<T, OtherCapacity>& rhs)
{
  *this = (ezArrayPtr<T>) rhs; // redirect this to the ezArrayPtr version
}

template <typename T, ezUInt32 C>
void ezStaticArray<T, C>::operator= (const ezArrayPtr<T>& rhs)
{
  EZ_ASSERT(rhs.GetCount() <= C, "Cannot copy an array of size %i into this array with capacity %i.", rhs.GetCount(), C);

  SetCount(ezMath::Min(C, rhs.GetCount()));

  ezMemoryUtils::Copy(this->m_pElements, rhs.GetPtr(), this->m_uiCount);
}

template <typename T, ezUInt32 C>
void ezStaticArray<T, C>::SetCount(ezUInt32 uiCount)
{
  EZ_ASSERT(uiCount <= C, "The static array has a capacity of %i elements, you cannot resize it to hold %i elements.", C, uiCount);

  const ezUInt32 uiOldCount = this->m_uiCount;
  const ezUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    ezMemoryUtils::Construct(this->m_pElements + uiOldCount, uiNewCount - uiOldCount);  
  }
  else if (uiNewCount < uiOldCount)
  {
    ezMemoryUtils::Destruct(this->m_pElements + uiNewCount, uiOldCount - uiNewCount);
  }

  this->m_uiCount = uiCount;
}
