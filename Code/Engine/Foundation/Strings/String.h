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
template <ezUInt16 Size>
class ezHybridStringBase : public ezStringBase<ezHybridStringBase<Size> >
{
protected:
  /// Creates an empty string.
  ezHybridStringBase(ezIAllocator* pAllocator); // [tested]

  /// Creates a string which references the same data as rhs.
  /// Such copies are cheap, as the internal data is not copied, but reference counted.
  ezHybridStringBase(const ezHybridStringBase& rhs, ezIAllocator* pAllocator); // [tested]

  /// Creates a new string from the given (Utf8) data.
  /// This will allocate a new chunk of data to hold a copy of the passed string.
  /// Passing in an empty string will not allocate any data.
  ezHybridStringBase(const char* rhs, ezIAllocator* pAllocator); // [tested]

  /// Creates a new string from the given wchar_t (Utf16 / Utf32) data.
  /// This will allocate a new chunk of data to hold a copy of the passed string.
  /// Passing in an empty string will not allocate any data.
  ezHybridStringBase(const wchar_t* rhs, ezIAllocator* pAllocator); // [tested]

  /// Creates a new string from the given string iterator.
  /// This will allocate a new chunk of data to hold a copy of the passed string.
  /// Passing in an empty string will not allocate any data.
  ezHybridStringBase(const ezStringIterator& rhs, ezIAllocator* pAllocator); // [tested]

  /// Only deallocates its data, if it is the last one to reference it.
  ~ezHybridStringBase(); // [tested]

  /// Creates a string which references the same data as rhs. Such copies are cheap, as the internal data is not copied, but reference counted.
  void operator=(const ezHybridStringBase& rhs); // [tested]

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

public:
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
  ezHybridArray<char, Size> m_Data;
  ezUInt32 m_uiCharacterCount;
};

template <ezUInt16 Size, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezHybridString : public ezHybridStringBase<Size>
{
public:
  ezHybridString();
  ezHybridString(ezIAllocator* pAllocator);

  ezHybridString(const ezHybridString<Size, AllocatorWrapper>& other);
  ezHybridString(const ezHybridStringBase<Size>& other);
  ezHybridString(const char* rhs);
  ezHybridString(const wchar_t* rhs);
  ezHybridString(const ezStringIterator& rhs);

  void operator=(const ezHybridString<Size, AllocatorWrapper>& rhs);
  void operator=(const ezHybridStringBase<Size>& rhs);
  void operator=(const char* szString);
  void operator=(const wchar_t* szString);
  void operator=(const ezStringIterator& rhs);
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
