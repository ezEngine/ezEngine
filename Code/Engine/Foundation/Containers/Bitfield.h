#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Implementation/BitIterator.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Math/Constants.h>

/// \brief A template interface, that turns any array class into a bitfield.
///
/// This class provides an interface to work with single bits, to store true/false values.
/// The underlying container is configurable, though it must support random access and a 'SetCount' function and it must use elements of type
/// ezUInt32. In most cases a dynamic array should be used. For this case the ezDynamicBitfield typedef is already available. There is also an
/// ezHybridBitfield typedef.
template <class Container>
class ezBitfield
{
public:
  ezBitfield() = default;

  /// \brief Returns the number of bits that this bitfield stores.
  ezUInt32 GetCount() const; // [tested]

  /// \brief Resizes the Bitfield to hold the given number of bits. This version does NOT initialize new bits!
  template <typename = void>                       // Template is used to only conditionally compile this function in when it is actually used.
  void SetCountUninitialized(ezUInt32 uiBitCount); // [tested]

  /// \brief Resizes the Bitfield to hold the given number of bits. If \a bSetNew is true, new bits are set to 1, otherwise they are cleared to 0.
  void SetCount(ezUInt32 uiBitCount, bool bSetNew = false); // [tested]

  /// \brief Returns true, if the bitfield does not store any bits.
  bool IsEmpty() const; // [tested]

  /// \brief Returns true, if the bitfield is not empty and any bit is 1.
  bool IsAnyBitSet(ezUInt32 uiFirstBit = 0, ezUInt32 uiNumBits = 0xFFFFFFFF) const; // [tested]

  /// \brief Returns true, if the bitfield is empty or all bits are set to zero.
  bool IsNoBitSet(ezUInt32 uiFirstBit = 0, ezUInt32 uiNumBits = 0xFFFFFFFF) const; // [tested]

  /// \brief Returns true, if the bitfield is not empty and all bits are set to one.
  bool AreAllBitsSet(ezUInt32 uiFirstBit = 0, ezUInt32 uiNumBits = 0xFFFFFFFF) const; // [tested]

  /// \brief Discards all bits and sets count to zero.
  void Clear(); // [tested]

  /// \brief Sets the given bit to 1.
  void SetBit(ezUInt32 uiBit); // [tested]

  /// \brief Clears the given bit to 0.
  void ClearBit(ezUInt32 uiBit); // [tested]

  /// \brief Flips the given bit to the opposite value.
  void FlipBit(ezUInt32 uiBit); // [tested]

  /// \brief Sets the given bit to 1 or 0 depending on the given value.
  void SetBitValue(ezUInt32 uiBit, bool bValue); // [tested]

  /// \brief Returns true, if the given bit is set to 1.
  bool IsBitSet(ezUInt32 uiBit) const; // [tested]

  /// \brief Clears all bits to 0.
  void ClearAllBits(); // [tested]

  /// \brief Sets all bits to 1.
  void SetAllBits(); // [tested]

  /// \brief Sets the range starting at uiFirstBit up to (and including) uiLastBit to 1.
  void SetBitRange(ezUInt32 uiFirstBit, ezUInt32 uiNumBits); // [tested]

  /// \brief Clears the range starting at uiFirstBit up to (and including) uiLastBit to 0.
  void ClearBitRange(ezUInt32 uiFirstBit, ezUInt32 uiNumBits); // [tested]

  /// \brief Flips the range starting at uiFirstBit up to (and including) uiLastBit.
  void FlipBitRange(ezUInt32 uiFirstBit, ezUInt32 uiNumBits); // [tested]

  /// \brief Swaps two bitfields
  void Swap(ezBitfield<Container>& other); // [tested]
  struct ConstIterator
  {
    using iterator_category = std::forward_iterator_tag;
    using value_type = ezUInt32;
    using sub_iterator = ::ezBitIterator<ezUInt32, true>;

    // Invalid iterator (end)
    EZ_FORCE_INLINE ConstIterator() = default; // [tested]

    // Start iterator.
    explicit ConstIterator(const ezBitfield<Container>& bitfield); // [tested]

    /// \brief Checks whether this iterator points to a valid element.
    bool IsValid() const; // [tested]

    /// \brief Returns the 'value' of the element that this iterator points to.
    ezUInt32 Value() const; // [tested]

    /// \brief Advances the iterator to the next element in the map. The iterator will not be valid anymore, if the end is reached.
    void Next();                                       // [tested]

    bool operator==(const ConstIterator& other) const; // [tested]
    bool operator!=(const ConstIterator& other) const; // [tested]

    /// \brief Returns 'Value()' to enable foreach.
    ezUInt32 operator*() const; // [tested]

    /// \brief Shorthand for 'Next'.
    void operator++(); // [tested]

  private:
    void FindNextChunk(ezUInt32 uiStartChunk);

  private:
    ezUInt32 m_uiChunk = 0;
    sub_iterator m_Iterator;
    const ezBitfield<Container>* m_pBitfield = nullptr;
  };

  /// \brief Returns a constant iterator to the very first set bit.
  /// Note that due to the way iterating through bits is accelerated, changes to the bitfield while iterating through the bits has undefined behaviour.
  ConstIterator GetIterator() const; // [tested]

  /// \brief Returns an invalid iterator. Needed to support range based for loops.
  ConstIterator GetEndIterator() const; // [tested]

private:
  friend struct ConstIterator;

  ezUInt32 GetBitInt(ezUInt32 uiBitIndex) const;
  ezUInt32 GetBitMask(ezUInt32 uiBitIndex) const;

  ezUInt32 m_uiCount = 0;
  Container m_Container;
};

/// \brief This should be the main type of bitfield to use, although other internal container types are possible.
using ezDynamicBitfield = ezBitfield<ezDynamicArray<ezUInt32>>;

/// \brief An ezBitfield that uses a hybrid array as internal container.
template <ezUInt32 BITS>
using ezHybridBitfield = ezBitfield<ezHybridArray<ezUInt32, (BITS + 31) / 32>>;

//////////////////////////////////////////////////////////////////////////
// begin() /end() for range-based for-loop support
template <typename Container>
typename ezBitfield<Container>::ConstIterator begin(const ezBitfield<Container>& container)
{
  return container.GetIterator();
}

template <typename Container>
typename ezBitfield<Container>::ConstIterator cbegin(const ezBitfield<Container>& container)
{
  return container.GetIterator();
}

template <typename Container>
typename ezBitfield<Container>::ConstIterator end(const ezBitfield<Container>& container)
{
  return container.GetEndIterator();
}

template <typename Container>
typename ezBitfield<Container>::ConstIterator cend(const ezBitfield<Container>& container)
{
  return container.GetEndIterator();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template <typename T>
class ezStaticBitfield
{
public:
  using StorageType = T;
  using ConstIterator = ezBitIterator<StorageType, true, ezUInt32>;

  static constexpr ezUInt32 GetStorageTypeBitCount() { return ezMath::NumBits<T>(); }

  /// \brief Initializes the bitfield to all zero.
  ezStaticBitfield();

  static ezStaticBitfield<T> MakeFromMask(StorageType bits);

  /// \brief Returns true, if the bitfield is not zero.
  bool IsAnyBitSet() const; // [tested]

  /// \brief Returns true, if the bitfield is all zero.
  bool IsNoBitSet() const; // [tested]

  /// \brief Returns true, if the bitfield is not empty and all bits are set to one.
  bool AreAllBitsSet() const; // [tested]

  /// \brief Sets the given bit to 1.
  void SetBit(ezUInt32 uiBit); // [tested]

  /// \brief Clears the given bit to 0.
  void ClearBit(ezUInt32 uiBit); // [tested]

  /// \brief Sets the given bit to 1 or 0 depending on the given value.
  void SetBitValue(ezUInt32 uiBit, bool bValue); // [tested]

  /// \brief Returns true, if the given bit is set to 1.
  bool IsBitSet(ezUInt32 uiBit) const; // [tested]

  /// \brief Clears all bits to 0. Same as Clear().
  void ClearAllBits(); // [tested]

  /// \brief Sets all bits to 1.
  void SetAllBits(); // [tested]

  /// \brief Sets the range starting at uiFirstBit up to (and including) uiLastBit to 1.
  void SetBitRange(ezUInt32 uiFirstBit, ezUInt32 uiNumBits); // [tested]

  /// \brief Clears the range starting at uiFirstBit up to (and including) uiLastBit to 0.
  void ClearBitRange(ezUInt32 uiFirstBit, ezUInt32 uiNumBits); // [tested]

  /// \brief Returns the index of the lowest bit that is set. Returns the max index+1 in case no bit is set, at all.
  ezUInt32 GetLowestBitSet() const; // [tested]

  /// \brief Returns the index of the highest bit that is set. Returns the max index+1 in case no bit is set, at all.
  ezUInt32 GetHighestBitSet() const; // [tested]

  /// \brief Returns the count of how many bits are set in total.
  ezUInt32 GetNumBitsSet() const; // [tested]

  /// \brief Returns the raw uint that stores all bits.
  T GetValue() const; // [tested]

  /// \brief Sets the raw uint that stores all bits.
  void SetValue(T value); // [tested]

  /// \brief Swaps two bitfields
  void Swap(ezStaticBitfield<T>& other); // [tested]

  /// \brief Modifies \a this to also contain the bits from \a rhs.
  EZ_ALWAYS_INLINE void operator|=(const ezStaticBitfield<T>& rhs) { m_Storage |= rhs.m_Storage; }

  /// \brief Modifies \a this to only contain the bits that were set in \a this and \a rhs.
  EZ_ALWAYS_INLINE void operator&=(const ezStaticBitfield<T>& rhs) { m_Storage &= rhs.m_Storage; }

  ezResult Serialize(ezStreamWriter& inout_writer) const
  {
    inout_writer.WriteVersion(s_Version);
    inout_writer << m_Storage;
    return EZ_SUCCESS;
  }

  ezResult Deserialize(ezStreamReader& inout_reader)
  {
    /*auto version =*/inout_reader.ReadVersion(s_Version);
    inout_reader >> m_Storage;
    return EZ_SUCCESS;
  }

  /// \brief Returns a constant iterator to the very first set bit.
  /// Note that due to the way iterating through bits is accelerated, changes to the bitfield while iterating through the bits has undefined behaviour.
  ConstIterator GetIterator() const // [tested]
  {
    return ConstIterator(m_Storage);
  };

  /// \brief Returns an invalid iterator. Needed to support range based for loops.
  ConstIterator GetEndIterator() const // [tested]
  {
    return ConstIterator();
  };

private:
  static constexpr ezTypeVersion s_Version = 1;

  ezStaticBitfield(StorageType initValue)
    : m_Storage(initValue)
  {
  }

  template <typename U>
  friend ezStaticBitfield<U> operator|(ezStaticBitfield<U> lhs, ezStaticBitfield<U> rhs);

  template <typename U>
  friend ezStaticBitfield<U> operator&(ezStaticBitfield<U> lhs, ezStaticBitfield<U> rhs);

  template <typename U>
  friend ezStaticBitfield<U> operator^(ezStaticBitfield<U> lhs, ezStaticBitfield<U> rhs);

  template <typename U>
  friend bool operator==(ezStaticBitfield<U> lhs, ezStaticBitfield<U> rhs);

  template <typename U>
  friend bool operator!=(ezStaticBitfield<U> lhs, ezStaticBitfield<U> rhs);

  StorageType m_Storage = 0;
};

template <typename T>
inline ezStaticBitfield<T> operator|(ezStaticBitfield<T> lhs, ezStaticBitfield<T> rhs)
{
  return ezStaticBitfield<T>(lhs.m_Storage | rhs.m_Storage);
}

template <typename T>
inline ezStaticBitfield<T> operator&(ezStaticBitfield<T> lhs, ezStaticBitfield<T> rhs)
{
  return ezStaticBitfield<T>(lhs.m_Storage & rhs.m_Storage);
}

template <typename T>
inline ezStaticBitfield<T> operator^(ezStaticBitfield<T> lhs, ezStaticBitfield<T> rhs)
{
  return ezStaticBitfield<T>(lhs.m_Storage ^ rhs.m_Storage);
}

template <typename T>
inline bool operator==(ezStaticBitfield<T> lhs, ezStaticBitfield<T> rhs)
{
  return lhs.m_Storage == rhs.m_Storage;
}

template <typename T>
inline bool operator!=(ezStaticBitfield<T> lhs, ezStaticBitfield<T> rhs)
{
  return lhs.m_Storage != rhs.m_Storage;
}

//////////////////////////////////////////////////////////////////////////
// begin() /end() for range-based for-loop support
template <typename Container>
typename ezStaticBitfield<Container>::ConstIterator begin(const ezStaticBitfield<Container>& container)
{
  return container.GetIterator();
}

template <typename Container>
typename ezStaticBitfield<Container>::ConstIterator cbegin(const ezStaticBitfield<Container>& container)
{
  return container.GetIterator();
}

template <typename Container>
typename ezStaticBitfield<Container>::ConstIterator end(const ezStaticBitfield<Container>& container)
{
  return container.GetEndIterator();
}

template <typename Container>
typename ezStaticBitfield<Container>::ConstIterator cend(const ezStaticBitfield<Container>& container)
{
  return container.GetEndIterator();
}

using ezStaticBitfield8 = ezStaticBitfield<ezUInt8>;
using ezStaticBitfield16 = ezStaticBitfield<ezUInt16>;
using ezStaticBitfield32 = ezStaticBitfield<ezUInt32>;
using ezStaticBitfield64 = ezStaticBitfield<ezUInt64>;

#include <Foundation/Containers/Implementation/Bitfield_inl.h>
