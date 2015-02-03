#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>

/// \brief A template interface, that turns any array class into a bitfield.
///
/// This class provides an interface to work with single bits, to store true/false values.
/// The underlying container is configurable, though it must support random access and a 'SetCount' function and it must use elements of type ezUInt32.
/// In most cases a dynamic array should be used. For this case the ezDynamicBitfield typedef is already available.
/// It is also useful to use an ezHybridArray, though there is not ezHybridBitfield typedef, due to language restrictions.
template<class Container>
class ezBitfield
{
public:
  ezBitfield();

  /// \brief Returns the number of bits that this bitfield stores.
  EZ_FORCE_INLINE ezUInt32 GetCount(); // [tested]

  /// \brief Resizes the Bitfield to hold the given number of bits. This version does NOT initialize new bits!
  void SetCount(ezUInt32 uiBitCount); // [tested]

  /// \brief Resizes the Bitfield to hold the given number of bits. If \a bSetNew is true, new bits are set to 1, otherwise they are cleared to 0.
  void SetCount(ezUInt32 uiBitCount, bool bSetNew); // [tested]

  /// \brief Returns true, if the bitfield does not store any bits.
  EZ_FORCE_INLINE bool IsEmpty() const; // [tested]

  /// \brief Discards all bits and sets count to zero.
  void Clear(); // [tested]

  /// \brief Sets the given bit to 1.
  EZ_FORCE_INLINE void SetBit(ezUInt32 uiBit); // [tested]

  /// \brief Clears the given bit to 0.
  EZ_FORCE_INLINE void ClearBit(ezUInt32 uiBit); // [tested]

  /// \brief Returns true, if the given bit is set to 1.
  EZ_FORCE_INLINE bool IsSet(ezUInt32 uiBit) const; // [tested]

  /// \brief Clears all bits to 0.
  void ClearAllBits(); // [tested]

  /// \brief Sets all bits to 1.
  void SetAllBits(); // [tested]

  /// \brief Sets the range starting at uiFirstBit up to (and including) uiLastBit to 1.
  void SetRange(ezUInt32 uiFirstBit, ezUInt32 uiLastBit); // [tested]

  /// \brief Clears the range starting at uiFirstBit up to (and including) uiLastBit to 0.
  void ClearRange(ezUInt32 uiFirstBit, ezUInt32 uiLastBit); // [tested]

private:
  EZ_FORCE_INLINE ezUInt32 GetBitInt(ezUInt32 uiBitIndex) const;
  EZ_FORCE_INLINE ezUInt32 GetBitMask(ezUInt32 uiBitIndex) const;

  ezUInt32 m_uiCount;
  Container m_Container;
};

/// \brief This should be the main type of bitfield to use, although other internal container types are possible.
typedef ezBitfield<ezDynamicArray<ezUInt32> > ezDynamicBitfield;

/// \brief An ezBitfield that uses a hybrid array as internal container.
//template<ezUInt32 Size>
//using ezHybridBitfield = ezBitfield<ezHybridArray<ezUInt32, Size> >;


#include <Foundation/Containers/Implementation/Bitfield_inl.h>

