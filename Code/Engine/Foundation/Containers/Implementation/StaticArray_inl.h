
template <typename T, ezUInt32 C>
ezStaticArray<T, C>::ezStaticArray()
{
  EZ_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
  this->m_uiCapacity = C;
}

template <typename T, ezUInt32 C>
ezStaticArray<T, C>::ezStaticArray(const ezStaticArray<T, C>& rhs)
{
  EZ_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
  this->m_uiCapacity = C;
  *this = (ezArrayPtr<const T>)rhs; // redirect this to the ezArrayPtr version
}

template <typename T, ezUInt32 C>
template <ezUInt32 OtherCapacity>
ezStaticArray<T, C>::ezStaticArray(const ezStaticArray<T, OtherCapacity>& rhs)
{
  EZ_CHECK_AT_COMPILETIME(OtherCapacity <= C);

  EZ_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
  this->m_uiCapacity = C;

  *this = (ezArrayPtr<const T>)rhs; // redirect this to the ezArrayPtr version
}

template <typename T, ezUInt32 C>
ezStaticArray<T, C>::ezStaticArray(const ezArrayPtr<const T>& rhs)
{
  EZ_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
  this->m_uiCapacity = C;

  *this = rhs;
}

template <typename T, ezUInt32 C>
ezStaticArray<T, C>::~ezStaticArray()
{
  this->Clear();
  EZ_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
}

template <typename T, ezUInt32 C>
EZ_ALWAYS_INLINE T* ezStaticArray<T, C>::GetStaticArray()
{
  return reinterpret_cast<T*>(m_Data);
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE const T* ezStaticArray<T, C>::GetStaticArray() const
{
  return reinterpret_cast<const T*>(m_Data);
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE void ezStaticArray<T, C>::Reserve(ezUInt32 uiCapacity)
{
  EZ_ASSERT_DEV(uiCapacity <= C, "The static array has a fixed capacity of {0}, cannot reserve more elements than that.", C);
  // Nothing to do here
}

template <typename T, ezUInt32 C>
EZ_ALWAYS_INLINE void ezStaticArray<T, C>::operator=(const ezStaticArray<T, C>& rhs)
{
  *this = (ezArrayPtr<const T>)rhs; // redirect this to the ezArrayPtr version
}

template <typename T, ezUInt32 C>
template <ezUInt32 OtherCapacity>
EZ_ALWAYS_INLINE void ezStaticArray<T, C>::operator=(const ezStaticArray<T, OtherCapacity>& rhs)
{
  *this = (ezArrayPtr<const T>)rhs; // redirect this to the ezArrayPtr version
}

template <typename T, ezUInt32 C>
EZ_ALWAYS_INLINE void ezStaticArray<T, C>::operator=(const ezArrayPtr<const T>& rhs)
{
  ezArrayBase<T, ezStaticArray<T, C>>::operator=(rhs);
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE T* ezStaticArray<T, C>::GetElementsPtr()
{
  return GetStaticArray();
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE const T* ezStaticArray<T, C>::GetElementsPtr() const
{
  return GetStaticArray();
}

