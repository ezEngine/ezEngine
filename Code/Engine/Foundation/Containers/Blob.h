
#pragma once

#include <Foundation/Types/ArrayPtr.h>

/// \brief This class encapsulates a blob's storage and it's size. It is recommended to use this class instead of directly working on the void* of the blob.
///
/// No data is deallocated at destruction, the ezBlobPtr only allows for easier access.
template <typename T>
class ezBlobPtr
{
public:
  EZ_DECLARE_POD_TYPE();

  static_assert(!std::is_same_v<T, void>, "ezBlobPtr<void> is not allowed (anymore)");
  static_assert(!std::is_same_v<T, const void>, "ezBlobPtr<void> is not allowed (anymore)");

  using ByteType = typename ezArrayPtrDetail::ByteTypeHelper<T>::type;
  using ValueType = T;
  using PointerType = T*;

  /// \brief Initializes the ezBlobPtr to be empty.
  EZ_ALWAYS_INLINE ezBlobPtr()
    : m_ptr(nullptr)
    , m_uiCount(0u)
  {
  }

  /// \brief Initializes the ezBlobPtr with the given pointer and number of elements. No memory is allocated or copied.
  template <typename U>
  inline ezBlobPtr(U* ptr, ezUInt64 uiCount)
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

  /// \brief Initializes the ezBlobPtr to encapsulate the given array.
  template <size_t N>
  EZ_ALWAYS_INLINE ezBlobPtr(ValueType (&staticArray)[N])
    : m_ptr(staticArray)
    , m_uiCount(static_cast<ezUInt64>(N))
  {
  }

  /// \brief Initializes the ezBlobPtr to be a copy of \a other. No memory is allocated or copied.
  EZ_ALWAYS_INLINE ezBlobPtr(const ezBlobPtr<T>& other)
    : m_ptr(other.m_ptr)
    , m_uiCount(other.m_uiCount)
  {
  }

  /// \brief Convert to const version.
  operator ezBlobPtr<const T>() const { return ezBlobPtr<const T>(static_cast<const T*>(GetPtr()), GetCount()); }

  /// \brief Copies the pointer and size of /a other. Does not allocate any data.
  EZ_ALWAYS_INLINE void operator=(const ezBlobPtr<T>& other)
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

  EZ_ALWAYS_INLINE void operator=(std::nullptr_t)
  {
    m_ptr = nullptr;
    m_uiCount = 0;
  }

  /// \brief Returns the pointer to the array.
  EZ_ALWAYS_INLINE const PointerType GetPtr() const
  {
    return m_ptr;
  }

  /// \brief Returns the pointer to the array.
  EZ_ALWAYS_INLINE PointerType GetPtr()
  {
    return m_ptr;
  }

  /// \brief Returns the pointer behind the last element of the array
  EZ_ALWAYS_INLINE PointerType GetEndPtr() { return m_ptr + m_uiCount; }

  /// \brief Returns the pointer behind the last element of the array
  EZ_ALWAYS_INLINE const PointerType GetEndPtr() const { return m_ptr + m_uiCount; }

  /// \brief Returns whether the array is empty.
  EZ_ALWAYS_INLINE bool IsEmpty() const
  {
    return GetCount() == 0;
  }

  /// \brief Returns the number of elements in the array.
  EZ_ALWAYS_INLINE ezUInt64 GetCount() const
  {
    return m_uiCount;
  }

  /// \brief Creates a sub-array from this array.
  EZ_FORCE_INLINE ezBlobPtr<T> GetSubArray(ezUInt64 uiStart, ezUInt64 uiCount) const // [tested]
  {
    EZ_ASSERT_DEV(uiStart + uiCount <= GetCount(), "uiStart+uiCount ({0}) has to be smaller or equal than the count ({1}).",
      uiStart + uiCount, GetCount());
    return ezBlobPtr<T>(GetPtr() + uiStart, uiCount);
  }

  /// \brief Creates a sub-array from this array.
  /// \note \code ap.GetSubArray(i) \endcode is equivalent to \code ap.GetSubArray(i, ap.GetCount() - i) \endcode.
  EZ_FORCE_INLINE ezBlobPtr<T> GetSubArray(ezUInt64 uiStart) const // [tested]
  {
    EZ_ASSERT_DEV(uiStart <= GetCount(), "uiStart ({0}) has to be smaller or equal than the count ({1}).", uiStart, GetCount());
    return ezBlobPtr<T>(GetPtr() + uiStart, GetCount() - uiStart);
  }

  /// \brief Reinterprets this array as a byte array.
  EZ_ALWAYS_INLINE ezBlobPtr<const ByteType> ToByteBlob() const
  {
    return ezBlobPtr<const ByteType>(reinterpret_cast<const ByteType*>(GetPtr()), GetCount() * sizeof(T));
  }

  /// \brief Reinterprets this array as a byte array.
  EZ_ALWAYS_INLINE ezBlobPtr<ByteType> ToByteBlob()
  {
    return ezBlobPtr<ByteType>(reinterpret_cast<ByteType*>(GetPtr()), GetCount() * sizeof(T));
  }

  /// \brief Cast an BlobPtr to an BlobPtr to a different, but same size, type
  template <typename U>
  EZ_ALWAYS_INLINE ezBlobPtr<U> Cast()
  {
    static_assert(sizeof(T) == sizeof(U), "Can only cast with equivalent element size.");
    return ezBlobPtr<U>(reinterpret_cast<U*>(GetPtr()), GetCount());
  }

  /// \brief Cast an BlobPtr to an BlobPtr to a different, but same size, type
  template <typename U>
  EZ_ALWAYS_INLINE ezBlobPtr<const U> Cast() const
  {
    static_assert(sizeof(T) == sizeof(U), "Can only cast with equivalent element size.");
    return ezBlobPtr<const U>(reinterpret_cast<const U*>(GetPtr()), GetCount());
  }

  /// \brief Index access.
  EZ_FORCE_INLINE const ValueType& operator[](ezUInt64 uiIndex) const // [tested]
  {
    EZ_ASSERT_DEV(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<const ValueType*>(GetPtr() + uiIndex);
  }

  /// \brief Index access.
  EZ_FORCE_INLINE ValueType& operator[](ezUInt64 uiIndex) // [tested]
  {
    EZ_ASSERT_DEV(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<ValueType*>(GetPtr() + uiIndex);
  }

  /// \brief Compares the two arrays for equality.
  inline bool operator==(const ezBlobPtr<const T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return false;

    if (GetPtr() == other.GetPtr())
      return true;

    return ezMemoryUtils::IsEqual(static_cast<const ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), GetCount());
  }

  /// \brief Compares the two arrays for inequality.
  EZ_ALWAYS_INLINE bool operator!=(const ezBlobPtr<const T>& other) const // [tested]
  {
    return !(*this == other);
  }

  /// \brief Copies the data from \a other into this array. The arrays must have the exact same size.
  inline void CopyFrom(const ezBlobPtr<const T>& other) // [tested]
  {
    EZ_ASSERT_DEV(GetCount() == other.GetCount(), "Count for copy does not match. Target has {0} elements, source {1} elements", GetCount(),
      other.GetCount());

    ezMemoryUtils::Copy(static_cast<ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), GetCount());
  }

  EZ_ALWAYS_INLINE void Swap(ezBlobPtr<T>& other)
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
  ezUInt64 m_uiCount;
};

//////////////////////////////////////////////////////////////////////////

using ezByteBlobPtr = ezBlobPtr<ezUInt8>;
using ezConstByteBlobPtr = ezBlobPtr<const ezUInt8>;

//////////////////////////////////////////////////////////////////////////

/// \brief Helper function to create ezBlobPtr from a pointer of some type and a count.
template <typename T>
EZ_ALWAYS_INLINE ezBlobPtr<T> ezMakeBlobPtr(T* ptr, ezUInt64 uiCount)
{
  return ezBlobPtr<T>(ptr, uiCount);
}

/// \brief Helper function to create ezBlobPtr from a static array the a size known at compile-time.
template <typename T, ezUInt64 N>
EZ_ALWAYS_INLINE ezBlobPtr<T> ezMakeBlobPtr(T (&staticArray)[N])
{
  return ezBlobPtr<T>(staticArray);
}

/// \brief Helper function to create ezConstByteBlobPtr from a pointer of some type and a count.
template <typename T>
EZ_ALWAYS_INLINE ezConstByteBlobPtr ezMakeByteBlobPtr(const T* ptr, ezUInt32 uiCount)
{
  return ezConstByteBlobPtr(static_cast<const ezUInt8*>(ptr), uiCount * sizeof(T));
}

/// \brief Helper function to create ezByteBlobPtr from a pointer of some type and a count.
template <typename T>
EZ_ALWAYS_INLINE ezByteBlobPtr ezMakeByteBlobPtr(T* ptr, ezUInt32 uiCount)
{
  return ezByteBlobPtr(reinterpret_cast<ezUInt8*>(ptr), uiCount * sizeof(T));
}

/// \brief Helper function to create ezByteBlobPtr from a void pointer and a count.
EZ_ALWAYS_INLINE ezByteBlobPtr ezMakeByteBlobPtr(void* ptr, ezUInt32 uiBytes)
{
  return ezByteBlobPtr(reinterpret_cast<ezUInt8*>(ptr), uiBytes);
}

/// \brief Helper function to create ezConstByteBlobPtr from a const void pointer and a count.
EZ_ALWAYS_INLINE ezConstByteBlobPtr ezMakeByteBlobPtr(const void* ptr, ezUInt32 uiBytes)
{
  return ezConstByteBlobPtr(static_cast<const ezUInt8*>(ptr), uiBytes);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
typename ezBlobPtr<T>::iterator begin(ezBlobPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T>
typename ezBlobPtr<T>::const_iterator begin(const ezBlobPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T>
typename ezBlobPtr<T>::const_iterator cbegin(const ezBlobPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T>
typename ezBlobPtr<T>::reverse_iterator rbegin(ezBlobPtr<T>& container)
{
  return typename ezBlobPtr<T>::reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T>
typename ezBlobPtr<T>::const_reverse_iterator rbegin(const ezBlobPtr<T>& container)
{
  return typename ezBlobPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T>
typename ezBlobPtr<T>::const_reverse_iterator crbegin(const ezBlobPtr<T>& container)
{
  return typename ezBlobPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T>
typename ezBlobPtr<T>::iterator end(ezBlobPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T>
typename ezBlobPtr<T>::const_iterator end(const ezBlobPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T>
typename ezBlobPtr<T>::const_iterator cend(const ezBlobPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T>
typename ezBlobPtr<T>::reverse_iterator rend(ezBlobPtr<T>& container)
{
  return typename ezBlobPtr<T>::reverse_iterator(container.GetPtr() - 1);
}

template <typename T>
typename ezBlobPtr<T>::const_reverse_iterator rend(const ezBlobPtr<T>& container)
{
  return typename ezBlobPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}

template <typename T>
typename ezBlobPtr<T>::const_reverse_iterator crend(const ezBlobPtr<T>& container)
{
  return typename ezBlobPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}

/// \brief ezBlob allows to store simple binary data larger than 4GB.
/// This storage class is used by ezImage to allow processing of large textures for example.
/// In the current implementation the start of the allocated memory is guaranteed to be 64 byte aligned.
class EZ_FOUNDATION_DLL ezBlob
{
public:
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

  /// \brief Default constructor. Does not allocate any memory.
  ezBlob();

  /// \brief Move constructor. Moves the storage pointer from the other blob to this blob.
  ezBlob(ezBlob&& other);

  /// \brief Move assignment. Moves the storage pointer from the other blob to this blob.
  void operator=(ezBlob&& rhs);

  /// \brief Default destructor. Will call Clear() to deallocate the memory.
  ~ezBlob();

  /// \brief Sets the blob to the content of pSource.
  /// This will allocate the necessary memory if needed and then copy uiSize bytes from pSource.
  void SetFrom(void* pSource, ezUInt64 uiSize);

  /// \brief Deallocates the memory allocated by this instance.
  void Clear();

  /// \brief Allocates uiCount bytes for storage in this object. The bytes will have undefined content.
  void SetCountUninitialized(ezUInt64 uiCount);

  /// \brief Convenience method to clear the content of the blob to all 0 bytes.
  void ZeroFill();

  /// \brief Returns a blob pointer to the blob data, or an empty blob pointer if the blob is empty.
  template <typename T>
  ezBlobPtr<T> GetBlobPtr()
  {
    return ezBlobPtr<T>(static_cast<T*>(m_pStorage), m_uiSize);
  }

  /// \brief Returns a blob pointer to the blob data, or an empty blob pointer if the blob is empty.
  template <typename T>
  ezBlobPtr<const T> GetBlobPtr() const
  {
    return ezBlobPtr<const T>(static_cast<T*>(m_pStorage), m_uiSize);
  }

  /// \brief Returns a blob pointer to the blob data, or an empty blob pointer if the blob is empty.
  ezByteBlobPtr GetByteBlobPtr()
  {
    return ezByteBlobPtr(reinterpret_cast<ezUInt8*>(m_pStorage), m_uiSize);
  }

  /// \brief Returns a blob pointer to the blob data, or an empty blob pointer if the blob is empty.
  ezConstByteBlobPtr GetByteBlobPtr() const
  {
    return ezConstByteBlobPtr(reinterpret_cast<const ezUInt8*>(m_pStorage), m_uiSize);
  }

private:
  void* m_pStorage = nullptr;
  ezUInt64 m_uiSize = 0;
};
