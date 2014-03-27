
#pragma once

#include <Foundation/Basics.h>


/// \brief This class provides a base class for hashable structs (e.g. descriptor objects).
///
/// To help with this there are two parts:
///   1) memclear on initialization.
///   2) a CalculateHash() function calculating the CRC32 hash of the object.
///
/// You can make your own struct hashable by deriving from ezHashableStruct providing the type of
/// your class / struct as the template parameter.
template<typename DERIVED>
class ezHashableStruct
{
public:

  EZ_FORCE_INLINE ezHashableStruct();  // [tested]

  /// \brief Calculates the (CRC32) hash of the struct and returns it
  EZ_FORCE_INLINE ezUInt32 CalculateHash() const;  // [tested]
};

#include <Foundation/Algorithm/Implementation/HashableStruct_inl.h>

