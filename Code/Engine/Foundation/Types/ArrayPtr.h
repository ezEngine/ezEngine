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

  typedef T ElementType;
  typedef typename ezTypeTraits<T>::NonConstType MutableElementType;

  /// \brief Initializes the ezArrayPtr to be empty.
  EZ_FORCE_INLINE ezArrayPtr() : m_ptr(nullptr), m_uiCount(0u) // [tested]
  {
  }

  /// \brief Initializes the ezArrayPtr with the given pointer and number of elements. No memory is allocated or copied.
  inline ezArrayPtr(T* ptr, ezUInt32 uiCount) : m_ptr(ptr), m_uiCount(uiCount) // [tested]
  {
    // If any of the arguments is invalid, we invalidate ourself.
    if (m_ptr == nullptr || m_uiCount == 0)
    {
      m_ptr = nullptr;
      m_uiCount = 0;
    }
  }

  /// \brief Initializes the ezArrayPtr to encapsulate the given array.
  template <size_t N>
  EZ_FORCE_INLINE ezArrayPtr(T (&staticArray)[N]) : m_ptr(staticArray), m_uiCount(static_cast<ezUInt32>(N)) // [tested]
  {
  }

  /// \brief Initializes the ezArrayPtr to be a copy of \a other. No memory is allocated or copied.
  EZ_FORCE_INLINE ezArrayPtr(const ezArrayPtr<T>& other) : m_ptr(other.m_ptr), m_uiCount(other.m_uiCount) // [tested]
  {
  }

  /// \brief Convert to const version.
  operator ezArrayPtr<const T>() const { return ezArrayPtr<const T>(GetPtr(), GetCount()); } // [tested]

  /// \brief Copies the pointer and size of /a other. Does not allocate any data.
  EZ_FORCE_INLINE void operator=(const ezArrayPtr<T>& other) // [tested]
  {
    m_ptr = other.m_ptr;
    m_uiCount = other.m_uiCount;
  }

  /// \brief Returns the pointer to the array.
  EZ_FORCE_INLINE const T* GetPtr() const // [tested]
  {
    return m_ptr;
  }

  /// \brief Returns the pointer to the array.
  EZ_FORCE_INLINE T* GetPtr() // [tested]
  {
    return m_ptr;
  }

  /// \brief Returns whether the array is empty.
  EZ_FORCE_INLINE bool IsEmpty() const // [tested]
  {
    return GetCount() == 0;
  }

  /// \brief Returns the number of elements in the array.
  EZ_FORCE_INLINE ezUInt32 GetCount() const // [tested]
  {
    return m_uiCount;
  }

  /// \brief Creates a sub-array from this array.
  EZ_FORCE_INLINE ezArrayPtr<const T> GetSubArray(ezUInt32 uiStart, ezUInt32 uiCount) const // [tested]
  {
    EZ_ASSERT_DEV(uiStart + uiCount <= GetCount(), "uiStart+uiCount (%i) has to be smaller or equal than the count (%i).", uiStart + uiCount, GetCount());
    return ezArrayPtr<const T>(GetPtr() + uiStart, uiCount);
  }

  /// \brief Creates a sub-array from this array.
  EZ_FORCE_INLINE ezArrayPtr<T> GetSubArray(ezUInt32 uiStart, ezUInt32 uiCount) // [tested]
  {
    EZ_ASSERT_DEV(uiStart + uiCount <= GetCount(), "uiStart+uiCount (%i) has to be smaller or equal than the count (%i).", uiStart + uiCount, GetCount());
    return ezArrayPtr<T>(GetPtr() + uiStart, uiCount);
  }

  /// \brief Creates a sub-array from this array.
  /// \note \code ap.GetSubArray(i) \endcode is equivalent to \code ap.GetSubArray(i, ap.GetCount() - i) \endcode.
  EZ_FORCE_INLINE ezArrayPtr<const T> GetSubArray(ezUInt32 uiStart) const // [tested]
  {
    EZ_ASSERT_DEV(uiStart < GetCount(), "uiStart (%i) has to be smaller or equal than the count (%i).", uiStart, GetCount());
    return ezArrayPtr<const T>(GetPtr() + uiStart, GetCount() - uiStart);
  }

  /// \brief Creates a sub-array from this array.
  /// \note \code ap.GetSubArray(i) \endcode is equivalent to \code ap.GetSubArray(i, ap.GetCount() - i) \endcode.
  EZ_FORCE_INLINE ezArrayPtr<T> GetSubArray(ezUInt32 uiStart) // [tested]
  {
    EZ_ASSERT_DEV(uiStart < GetCount(), "uiStart (%i) has to be smaller or equal than the count (%i).", uiStart, GetCount());
    return ezArrayPtr<T>(GetPtr() + uiStart, GetCount() - uiStart);
  }

  /// \brief Index access.
  EZ_FORCE_INLINE const T& operator[](ezUInt32 uiIndex) const // [tested]
  {
    EZ_ASSERT_DEV(uiIndex < GetCount(), "Cannot access element %i, the array only holds %i elements.", uiIndex, GetCount());
    return GetPtr()[uiIndex];
  }

  /// \brief Index access.
  EZ_FORCE_INLINE T& operator[](ezUInt32 uiIndex) // [tested]
  {
    EZ_ASSERT_DEV(uiIndex < GetCount(), "Cannot access element %i, the array only holds %i elements.", uiIndex, GetCount());
    return GetPtr()[uiIndex];
  }

  /// \brief Compares the two arrays for equality.
  inline bool operator==(const ezArrayPtr<const T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return false;

    if (GetPtr() == other.GetPtr())
      return true;

    return ezMemoryUtils::IsEqual(GetPtr(), other.GetPtr(), GetCount());
  }

  /// \brief Compares the two arrays for inequality.
  EZ_FORCE_INLINE bool operator!=(const ezArrayPtr<const T>& other) const // [tested]
  {
    return !(*this == other);
  }

  /// \brief Copies the data from \a other into this array. The arrays must have the exact same size.
  inline void CopyFrom(const ezArrayPtr<const T>& other) // [tested]
  {
    EZ_ASSERT_DEV(GetCount() == other.GetCount(), "Count for copy does not match. Target has %d elements, source %d elements", GetCount(), other.GetCount());

    ezMemoryUtils::Copy(GetPtr(), other.GetPtr(), GetCount());
  }

  /// \brief Resets the ezArray to be empty. 
  EZ_FORCE_INLINE void Reset() // [tested]
  {
    m_ptr = nullptr;
    m_uiCount = 0;
  }

  typedef const T* const_iterator;
  typedef const_reverse_pointer_iterator<T> const_reverse_iterator;
  typedef T* iterator;
  typedef reverse_pointer_iterator<T> reverse_iterator;

private:
  T* m_ptr;
  ezUInt32 m_uiCount;
};

/// \brief Helper function to create ezArrayPtr from a pointer of some type and a count.
template<typename T>
EZ_FORCE_INLINE ezArrayPtr<T> ezMakeArrayPtr(T* ptr, ezUInt32 uiCount)
{
  return ezArrayPtr<T>(ptr, uiCount);
}

/// \brief Helper function to create ezArrayPtr from a static array the a size known at compile-time.
template<typename T, ezUInt32 N>
EZ_FORCE_INLINE ezArrayPtr<T> ezMakeArrayPtr(T(&staticArray)[N])
{
  return ezArrayPtr<T>(staticArray);
}

template <typename T>
typename ezArrayPtr<T>::iterator begin(ezArrayPtr<T>& container) { return container.GetPtr(); }

template <typename T>
typename ezArrayPtr<T>::const_iterator  begin(const ezArrayPtr<T>& container) { return container.GetPtr(); }

template <typename T>
typename ezArrayPtr<T>::const_iterator cbegin(const ezArrayPtr<T>& container) { return container.GetPtr(); }

template <typename T>
typename ezArrayPtr<T>::reverse_iterator rbegin(ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::reverse_iterator(container.GetPtr() + container.GetCount() - 1); }

template <typename T>
typename ezArrayPtr<T>::const_reverse_iterator rbegin(const ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1); }

template <typename T>
typename ezArrayPtr<T>::const_reverse_iterator crbegin(const ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1); }

template <typename T>
typename ezArrayPtr<T>::iterator end(ezArrayPtr<T>& container) { return container.GetPtr() + container.GetCount(); }

template <typename T>
typename ezArrayPtr<T>::const_iterator end(const ezArrayPtr<T>& container) { return container.GetPtr() + container.GetCount(); }

template <typename T>
typename ezArrayPtr<T>::const_iterator cend(const ezArrayPtr<T>& container) { return container.GetPtr() + container.GetCount(); }

template <typename T>
typename ezArrayPtr<T>::reverse_iterator rend(ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::reverse_iterator(container.GetPtr() - 1); }

template <typename T>
typename ezArrayPtr<T>::const_reverse_iterator  rend(const ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::const_reverse_iterator(container.GetPtr() - 1); }

template <typename T>
typename ezArrayPtr<T>::const_reverse_iterator crend(const ezArrayPtr<T>& container) { return typename ezArrayPtr<T>::const_reverse_iterator(container.GetPtr() - 1); }


