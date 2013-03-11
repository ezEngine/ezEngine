#pragma once

#include <Foundation/Memory/MemoryUtils.h>

/// This class encapsulates an array and it's size. It is recommended to use this class instead of plain C arrays.
template <typename T>
class ezArrayPtr
{
public:
  EZ_FORCE_INLINE ezArrayPtr() :
    m_ptr(NULL), m_uiCount(0)
  {
  }

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

  template <ezUInt32 N>
  EZ_FORCE_INLINE ezArrayPtr(T (&staticArray)[N]) :
    m_ptr(staticArray), m_uiCount(N)
  {
  }

  EZ_FORCE_INLINE ezArrayPtr(const ezArrayPtr<T>& other) :
    m_ptr(other.m_ptr), m_uiCount(other.m_uiCount)
  {
  }

  EZ_FORCE_INLINE void operator=(const ezArrayPtr<T>& other)
  {
    m_ptr = other.m_ptr;
    m_uiCount = other.m_uiCount;
  }

  EZ_FORCE_INLINE T* GetPtr() const
  {
    return m_ptr;
  }

  EZ_FORCE_INLINE ezUInt32 GetCount() const
  {
    return m_uiCount;
  }

  /// creates a sub-array from this array
  EZ_FORCE_INLINE ezArrayPtr<T> operator()(ezUInt32 uiStart, ezUInt32 uiEnd)
  {
    EZ_ASSERT(uiStart < uiEnd, "Start has to be smaller than end");
    EZ_ASSERT(uiEnd <= m_uiCount, "End has to be smaller or equal than the count");
    return Array<T>(m_ptr + uiStart, uiEnd - uiStart);
  }

  /// creates a sub-array from this array
  EZ_FORCE_INLINE const ezArrayPtr<T> operator()(ezUInt32 uiStart, ezUInt32 uiEnd) const
  {
    EZ_ASSERT(uiStart < uiEnd, "Start has to be smaller than end");
    EZ_ASSERT(uiEnd <= m_uiCount, "End has to be smaller or equal than the count");
    return Array<T>(m_ptr + uiStart, uiEnd - uiStart);
  }

  EZ_FORCE_INLINE T& operator[](ezUInt32 uiIndex)
  {
    EZ_ASSERT(uiIndex < m_uiCount, "out of bounds access");
    return m_ptr[uiIndex];
  }

  EZ_FORCE_INLINE const T& operator[](ezUInt32 uiIndex) const
  {
    EZ_ASSERT(uiIndex < m_uiCount, "out of bounds access");
    return m_ptr[uiIndex];
  }

  inline bool operator==(const ezArrayPtr<T>& other) const
  {
    if (m_uiCount != other.m_uiCount)
      return false;

    if (m_ptr == other.m_ptr)
      return true;

    return ezMemoryUtils::IsEqual(m_ptr, other.m_ptr, m_uiCount);
  }

  EZ_FORCE_INLINE bool operator!=(const ezArrayPtr<T>& other) const
  {
    return !operator==(other);
  }

  inline void CopyFrom(const ezArrayPtr<T>& other)
  {
    EZ_ASSERT(m_uiCount == other.m_uiCount, "Count for copy does not match. Target has %d elements, source %d elements", 
      m_uiCount, other.m_uiCount);
    ezMemoryUtils::Copy(m_ptr, other.m_ptr, m_uiCount);
  }

  EZ_FORCE_INLINE void Reset()
  {
    m_ptr = NULL;
    m_uiCount = 0;
  }

private:
  T* m_ptr;
  ezUInt32 m_uiCount;
};



