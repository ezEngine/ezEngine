
#pragma once

#include <Foundation/Basics.h>


/// \brief This class provides implementations of different hashing algorithms.
class EZ_FOUNDATION_DLL ezHashingUtils
{
public:
  /// \brief helper struct to wrap a string pointer
  struct StringWrapper
  {
    EZ_ALWAYS_INLINE StringWrapper(const char* str) : m_str(str) {}
    const char* m_str;
  };

  /// \brief Calculates the CRC32 checksum of the given key.
  static ezUInt32 CRC32Hash(const void* pKey, size_t uiSizeInBytes); // [tested]

  /// \brief Calculates the 32bit murmur hash of the given key.
  static ezUInt32 MurmurHash32(const void* pKey, size_t uiSizeInByte, ezUInt32 uiSeed = 0); // [tested]

  /// \brief Calculates the 64bit murmur hash of the given key.
  static ezUInt64 MurmurHash64(const void* pKey, size_t uiSizeInByte, ezUInt64 uiSeed = 0); // [tested]

  /// \brief Calculates the 32bit murmur hash of a string constant at compile time. Encoding does not matter here.
  template <size_t N>
  constexpr static ezUInt32 MurmurHash32String(const char (&str)[N], ezUInt32 uiSeed = 0); // [tested]

  /// \brief Calculates the 32bit murmur hash of a string pointer during runtime. Encoding does not matter here.
  ///
  /// We cannot pass a string pointer directly since a string constant would be treated as pointer as well.
  static ezUInt32 MurmurHash32String(StringWrapper str, ezUInt32 uiSeed = 0); // [tested]

  /// \brief Calculates the 32bit xxHash of the given key.
  static ezUInt32 xxHash32(const void* pKey, size_t uiSizeInByte, ezUInt32 uiSeed = 0); // [tested]

  /// \brief Calculates the 64bit xxHash of the given key.
  static ezUInt64 xxHash64(const void* pKey, size_t uiSizeInByte, ezUInt64 uiSeed = 0); // [tested]
};

/// \brief Helper struct to calculate the Hash of different types.
///
/// This struct can be used to provide a custom hash function for ezHashTable. The default implementation uses the murmur hash function.
template <typename T>
struct ezHashHelper
{
  static ezUInt32 Hash(const T& value);
  static bool Equal(const T& a, const T& b);
};

#include <Foundation/Algorithm/Implementation/HashingMurmur_inl.h>
#include <Foundation/Algorithm/Implementation/HashingUtils_inl.h>

