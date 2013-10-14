
#pragma once

#include <Foundation/Basics.h>


/// \brief This class provides a base class for hashable structs (e.g. descriptor objects).
/// To help with this there are two parts: 1) memclear on initialization 2) a Hash() function calculating the hash of the object
template<typename T> class ezHashableStruct
{
public:

  EZ_FORCE_INLINE ezHashableStruct();  // [tested]

  /// \brief Calculates the (CRC32) hash of the struct and returns it
  EZ_FORCE_INLINE ezUInt32 CalculateHash() const;  // [tested]
};

#include <Foundation/Algorithm/Implementation/HashableStruct_inl.h>
