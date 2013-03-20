#pragma once

#include <Foundation/Memory/MemoryUtils.h>

/// \brief This class encapsulates an array and it's size. It is recommended to use this class instead of plain C arrays.
/// No data is deallocated at destruction, the ezArrayPtr only allows for easier access.
template <typename T>
class ezArrayPtr
{
public:

  /// \brief Initializes the ezArrayPtr to be empty.
  EZ_FORCE_INLINE ezArrayPtr() :
    m_ptr(NULL), m_uiCount(0)
  {
  }

  /// \brief Initializes the ezArrayPtr with the given pointer and number of elements. No memory is allocated or copied.
  inline ezArrayPtr(T* ptr, ezUInt32 uiCount)
  {
    if (ptr != NULL && uiCount != 0)
    {        
      m_ptr = ptr;
      m_uiCount = uiCount;
    }
    else
    {
      m_ptr = NULL;
      m_uiCount = 0;
    }
  }

  /// \brief Initializes the ezArrayPtr to encapsulate the given array.
  template <ezUInt32 N>
  EZ_FORCE_INLINE ezArrayPtr(T (&staticArray)[N]) : m_ptr(staticArray), m_uiCount(N)
  {
  }

  /// \brief Initializes the ezArrayPtr to be a copy of \a other. No memory is allocated or copied.
  EZ_FORCE_INLINE ezArrayPtr(const ezArrayPtr<T>& other) : m_ptr(other.m_ptr), m_uiCount(other.m_uiCount)
  {
  }

  /// \brief Copies the pointer and size of /a other. Does not allocate any data.
  EZ_FORCE_INLINE void operator=(const ezArrayPtr<T>& other)
  {
    m_ptr = other.m_ptr;
    m_uiCount = other.m_uiCount;
  }

  /// \brief Returns the pointer to the array.
  EZ_FORCE_INLINE T* GetPtr() const
  {
    return m_ptr;
  }

  /// \brief Returns the number of elements in the array.
  EZ_FORCE_INLINE ezUInt32 GetCount() const
  {
    return m_uiCount;
  }

  /// \brief Creates a sub-array from this array.
  EZ_FORCE_INLINE ezArrayPtr<T> operator()(ezUInt32 uiStart, ezUInt32 uiEnd)
  {
    EZ_ASSERT(uiStart < uiEnd, "Start has to be smaller than end");
    EZ_ASSERT(uiEnd <= m_uiCount, "End has to be smaller or equal than the count");
    return ezArrayPtr<T>(m_ptr + uiStart, uiEnd - uiStart);
  }

  /// \brief Creates a sub-array from this array.
  EZ_FORCE_INLINE const ezArrayPtr<T> operator()(ezUInt32 uiStart, ezUInt32 uiEnd) const
  {
    EZ_ASSERT(uiStart < uiEnd, "Start has to be smaller than end");
    EZ_ASSERT(uiEnd <= m_uiCount, "End has to be smaller or equal than the count");
    return ezArrayPtr<T>(m_ptr + uiStart, uiEnd - uiStart);
  }

  /// \brief Index access.
  EZ_FORCE_INLINE T& operator[](ezUInt32 uiIndex)
  {
    EZ_ASSERT(uiIndex < m_uiCount, "out of bounds access");
    return m_ptr[uiIndex];
  }

  /// \brief Index access.
  EZ_FORCE_INLINE const T& operator[](ezUInt32 uiIndex) const
  {
    EZ_ASSERT(uiIndex < m_uiCount, "out of bounds access");
    return m_ptr[uiIndex];
  }

  /// \brief Compares the two arrays for equality.
  inline bool operator==(const ezArrayPtr<T>& other) const
  {
    if (m_uiCount != other.m_uiCount)
      return false;

    if (m_ptr == other.m_ptr)
      return true;

    return ezMemoryUtils::IsEqual(m_ptr, other.m_ptr, m_uiCount);
  }

  /// \brief Compares the two arrays for inequality.
  EZ_FORCE_INLINE bool operator!=(const ezArrayPtr<T>& other) const
  {
    return !operator==(other);
  }

  /// \brief Copies the data from \a other into this array. The arrays must have the exact same size.
  inline void CopyFrom(const ezArrayPtr<T>& other)
  {
    EZ_ASSERT(m_uiCount == other.m_uiCount, "Count for copy does not match. Target has %d elements, source %d elements", m_uiCount, other.m_uiCount);

    ezMemoryUtils::Copy(m_ptr, other.m_ptr, m_uiCount);
  }

  /// \brief Resets the ezArray to be empty. 
  EZ_FORCE_INLINE void Reset()
  {
    m_ptr = NULL;
    m_uiCount = 0;
  }

private:
  T* m_ptr;
  ezUInt32 m_uiCount;
};



