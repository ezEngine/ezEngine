#pragma once

#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/StringIterator.h>
#include <Foundation/Strings/Implementation/StringBase.h>
#include <Foundation/Containers/HybridArray.h>

/// A string class for storing and passing around strings.
/// This class only allows read-access to its data. It does not allow modifications.
/// To build / modify strings, use the ezStringBuilder class.
/// ezHybridString is optimized to reduce allocations when passing strings around. It does not copy data, when it is assigned
/// to / from another ezHybridString, but uses reference counting and a shared data object to prevent unnecessary copies.
/// It is thus no problem to pass it around by value, for example as a return value.
/// The reference counting is thread-safe.
template<ezUInt16 SIZE = 32>
class ezHybridString : public ezStringBase<ezHybridString<SIZE> >
{
public:
  /// Creates an empty string.
  ezHybridString(); // [tested]

  /// Creates a string which references the same data as rhs.
  /// Such copies are cheap, as the internal data is not copied, but reference counted.
  ezHybridString(const ezHybridString& rhs); // [tested]

  /// Creates a new string from the given (Utf8) data.
  /// This will allocate a new chunk of data to hold a copy of the passed string.
  /// Passing in an empty string will not allocate any data.
  ezHybridString(const char* rhs); // [tested]

  /// Creates a new string from the given wchar_t (Utf16 / Utf32) data.
  /// This will allocate a new chunk of data to hold a copy of the passed string.
  /// Passing in an empty string will not allocate any data.
  ezHybridString(const wchar_t* rhs); // [tested]

  /// Creates a new string from the given string iterator.
  /// This will allocate a new chunk of data to hold a copy of the passed string.
  /// Passing in an empty string will not allocate any data.
  ezHybridString(const ezStringIterator& rhs); // [tested]

  /// Only deallocates its data, if it is the last one to reference it.
  ~ezHybridString(); // [tested]

  /// Creates a string which references the same data as rhs. Such copies are cheap, as the internal data is not copied, but reference counted.
  void operator=(const ezHybridString& rhs); // [tested]

  /// Creates a new string from the given (Utf8) data.
  /// This will allocate a new chunk of data to hold a copy of the passed string.
  /// Assigning an empty string is the same as calling 'Clear'
  void operator=(const char* szString); // [tested]

  /// Creates a new string from the given wchar_t (Utf16 / Utf32) data.
  /// This will allocate a new chunk of data to hold a copy of the passed string.
  /// Assigning an empty string is the same as calling 'Clear'
  void operator=(const wchar_t* szString); // [tested]

  /// Creates a new string from the given string iterator.
  /// The string iterator may be created from the same ezHybridString, into which it is copied.
  /// This will allocate a new chunk of data to hold a copy of the passed string.
  /// Assigning an empty string is the same as calling 'Clear'
  void operator=(const ezStringIterator& rhs); // [tested]

  /// Resets this string to an empty string.
  /// This will deallocate any previous data, if this object held the last reference to it.
  void Clear(); // [tested]

  /// Returns a pointer to the internal Utf8 string.
  const char* GetData() const; // [tested]

  /// Returns the amount of bytes that this string takes (excluding the '\0' terminator).
  ezUInt32 GetElementCount() const; // [tested]

  /// Returns the number of characters in this string.
  ezUInt32 GetCharacterCount() const; // [tested]

  /// Returns an iterator to this string, which points to the very first character.
  /// Note that this iterator will only be valid as long as this ezHybridString lives.
  /// Once the original string is destroyed, all iterators to them will point into invalid memory.
  ezStringIterator GetIteratorFront() const; // [tested]

  /// Returns an iterator to this string, which points to the very last character (NOT the end).
  /// Note that this iterator will only be valid as long as this ezHybridString lives.
  /// Once the original string is destroyed, all iterators to them will point into invalid memory.
  ezStringIterator GetIteratorBack() const; // [tested]

  /// Returns an iterator to a sub-string of this string, starting at character uiFirstCharacter, up until uiFirstCharacter +  uiNumCharacters.
  /// Note that this iterator will only be valid as long as this ezHybridString lives.
  /// Once the original string is destroyed, all iterators to them will point into invalid memory.
  ezStringIterator GetSubString(ezUInt32 uiFirstCharacter, ezUInt32 uiNumCharacters) const; // [tested]

  /// Returns an iterator to the sub-string containing the first uiNumCharacters characters of this string.
  /// Note that this iterator will only be valid as long as this ezHybridString lives.
  /// Once the original string is destroyed, all iterators to them will point into invalid memory.
  ezStringIterator GetFirst(ezUInt32 uiNumCharacters) const; // [tested]

  /// Returns an iterator to the sub-string containing the last uiNumCharacters characters of this string.
  /// Note that this iterator will only be valid as long as this ezHybridString lives.
  /// Once the original string is destroyed, all iterators to them will point into invalid memory.
  ezStringIterator GetLast(ezUInt32 uiNumCharacters) const; // [tested]

private:
  ezHybridArray<char, SIZE> m_Data;
  ezUInt32 m_uiCharacterCount;
};

typedef ezHybridString<32> ezString;
typedef ezHybridString<16> ezString16;
typedef ezHybridString<24> ezString24;
typedef ezHybridString<32> ezString32;
typedef ezHybridString<48> ezString48;
typedef ezHybridString<64> ezString64;
typedef ezHybridString<128> ezString128;
typedef ezHybridString<256> ezString256;

#include <Foundation/Strings/Implementation/String_inl.h>
