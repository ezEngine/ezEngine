#pragma once

#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Containers/Implementation/ArrayIterator.h>

/// \brief This class encapsulates an array and it's size. It is recommended to use this class instead of plain C arrays.
///
/// No data is deallocated at destruction, the ezArrayPtr only allows for easier access.
template <typename T>
class ezArrayPtr
{
public:
  EZ_DECLARE_POD_TYPE();

  /// \brief Initializes the ezArrayPtr to be empty.
  EZ_FORCE_INLINE ezArrayPtr() : m_ptr(nullptr), m_uiCount(0) // [tested]
  {
  }

  /// \brief Initializes the ezArrayPtr with the given pointer and number of elements. No memory is allocated or copied.
  inline ezArrayPtr(T* ptr, ezUInt32 uiCount) // [tested]
  {
    if (ptr != nullptr && uiCount != 0)
    {
      m_ptr = ptr;
      m_uiCount = uiCount;
    }
    else
    {
      m_ptr = nullptr;
      m_uiCount = 0;
    }
  }

  /// \brief Initializes the ezArrayPtr to encapsulate the given array.
  template <ezUInt32 N>
  EZ_FORCE_INLINE ezArrayPtr(T (&staticArray)[N]) : m_ptr(staticArray), m_uiCount(N) // [tested]
  {
  }

  /// \brief Initializes the ezArrayPtr to be a copy of \a other. No memory is allocated or copied.
  EZ_FORCE_INLINE ezArrayPtr(const ezArrayPtr<typename ezTypeTraits<T>::NonConstType>& other) // [tested]
  {
    m_ptr = other.GetPtr();
    m_uiCount = other.GetCount();
  }

  /// \brief Copies the pointer and size of /a other. Does not allocate any data.
  EZ_FORCE_INLINE void operator=(const ezArrayPtr<typename ezTypeTraits<T>::NonConstType>& other) // [tested]
  {
    m_ptr = other.GetPtr();
    m_uiCount = other.GetCount();
  }

  /// \brief Returns the pointer to the array.
  EZ_FORCE_INLINE T* GetPtr() const // [tested]
  {
    return m_ptr;
  }

  /// \brief Returns the number of elements in the array.
  EZ_FORCE_INLINE ezUInt32 GetCount() const // [tested]
  {
    return m_uiCount;
  }

  /// \brief Creates a sub-array from this array.
  EZ_FORCE_INLINE ezArrayPtr<T> GetSubArray(ezUInt32 uiStart, ezUInt32 uiCount) // [tested]
  {
    EZ_ASSERT_DEV(uiStart + uiCount <= m_uiCount, "uiStart+uiCount (%i) has to be smaller or equal than the count (%i).", uiStart+uiCount, m_uiCount);
    return ezArrayPtr<T>(m_ptr + uiStart, uiCount);
  }

  /// \brief Creates a sub-array from this array.
  EZ_FORCE_INLINE const ezArrayPtr<T> GetSubArray(ezUInt32 uiStart, ezUInt32 uiCount) const // [tested]
  {
    EZ_ASSERT_DEV(uiStart + uiCount <= m_uiCount, "uiStart+uiCount (%i) has to be smaller or equal than the count (%i).", uiStart+uiCount, m_uiCount);
    return ezArrayPtr<T>(m_ptr + uiStart, uiCount);
  }

  /// \brief Index access.
  EZ_FORCE_INLINE T& operator[](ezUInt32 uiIndex) const // [tested]
  {
    EZ_ASSERT_DEV(uiIndex < m_uiCount, "Cannot access element %i, the array only holds %i elements.", uiIndex, m_uiCount);
    return m_ptr[uiIndex];
  }

  /// \brief Compares the two arrays for equality.
  inline bool operator==(const ezArrayPtr<typename ezTypeTraits<T>::NonConstType>& other) const // [tested]
  {
    if (m_uiCount != other.GetCount())
      return false;

    if (m_ptr == other.GetPtr())
      return true;

    return ezMemoryUtils::IsEqual(m_ptr, other.GetPtr(), m_uiCount);
  }

  /// \brief Compares the two arrays for equality.
  EZ_FORCE_INLINE bool operator==(const ezArrayPtr<const T>& other) const // [tested]
  {
    return other == *this;
  }

  /// \brief Compares the two arrays for inequality.
  EZ_FORCE_INLINE bool operator!=(const ezArrayPtr<typename ezTypeTraits<T>::NonConstType>& other) const // [tested]
  {
    return !operator==(other);
  }

  /// \brief Compares the two arrays for inequality.
  EZ_FORCE_INLINE bool operator!=(const ezArrayPtr<const T>& other) const // [tested]
  {
    return !operator==(other);
  }

  /// \brief Copies the data from \a other into this array. The arrays must have the exact same size.
  inline void CopyFrom(const ezArrayPtr<T>& other) // [tested]
  {
    EZ_ASSERT_DEV(m_uiCount == other.m_uiCount, "Count for copy does not match. Target has %d elements, source %d elements", m_uiCount, other.m_uiCount);

    ezMemoryUtils::Copy(m_ptr, other.m_ptr, m_uiCount);
  }

  /// \brief Resets the ezArray to be empty. 
  EZ_FORCE_INLINE void Reset() // [tested]
  {
    m_ptr = nullptr;
    m_uiCount = 0;
  }

  typedef const_iterator_base<ezArrayPtr<T>, T, false> const_iterator;
  typedef const_iterator_base<ezArrayPtr<T>, T, true> const_reverse_iterator;
  typedef iterator_base<ezArrayPtr<T>, T, false> iterator;
  typedef iterator_base<ezArrayPtr<T>, T, true> reverse_iterator;

private:
  T* m_ptr;
  ezUInt32 m_uiCount;
};

template <typename T>
typename ezArrayPtr<T>::iterator begin(ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::iterator(container, (size_t) 0); }

template <typename T>
typename ezArrayPtr<T>::const_iterator  begin(const ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::const_iterator(container, (size_t) 0); }

template <typename T>
typename ezArrayPtr<T>::const_iterator cbegin(const ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::const_iterator(container, (size_t) 0); }

template <typename T>
typename ezArrayPtr<T>::reverse_iterator rbegin(ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::reverse_iterator(container, (size_t) 0); }

template <typename T>
typename ezArrayPtr<T>::const_reverse_iterator rbegin(const ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::const_reverse_iterator(container, (size_t) 0); }

template <typename T>
typename ezArrayPtr<T>::const_reverse_iterator crbegin(const ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::const_reverse_iterator(container, (size_t) 0); }

template <typename T>
typename ezArrayPtr<T>::iterator end(ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::iterator(container, (size_t) container.GetCount()); }

template <typename T>
typename ezArrayPtr<T>::const_iterator end(const ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::const_iterator(container, (size_t) container.GetCount()); }

template <typename T>
typename ezArrayPtr<T>::const_iterator cend(const ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::const_iterator(container, (size_t) container.GetCount()); }

template <typename T>
typename ezArrayPtr<T>::reverse_iterator rend(ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::reverse_iterator(container, (size_t) container.GetCount()); }

template <typename T>
typename ezArrayPtr<T>::const_reverse_iterator  rend(const ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::const_reverse_iterator(container, (size_t) container.GetCount()); }

template <typename T>
typename ezArrayPtr<T>::const_reverse_iterator crend(const ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::const_reverse_iterator(container, (size_t) container.GetCount()); }


