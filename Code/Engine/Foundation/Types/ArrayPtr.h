#pragma once

#include <Foundation/Memory/MemoryUtils.h>

#include <Foundation/Containers/Implementation/ArrayIterator.h>

// This #include is quite vital, do not remove it!
#include <Foundation/Strings/FormatString.h>

namespace ezArrayPtrDetail
{
  template <typename U>
  struct ByteTypeHelper
  {
    typedef ezUInt8 type;
  };

  template <typename U>
  struct ByteTypeHelper<const U>
  {
    typedef const ezUInt8 type;
  };

  /// \brief Helper to allow simple pointer arithmetic in case the ArrayPtr's type is
  ///    is void or const void.
  template <typename U>
  struct VoidPtrHelper
  {
  public:
    EZ_DECLARE_POD_TYPE();

    VoidPtrHelper() {}
    VoidPtrHelper(U* p)
        : m_ptr(p)
    {
    }
    operator U*() const { return m_ptr; }
    template <typename V, typename = std::enable_if_t<sizeof(V) == 1 && !std::is_const<U>::value>>
    explicit operator V*() const
    {
      return static_cast<V*>(m_ptr);
    }
    template <typename V, typename = std::enable_if_t<sizeof(V) == 1>>
    explicit operator const V*() const
    {
      return static_cast<const V*>(m_ptr);
    }
    U* operator+(ptrdiff_t off) const { return (U*)((ptrdiff_t(m_ptr) + off)); }
    U* operator-(ptrdiff_t off) const { return (U*)((ptrdiff_t(m_ptr) - off)); }
    ptrdiff_t operator-(const void* other) const { return ptrdiff_t(m_ptr) - ptrdiff_t(other); }

  private:
    U* m_ptr;
  };

  template <typename U>
  struct VoidTypeHelper
  {
    typedef U valueType;
    typedef U* pointerType;
  };

  template<>
  struct VoidTypeHelper<void>
  {
    typedef ezUInt8 valueType;
    typedef VoidPtrHelper<void> pointerType;
  };

  template <>
  struct VoidTypeHelper<const void>
  {
    typedef const ezUInt8 valueType;
    typedef VoidPtrHelper<const void> pointerType;
  };
} // namespace ezArrayPtrDetail

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

public:
  typedef typename ezArrayPtrDetail::ByteTypeHelper<T>::type ByteType;
  typedef typename ezArrayPtrDetail::VoidTypeHelper<T>::valueType ValueType;
  typedef typename ezArrayPtrDetail::VoidTypeHelper<T>::pointerType PointerType;

  /// \brief Initializes the ezArrayPtr to be empty.
  EZ_ALWAYS_INLINE ezArrayPtr() // [tested]
      : m_ptr(nullptr)
      , m_uiCount(0u)
  {
  }

  /// \brief Initializes the ezArrayPtr with the given pointer and number of elements. No memory is allocated or copied.
  /// \note For ArrayPtr<void> and ArrayPtr<const void>, this constructor is only available if ptr is of
  ///    type void*, const void*, or any T* with sizeof(T) == 1
  template <typename U, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<T>, void>::value ||
                                                    sizeof(typename ezArrayPtrDetail::VoidTypeHelper<U>::valueType) == 1>>
  inline ezArrayPtr(U* ptr, ezUInt32 uiCount) // [tested]
      : m_ptr(ptr)
      , m_uiCount(uiCount)
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
  EZ_ALWAYS_INLINE ezArrayPtr(ValueType (&staticArray)[N]) // [tested]
      : m_ptr(staticArray)
      , m_uiCount(static_cast<ezUInt32>(N))
  {
  }

  /// \brief Initializes the ezArrayPtr to be a copy of \a other. No memory is allocated or copied.
  EZ_ALWAYS_INLINE ezArrayPtr(const ezArrayPtr<T>& other) // [tested]
      : m_ptr(other.m_ptr)
      , m_uiCount(other.m_uiCount)
  {
  }

  /// \brief Convert to const version.
  operator ezArrayPtr<const T>() const { return ezArrayPtr<const T>(static_cast<const T*>(GetPtr()), GetCount()); } // [tested]

  /// \brief Copies the pointer and size of /a other. Does not allocate any data.
  EZ_ALWAYS_INLINE void operator=(const ezArrayPtr<T>& other) // [tested]
  {
    m_ptr = other.m_ptr;
    m_uiCount = other.m_uiCount;
  }

  /// \brief Clears the array
  EZ_ALWAYS_INLINE void Clear()
  {
    m_ptr = nullptr;
    m_uiCount = 0;
  }

  EZ_ALWAYS_INLINE void operator=(std::nullptr_t) // [tested]
  {
    m_ptr = nullptr;
    m_uiCount = 0;
  }

  /// \brief Returns the pointer to the array.
  EZ_ALWAYS_INLINE const PointerType GetPtr() const // [tested]
  {
    return m_ptr;
  }

  /// \brief Returns the pointer to the array.
  EZ_ALWAYS_INLINE PointerType GetPtr() // [tested]
  {
    return m_ptr;
  }

  /// \brief Returns the pointer behind the last element of the array
  EZ_ALWAYS_INLINE PointerType GetEndPtr() { return m_ptr + m_uiCount; }

  /// \brief Returns the pointer behind the last element of the array
  EZ_ALWAYS_INLINE const PointerType GetEndPtr() const { return m_ptr + m_uiCount; }

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
  EZ_FORCE_INLINE ezArrayPtr<const T> GetSubArray(ezUInt32 uiStart, ezUInt32 uiCount) const // [tested]
  {
    EZ_ASSERT_DEV(uiStart + uiCount <= GetCount(), "uiStart+uiCount ({0}) has to be smaller or equal than the count ({1}).",
                  uiStart + uiCount, GetCount());
    return ezArrayPtr<const T>(GetPtr() + uiStart, uiCount);
  }

  /// \brief Creates a sub-array from this array.
  EZ_FORCE_INLINE ezArrayPtr<T> GetSubArray(ezUInt32 uiStart, ezUInt32 uiCount) // [tested]
  {
    EZ_ASSERT_DEV(uiStart + uiCount <= GetCount(), "uiStart+uiCount ({0}) has to be smaller or equal than the count ({1}).",
                  uiStart + uiCount, GetCount());
    return ezArrayPtr<T>(GetPtr() + uiStart, uiCount);
  }

  /// \brief Creates a sub-array from this array.
  /// \note \code ap.GetSubArray(i) \endcode is equivalent to \code ap.GetSubArray(i, ap.GetCount() - i) \endcode.
  EZ_FORCE_INLINE ezArrayPtr<const T> GetSubArray(ezUInt32 uiStart) const // [tested]
  {
    EZ_ASSERT_DEV(uiStart <= GetCount(), "uiStart ({0}) has to be smaller or equal than the count ({1}).", uiStart, GetCount());
    return ezArrayPtr<const T>(GetPtr() + uiStart, GetCount() - uiStart);
  }

  /// \brief Creates a sub-array from this array.
  /// \note \code ap.GetSubArray(i) \endcode is equivalent to \code ap.GetSubArray(i, ap.GetCount() - i) \endcode.
  EZ_FORCE_INLINE ezArrayPtr<T> GetSubArray(ezUInt32 uiStart) // [tested]
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
  EZ_ALWAYS_INLINE ezArrayPtr<ByteType> ToByteArray()
  {
    return ezArrayPtr<ByteType>(reinterpret_cast<ByteType*>(GetPtr()), GetCount() * sizeof(T));
  }

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
    EZ_ASSERT_DEV(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<const ValueType*>(GetPtr() + uiIndex);
  }

  /// \brief Index access.
  EZ_FORCE_INLINE ValueType& operator[](ezUInt32 uiIndex) // [tested]
  {
    EZ_ASSERT_DEV(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<ValueType*>(GetPtr() + uiIndex);
  }

  /// \brief Compares the two arrays for equality.
  inline bool operator==(const ezArrayPtr<const T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return false;

    if (GetPtr() == other.GetPtr())
      return true;

    return ezMemoryUtils::IsEqual(static_cast<const ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), GetCount());
  }

  /// \brief Compares the two arrays for inequality.
  EZ_ALWAYS_INLINE bool operator!=(const ezArrayPtr<const T>& other) const // [tested]
  {
    return !(*this == other);
  }

  /// \brief Copies the data from \a other into this array. The arrays must have the exact same size.
  inline void CopyFrom(const ezArrayPtr<const T>& other) // [tested]
  {
    EZ_ASSERT_DEV(GetCount() == other.GetCount(), "Count for copy does not match. Target has {0} elements, source {1} elements", GetCount(),
                  other.GetCount());

    ezMemoryUtils::Copy(static_cast<ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), GetCount());
  }

  EZ_ALWAYS_INLINE void Swap(ezArrayPtr<T>& other)
  {
    ezMath::Swap(m_ptr, other.m_ptr);
    ezMath::Swap(m_uiCount, other.m_uiCount);
  }

  typedef const T* const_iterator;
  typedef const_reverse_pointer_iterator<T> const_reverse_iterator;
  typedef T* iterator;
  typedef reverse_pointer_iterator<T> reverse_iterator;

private:
  PointerType m_ptr;
  ezUInt32 m_uiCount;
};

/// \brief Helper function to create ezArrayPtr from a pointer of some type and a count.
template <typename T>
EZ_ALWAYS_INLINE ezArrayPtr<T> ezMakeArrayPtr(T* ptr, ezUInt32 uiCount)
{
  return ezArrayPtr<T>(ptr, uiCount);
}

/// \brief Helper function to create ezArrayPtr from a static array the a size known at compile-time.
template <typename T, ezUInt32 N>
EZ_ALWAYS_INLINE ezArrayPtr<T> ezMakeArrayPtr(T (&staticArray)[N])
{
  return ezArrayPtr<T>(staticArray);
}

template <typename T, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<T>, void>::value>>
typename ezArrayPtr<T>::iterator begin(ezArrayPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<T>, void>::value>>
typename ezArrayPtr<T>::const_iterator begin(const ezArrayPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<T>, void>::value>>
typename ezArrayPtr<T>::const_iterator cbegin(const ezArrayPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<T>, void>::value>>
typename ezArrayPtr<T>::reverse_iterator rbegin(ezArrayPtr<T>& container)
{
  return typename ezArrayPtr<T>::reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<T>, void>::value>>
typename ezArrayPtr<T>::const_reverse_iterator rbegin(const ezArrayPtr<T>& container)
{
  return typename ezArrayPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<T>, void>::value>>
typename ezArrayPtr<T>::const_reverse_iterator crbegin(const ezArrayPtr<T>& container)
{
  return typename ezArrayPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<T>, void>::value>>
typename ezArrayPtr<T>::iterator end(ezArrayPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<T>, void>::value>>
typename ezArrayPtr<T>::const_iterator end(const ezArrayPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<T>, void>::value>>
typename ezArrayPtr<T>::const_iterator cend(const ezArrayPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<T>, void>::value>>
typename ezArrayPtr<T>::reverse_iterator rend(ezArrayPtr<T>& container)
{
  return typename ezArrayPtr<T>::reverse_iterator(container.GetPtr() - 1);
}

template <typename T, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<T>, void>::value>>
typename ezArrayPtr<T>::const_reverse_iterator rend(const ezArrayPtr<T>& container)
{
  return typename ezArrayPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}

template <typename T, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<T>, void>::value>>
typename ezArrayPtr<T>::const_reverse_iterator crend(const ezArrayPtr<T>& container)
{
  return typename ezArrayPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}

