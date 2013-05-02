#pragma once

#include <Foundation/Basics.h>

/// \brief An id class that holds an id combined of an index and a counter
template <typename T, ezUInt32 IndexBits>
class ezId
{
public:
  typedef T IndexType;
  enum 
  { 
    MAX_OBJECTS = (1U << IndexBits), 
    INDEX_MASK = MAX_OBJECTS - 1,
    INDEX_BITS = IndexBits,
    COUNTER_BITS = (sizeof(T) * 8) - IndexBits,
    COUNTER_MASK = (1U << COUNTER_BITS) - 1
  };

  EZ_DECLARE_POD_TYPE();

  EZ_FORCE_INLINE ezId()
  {
    m_data = INDEX_MASK;
  }

  EZ_FORCE_INLINE ezId(T index, T counter)
  {
    m_data = (index & INDEX_MASK) | (counter & COUNTER_MASK) << INDEX_BITS;
  }

  EZ_FORCE_INLINE bool operator==(const ezId<T, IndexBits>& other) const
  {
    return m_data == other.m_data;
  }

  EZ_FORCE_INLINE bool operator!=(const ezId<T, IndexBits>& other) const
  {
    return m_data != other.m_data;
  }

  EZ_FORCE_INLINE T GetIndex() const
  {
    return m_data & INDEX_MASK;
  }

  EZ_FORCE_INLINE void SetIndex(T index)
  {
    m_data = (index & INDEX_MASK) | m_data & (COUNTER_MASK << INDEX_BITS);
  }

  EZ_FORCE_INLINE T GetCounter() const
  {
    return (m_data >> INDEX_BITS) & COUNTER_MASK;
  }

  EZ_FORCE_INLINE void SetCounter(T innerId)
  {
    m_data = (m_data & INDEX_MASK) | (innerId & COUNTER_MASK) << INDEX_BITS;
  }

private:
  T m_data;
};

typedef ezId<ezUInt32, 16> ezId16;
typedef ezId<ezUInt32, 24> ezId24;
typedef ezId<ezUInt64, 32> ezId32;
typedef ezId<ezUInt64, 48> ezId48;
