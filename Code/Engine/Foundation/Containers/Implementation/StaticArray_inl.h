
template <typename T, ezUInt32 C>
ezStaticArray<T, C>::ezStaticArray()
{
  this->m_pElements = GetStaticArray();
}

template <typename T, ezUInt32 C>
ezStaticArray<T, C>::ezStaticArray(const ezStaticArray<T, C>& rhs)
{
  this->m_pElements = GetStaticArray();

  *this = (ezArrayPtr<const T>) rhs; // redirect this to the ezArrayPtr version
}

template <typename T, ezUInt32 C>
template <ezUInt32 OtherCapacity>
ezStaticArray<T, C>::ezStaticArray(const ezStaticArray<T, OtherCapacity>& rhs)
{
  this->m_pElements = GetStaticArray();

  *this = (ezArrayPtr<const T>) rhs; // redirect this to the ezArrayPtr version
}

template <typename T, ezUInt32 C>
ezStaticArray<T, C>::ezStaticArray(const ezArrayPtr<const T>& rhs)
{
  this->m_pElements = GetStaticArray();

  *this = rhs;
}

template <typename T, ezUInt32 C>
ezStaticArray<T, C>::~ezStaticArray()
{
  this->Clear();

  this->m_pElements = nullptr;
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE T* ezStaticArray<T, C>::GetStaticArray()
{
  return reinterpret_cast<T*>(m_Data);
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE void ezStaticArray<T, C>::Reserve(ezUInt32 uiCapacity)
{
  EZ_ASSERT_DEV(uiCapacity <= C, "The static array has a fixed capacity of %i, cannot reserve more elements than that.", C);
  // Nothing to do here
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE void ezStaticArray<T, C>::operator= (const ezStaticArray<T, C>& rhs)
{
  *this = (ezArrayPtr<const T>) rhs; // redirect this to the ezArrayPtr version
}

template <typename T, ezUInt32 C>
template <ezUInt32 OtherCapacity>
EZ_FORCE_INLINE void ezStaticArray<T, C>::operator= (const ezStaticArray<T, OtherCapacity>& rhs)
{
  *this = (ezArrayPtr<const T>) rhs; // redirect this to the ezArrayPtr version
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE void ezStaticArray<T, C>::operator= (const ezArrayPtr<const T>& rhs)
{
  ezArrayBase<T, ezStaticArray<T, C>>::operator=(rhs);
}

