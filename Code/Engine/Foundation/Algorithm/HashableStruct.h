
#pragma once

#include <Foundation/Basics.h>


/// \brief This class provides a base class for hashable structs (e.g. descriptor objects).
///
/// To help with this there are two parts:
///   1) memclear on initialization.
///   2) a CalculateHash() function calculating the 32 bit hash of the object.
///
/// You can make your own struct hashable by deriving from ezHashableStruct providing the type of
/// your class / struct as the template parameter.
template <typename DERIVED>
class ezHashableStruct
{
public:
  ezHashableStruct();                                       // [tested]
  ezHashableStruct(const ezHashableStruct<DERIVED>& other); // [tested]

  void operator=(const ezHashableStruct<DERIVED>& other);   // [tested]
  bool operator==(const ezHashableStruct<DERIVED>& other) const;
  bool operator!=(const ezHashableStruct<DERIVED>& other) const;
  bool operator<(const ezHashableStruct<DERIVED>& other) const;

  /// \brief Calculates the 32 bit hash of the struct and returns it
  ezUInt32 CalculateHash() const; // [tested]
};

#include <Foundation/Algorithm/Implementation/HashableStruct_inl.h>
