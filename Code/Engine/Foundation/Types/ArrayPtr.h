#pragma once

#include <Foundation/Memory/MemoryUtils.h>

#include <Foundation/Containers/Implementation/ArrayIterator.h>

// This #include is quite vital, do not remove it!
#include <Foundation/Strings/FormatString.h>

#include <Foundation/Math/Math.h>

#if EZ_ENABLED(EZ_INTEROP_STL_SPAN)
#  include <span>
#endif

/// \brief Value used by containers for indices to indicate an invalid index.
#ifndef ezInvalidIndex
#  define ezInvalidIndex 0xFFFFFFFF
#endif

namespace ezArrayPtrDetail
{
  template <typename U>
  struct ByteTypeHelper
  {
    using type = ezUInt8;
  };

  template <typename U>
  struct ByteTypeHelper<const U>
  {
    using type = const ezUInt8;
  };
} // namespace ezArrayPtrDetail

/// \brief This class encapsulates an array and it's size. It is recommended to use this class instead of plain C arrays.
///
/// No data is deallocated at destruction, the ezArrayPtr only allows for easier access.
template <typename T>
class ezArrayPtr
{
  template <typename U>
  friend class ezArrayPtr;

public:
  EZ_DECLARE_POD_TYPE();

  static_assert(!std::is_same_v<T, void>, "ezArrayPtr<void> is not allowed (anymore)");
  static_assert(!std::is_same_v<T, const void>, "ezArrayPtr<void> is not allowed (anymore)");

  using ByteType = typename ezArrayPtrDetail::ByteTypeHelper<T>::type;
  using ValueType = T;
  using PointerType = T*;

  /// \brief Initializes the ezArrayPtr to be empty.
  EZ_ALWAYS_INLINE ezArrayPtr() // [tested]
    : m_pPtr(nullptr)
    , m_uiCount(0u)
  {
  }

  /// \brief Copies the pointer and size of /a other. Does not allocate any data.
  EZ_ALWAYS_INLINE ezArrayPtr(const ezArrayPtr<T>& other) // [tested]
  {
    m_pPtr = other.m_pPtr;
    m_uiCount = other.m_uiCount;
  }

  /// \brief Initializes the ezArrayPtr with the given pointer and number of elements. No memory is allocated or copied.
  inline ezArrayPtr(T* pPtr, ezUInt32 uiCount) // [tested]
    : m_pPtr(pPtr)
    , m_uiCount(uiCount)
  {
    // If any of the arguments is invalid, we invalidate ourself.
    if (m_pPtr == nullptr || m_uiCount == 0)
    {
      m_pPtr = nullptr;
      m_uiCount = 0;
    }
  }

  /// \brief Initializes the ezArrayPtr to encapsulate the given array.
  template <size_t N>
  EZ_ALWAYS_INLINE ezArrayPtr(T (&staticArray)[N]) // [tested]
    : m_pPtr(staticArray)
    , m_uiCount(static_cast<ezUInt32>(N))
  {
  }

  /// \brief Initializes the ezArrayPtr to be a copy of \a other. No memory is allocated or copied.
  template <typename U>
  EZ_ALWAYS_INLINE ezArrayPtr(const ezArrayPtr<U>& other) // [tested]
    : m_pPtr(other.m_pPtr)
    , m_uiCount(other.m_uiCount)
  {
  }

#if EZ_ENABLED(EZ_INTEROP_STL_SPAN)
  template <typename U>
  EZ_ALWAYS_INLINE ezArrayPtr(const std::span<U>& other)
    : m_pPtr(other.data())
    , m_uiCount((ezUInt32)other.size())
  {
  }

  operator std::span<const T>() const
  {
    return std::span(GetPtr(), static_cast<size_t>(GetCount()));
  }

  operator std::span<T>()
  {
    return std::span(GetPtr(), static_cast<size_t>(GetCount()));
  }

  std::span<T> GetSpan()
  {
    return std::span(GetPtr(), static_cast<size_t>(GetCount()));
  }

  std::span<const T> GetSpan() const
  {
    return std::span(GetPtr(), static_cast<size_t>(GetCount()));
  }
#  endif

  /// \brief Convert to const version.
  operator ezArrayPtr<const T>() const { return ezArrayPtr<const T>(static_cast<const T*>(GetPtr()), GetCount()); } // [tested]

  /// \brief Copies the pointer and size of /a other. Does not allocate any data.
  EZ_ALWAYS_INLINE void operator=(const ezArrayPtr<T>& other) // [tested]
  {
    m_pPtr = other.m_pPtr;
    m_uiCount = other.m_uiCount;
  }

  /// \brief Clears the array
  EZ_ALWAYS_INLINE void Clear()
  {
    m_pPtr = nullptr;
    m_uiCount = 0;
  }

  EZ_ALWAYS_INLINE void operator=(std::nullptr_t) // [tested]
  {
    m_pPtr = nullptr;
    m_uiCount = 0;
  }

  /// \brief Returns the pointer to the array.
  EZ_ALWAYS_INLINE PointerType GetPtr() const // [tested]
  {
    return m_pPtr;
  }

  /// \brief Returns the pointer to the array.
  EZ_ALWAYS_INLINE PointerType GetPtr() // [tested]
  {
    return m_pPtr;
  }

  /// \brief Returns the pointer behind the last element of the array
  EZ_ALWAYS_INLINE PointerType GetEndPtr() { return m_pPtr + m_uiCount; }

  /// \brief Returns the pointer behind the last element of the array
  EZ_ALWAYS_INLINE PointerType GetEndPtr() const { return m_pPtr + m_uiCount; }

  /// \brief Returns whether the array is empty.
  EZ_ALWAYS_INLINE bool IsEmpty() const // [tested]
  {
    return GetCount() == 0;
  }

  /// \brief Returns the number of elements in the array.
  EZ_ALWAYS_INLINE ezUInt32 GetCount() const // [tested]
  {
    return m_uiCount;
  }

  /// \brief Creates a sub-array from this array.
  EZ_FORCE_INLINE ezArrayPtr<T> GetSubArray(ezUInt32 uiStart, ezUInt32 uiCount) const // [tested]
  {
    // the first check is necessary to also detect errors when uiStart+uiCount would overflow
    EZ_ASSERT_DEV(uiStart <= GetCount() && uiStart + uiCount <= GetCount(), "uiStart+uiCount ({0}) has to be smaller or equal than the count ({1}).",
      uiStart + uiCount, GetCount());
    return ezArrayPtr<T>(GetPtr() + uiStart, uiCount);
  }

  /// \brief Creates a sub-array from this array.
  /// \note \code ap.GetSubArray(i) \endcode is equivalent to \code ap.GetSubArray(i, ap.GetCount() - i) \endcode.
  EZ_FORCE_INLINE ezArrayPtr<T> GetSubArray(ezUInt32 uiStart) const // [tested]
  {
    EZ_ASSERT_DEV(uiStart <= GetCount(), "uiStart ({0}) has to be smaller or equal than the count ({1}).", uiStart, GetCount());
    return ezArrayPtr<T>(GetPtr() + uiStart, GetCount() - uiStart);
  }

  /// \brief Reinterprets this array as a byte array.
  EZ_ALWAYS_INLINE ezArrayPtr<const ByteType> ToByteArray() const
  {
    return ezArrayPtr<const ByteType>(reinterpret_cast<const ByteType*>(GetPtr()), GetCount() * sizeof(T));
  }

  /// \brief Reinterprets this array as a byte array.
  EZ_ALWAYS_INLINE ezArrayPtr<ByteType> ToByteArray() { return ezArrayPtr<ByteType>(reinterpret_cast<ByteType*>(GetPtr()), GetCount() * sizeof(T)); }


  /// \brief Cast an ArrayPtr to an ArrayPtr to a different, but same size, type
  template <typename U>
  EZ_ALWAYS_INLINE ezArrayPtr<U> Cast()
  {
    static_assert(sizeof(T) == sizeof(U), "Can only cast with equivalent element size.");
    return ezArrayPtr<U>(reinterpret_cast<U*>(GetPtr()), GetCount());
  }

  /// \brief Cast an ArrayPtr to an ArrayPtr to a different, but same size, type
  template <typename U>
  EZ_ALWAYS_INLINE ezArrayPtr<const U> Cast() const
  {
    static_assert(sizeof(T) == sizeof(U), "Can only cast with equivalent element size.");
    return ezArrayPtr<const U>(reinterpret_cast<const U*>(GetPtr()), GetCount());
  }

  /// \brief Index access.
  EZ_FORCE_INLINE const ValueType& operator[](ezUInt32 uiIndex) const // [tested]
  {
    EZ_ASSERT_DEBUG(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<const ValueType*>(GetPtr() + uiIndex);
  }

  /// \brief Index access.
  EZ_FORCE_INLINE ValueType& operator[](ezUInt32 uiIndex) // [tested]
  {
    EZ_ASSERT_DEBUG(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<ValueType*>(GetPtr() + uiIndex);
  }

  /// \brief Compares the two arrays for equality.
  template <typename = typename std::enable_if<std::is_const<T>::value == false>>
  inline bool operator==(const ezArrayPtr<const T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return false;

    if (GetPtr() == other.GetPtr())
      return true;

    return ezMemoryUtils::IsEqual(static_cast<const ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), GetCount());
  }

#if EZ_DISABLED(EZ_USE_CPP20_OPERATORS)
  template <typename = typename std::enable_if<std::is_const<T>::value == false>>
  inline bool operator!=(const ezArrayPtr<const T>& other) const // [tested]
  {
    return !(*this == other);
  }
#endif

  /// \brief Compares the two arrays for equality.
  inline bool operator==(const ezArrayPtr<T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return false;

    if (GetPtr() == other.GetPtr())
      return true;

    return ezMemoryUtils::IsEqual(static_cast<const ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), GetCount());
  }
  EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const ezArrayPtr<T>&);

  /// \brief Compares the two arrays for less.
  inline bool operator<(const ezArrayPtr<const T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return GetCount() < other.GetCount();

    for (ezUInt32 i = 0; i < GetCount(); ++i)
    {
      if (GetPtr()[i] < other.GetPtr()[i])
        return true;

      if (other.GetPtr()[i] < GetPtr()[i])
        return false;
    }

    return false;
  }

  /// \brief Copies the data from \a other into this array. The arrays must have the exact same size.
  inline void CopyFrom(const ezArrayPtr<const T>& other) // [tested]
  {
    EZ_ASSERT_DEV(GetCount() == other.GetCount(), "Count for copy does not match. Target has {0} elements, source {1} elements", GetCount(), other.GetCount());

    ezMemoryUtils::Copy(static_cast<ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), GetCount());
  }

  EZ_ALWAYS_INLINE void Swap(ezArrayPtr<T>& other)
  {
    ::ezMath::Swap(m_pPtr, other.m_pPtr);
    ::ezMath::Swap(m_uiCount, other.m_uiCount);
  }

  /// \brief Checks whether the given value can be found in the array. O(n) complexity.
  EZ_ALWAYS_INLINE bool Contains(const T& value) const // [tested]
  {
    return IndexOf(value) != ezInvalidIndex;
  }

  /// \brief Searches for the first occurrence of the given value and returns its index or ezInvalidIndex if not found.
  inline ezUInt32 IndexOf(const T& value, ezUInt32 uiStartIndex = 0) const // [tested]
  {
    for (ezUInt32 i = uiStartIndex; i < m_uiCount; ++i)
    {
      if (ezMemoryUtils::IsEqual(m_pPtr + i, &value))
        return i;
    }

    return ezInvalidIndex;
  }

  /// \brief Searches for the last occurrence of the given value and returns its index or ezInvalidIndex if not found.
  inline ezUInt32 LastIndexOf(const T& value, ezUInt32 uiStartIndex = ezInvalidIndex) const // [tested]
  {
    for (ezUInt32 i = ::ezMath::Min(uiStartIndex, m_uiCount); i-- > 0;)
    {
      if (ezMemoryUtils::IsEqual(m_pPtr + i, &value))
        return i;
    }
    return ezInvalidIndex;
  }

  using const_iterator = const T*;
  using const_reverse_iterator = const_reverse_pointer_iterator<T>;
  using iterator = T*;
  using reverse_iterator = reverse_pointer_iterator<T>;

private:
  PointerType m_pPtr;
  ezUInt32 m_uiCount;
};

//////////////////////////////////////////////////////////////////////////

using ezByteArrayPtr = ezArrayPtr<ezUInt8>;
using ezConstByteArrayPtr = ezArrayPtr<const ezUInt8>;

//////////////////////////////////////////////////////////////////////////

/// \brief Helper function to create ezArrayPtr from a pointer of some type and a count.
template <typename T>
EZ_ALWAYS_INLINE ezArrayPtr<T> ezMakeArrayPtr(T* pPtr, ezUInt32 uiCount)
{
  return ezArrayPtr<T>(pPtr, uiCount);
}

/// \brief Helper function to create ezArrayPtr from a static array the a size known at compile-time.
template <typename T, ezUInt32 N>
EZ_ALWAYS_INLINE ezArrayPtr<T> ezMakeArrayPtr(T (&staticArray)[N])
{
  return ezArrayPtr<T>(staticArray);
}

/// \brief Helper function to create ezConstByteArrayPtr from a pointer of some type and a count.
template <typename T>
EZ_ALWAYS_INLINE ezConstByteArrayPtr ezMakeByteArrayPtr(const T* pPtr, ezUInt32 uiCount)
{
  return ezConstByteArrayPtr(static_cast<const ezUInt8*>(pPtr), uiCount * sizeof(T));
}

/// \brief Helper function to create ezByteArrayPtr from a pointer of some type and a count.
template <typename T>
EZ_ALWAYS_INLINE ezByteArrayPtr ezMakeByteArrayPtr(T* pPtr, ezUInt32 uiCount)
{
  return ezByteArrayPtr(reinterpret_cast<ezUInt8*>(pPtr), uiCount * sizeof(T));
}

/// \brief Helper function to create ezByteArrayPtr from a void pointer and a count.
EZ_ALWAYS_INLINE ezByteArrayPtr ezMakeByteArrayPtr(void* pPtr, ezUInt32 uiBytes)
{
  return ezByteArrayPtr(reinterpret_cast<ezUInt8*>(pPtr), uiBytes);
}

/// \brief Helper function to create ezConstByteArrayPtr from a const void pointer and a count.
EZ_ALWAYS_INLINE ezConstByteArrayPtr ezMakeByteArrayPtr(const void* pPtr, ezUInt32 uiBytes)
{
  return ezConstByteArrayPtr(static_cast<const ezUInt8*>(pPtr), uiBytes);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
typename ezArrayPtr<T>::iterator begin(ezArrayPtr<T>& ref_container)
{
  return ref_container.GetPtr();
}

template <typename T>
typename ezArrayPtr<T>::const_iterator begin(const ezArrayPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T>
typename ezArrayPtr<T>::const_iterator cbegin(const ezArrayPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T>
typename ezArrayPtr<T>::reverse_iterator rbegin(ezArrayPtr<T>& ref_container)
{
  return typename ezArrayPtr<T>::reverse_iterator(ref_container.GetPtr() + ref_container.GetCount() - 1);
}

template <typename T>
typename ezArrayPtr<T>::const_reverse_iterator rbegin(const ezArrayPtr<T>& container)
{
  return typename ezArrayPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T>
typename ezArrayPtr<T>::const_reverse_iterator crbegin(const ezArrayPtr<T>& container)
{
  return typename ezArrayPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T>
typename ezArrayPtr<T>::iterator end(ezArrayPtr<T>& ref_container)
{
  return ref_container.GetPtr() + ref_container.GetCount();
}

template <typename T>
typename ezArrayPtr<T>::const_iterator end(const ezArrayPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T>
typename ezArrayPtr<T>::const_iterator cend(const ezArrayPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T>
typename ezArrayPtr<T>::reverse_iterator rend(ezArrayPtr<T>& ref_container)
{
  return typename ezArrayPtr<T>::reverse_iterator(ref_container.GetPtr() - 1);
}

template <typename T>
typename ezArrayPtr<T>::const_reverse_iterator rend(const ezArrayPtr<T>& container)
{
  return typename ezArrayPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}

template <typename T>
typename ezArrayPtr<T>::const_reverse_iterator crend(const ezArrayPtr<T>& container)
{
  return typename ezArrayPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}
