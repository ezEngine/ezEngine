#pragma once

#include <Foundation/Math/Math.h>

/// Chooses either ezUInt32 or ezUInt64 as the storage type for a given type T depending on its size. Required as ezMath::FirstBitLow only supports ezUInt32 or ezUInt64.
/// \tparam T Type for which the storage should be inferred.
template <typename T, typename = std::void_t<>>
struct ezBitIteratorStorage;
template <typename T>
struct ezBitIteratorStorage<T, std::enable_if_t<sizeof(T) <= 4>>
{
  using Type = ezUInt32;
};
template <typename T>
struct ezBitIteratorStorage<T, std::enable_if_t<sizeof(T) >= 5>>
{
  using Type = ezUInt64;
};

/// Configurable bit iterator. Allows for iterating over the bits in an integer, returning either the bit index or value.
/// \tparam DataType The type of data that is being iterated over.
/// \tparam ReturnsIndex If set, returns the index of the bit. Otherwise returns the value of the bit, i.e. EZ_BIT(value).
/// \tparam ReturnType Returned value type of the iterator. Defaults to same as DataType.
/// \tparam StorageType The storage type that the bit operations are performed on (either ezUInt32 or ezUInt64). Auto-computed.
template <typename DataType, bool ReturnsIndex = true, typename ReturnType = DataType, typename StorageType = typename ezBitIteratorStorage<DataType>::Type>
struct ezBitIterator
{
  using iterator_category = std::forward_iterator_tag;
  using value_type = DataType;
  static_assert(sizeof(DataType) <= 8);

  // Invalid iterator (end)
  EZ_ALWAYS_INLINE ezBitIterator() = default;

  // Start iterator.
  EZ_ALWAYS_INLINE explicit ezBitIterator(DataType data)
  {
    m_uiMask = static_cast<StorageType>(data);
  }

  EZ_ALWAYS_INLINE bool IsValid() const
  {
    return m_uiMask != 0;
  }

  EZ_ALWAYS_INLINE ReturnType Value() const
  {
    if constexpr (ReturnsIndex)
    {
      return static_cast<ReturnType>(ezMath::FirstBitLow(m_uiMask));
    }
    else
    {
      return static_cast<ReturnType>(EZ_BIT(ezMath::FirstBitLow(m_uiMask)));
    }
  }

  EZ_ALWAYS_INLINE void Next()
  {
    // Clear the lowest set bit. Why this works: https://www.geeksforgeeks.org/turn-off-the-rightmost-set-bit/
    m_uiMask = m_uiMask & (m_uiMask - 1);
  }

  EZ_ALWAYS_INLINE bool operator==(const ezBitIterator& other) const
  {
    return m_uiMask == other.m_uiMask;
  }

  EZ_ALWAYS_INLINE bool operator!=(const ezBitIterator& other) const
  {
    return m_uiMask != other.m_uiMask;
  }

  EZ_ALWAYS_INLINE ReturnType operator*() const
  {
    return Value();
  }

  EZ_ALWAYS_INLINE void operator++()
  {
    Next();
  }

  StorageType m_uiMask = 0;
};
