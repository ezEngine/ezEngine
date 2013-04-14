#pragma once

#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/StringIterator.h>
#include <Foundation/Strings/Implementation/SharedStringBase.h>
#include <Foundation/Strings/Implementation/StringBase.h>

/// \brief A string class for storing and passing around strings.
///
/// This class only allows read-access to its data. It does not allow modifications.
/// To build / modify strings, use the ezStringBuilder class.
/// ezSharedString is optimized to reduce allocations when passing strings around. It does not copy data, when it is assigned
/// to / from another ezSharedString, but uses reference counting and a shared data object to prevent unnecessary copies.
/// It is thus no problem to pass it around by value, for example as a return value.
/// The reference counting is thread-safe.
class EZ_FOUNDATION_DLL ezSharedString : public ezStringBase<ezSharedString>
{
public:
  /// \brief Creates an empty string.
  ezSharedString(); // [tested]

  /// \brief Creates a string which references the same data as \a rhs.
  ///
  /// Such copies are cheap, as the internal data is not copied, but reference counted.
  ezSharedString(const ezSharedString& rhs); // [tested]

  /// \brief Creates a new string from the given (Utf8) data.
  ///
  /// This will allocate a new chunk of data to hold a copy of the passed string.
  /// Passing in an empty string will not allocate any data.
  ezSharedString(const char* rhs); // [tested]

  /// \brief Creates a new string from the given wchar_t (Utf16 / Utf32) data.
  ///
  /// This will allocate a new chunk of data to hold a copy of the passed string.
  /// Passing in an empty string will not allocate any data.
  ezSharedString(const wchar_t* rhs); // [tested]

  /// \brief Creates a new string from the given string iterator.
  ///
  /// This will allocate a new chunk of data to hold a copy of the passed string.
  /// Passing in an empty string will not allocate any data.
  ezSharedString(const ezStringIterator& rhs); // [tested]

  /// \brief Only deallocates its data, if it is the last one to reference it.
  ~ezSharedString(); // [tested]

  /// \brief Creates a string which references the same data as rhs. Such copies are cheap, as the internal data is not copied, but reference counted.
  void operator=(const ezSharedString& rhs); // [tested]

  /// \brief Creates a new string from the given (Utf8) data.
  ///
  /// This will allocate a new chunk of data to hold a copy of the passed string.
  /// Assigning an empty string is the same as calling 'Clear'
  void operator=(const char* szString); // [tested]

  /// \brief Creates a new string from the given wchar_t (Utf16 / Utf32) data.
  ///
  /// This will allocate a new chunk of data to hold a copy of the passed string.
  /// Assigning an empty string is the same as calling 'Clear'
  void operator=(const wchar_t* szString); // [tested]

  /// \brief Creates a new string from the given string iterator.
  ///
  /// The string iterator may be created from the same ezSharedString, into which it is copied.
  /// This will allocate a new chunk of data to hold a copy of the passed string.
  /// Assigning an empty string is the same as calling 'Clear'
  void operator=(const ezStringIterator& rhs); // [tested]

  /// \brief Resets this string to an empty string.
  ///
  /// This will deallocate any previous data, if this object held the last reference to it.
  void Clear(); // [tested]

  /// \brief Returns a pointer to the internal Utf8 string.
  const char* GetData() const; // [tested]

  /// \brief Returns the amount of bytes that this string takes (excluding the '\0' terminator).
  ezUInt32 GetElementCount() const; // [tested]

  /// \brief Returns the number of characters in this string.
  ezUInt32 GetCharacterCount() const; // [tested]

  /// \brief Returns an iterator to this string, which points to the very first character.
  ///
  /// \note Note that this iterator will only be valid as long as this ezSharedString lives.
  ///       Once the original string is destroyed, all iterators to them will point into invalid memory.
  ezStringIterator GetIteratorFront() const; // [tested]

  /// \brief Returns an iterator to this string, which points to the very last character (NOT the end).
  ///
  /// \note Note that this iterator will only be valid as long as this ezSharedString lives.
  ///       Once the original string is destroyed, all iterators to them will point into invalid memory.
  ezStringIterator GetIteratorBack() const; // [tested]

  /// \brief Returns an iterator to a sub-string of this string, starting at character uiFirstCharacter, up until uiFirstCharacter +  uiNumCharacters.
  ///
  /// \note Note that this iterator will only be valid as long as this ezSharedString lives.
  ///       Once the original string is destroyed, all iterators to them will point into invalid memory.
  ezStringIterator GetSubString(ezUInt32 uiFirstCharacter, ezUInt32 uiNumCharacters) const; // [tested]

  /// \brief Returns an iterator to the sub-string containing the first uiNumCharacters characters of this string.
  ///
  /// \note Note that this iterator will only be valid as long as this ezSharedString lives.
  ///       Once the original string is destroyed, all iterators to them will point into invalid memory.
  ezStringIterator GetFirst(ezUInt32 uiNumCharacters) const; // [tested]

  /// \brief Returns an iterator to the sub-string containing the last uiNumCharacters characters of this string.
  ///
  /// \note Note that this iterator will only be valid as long as this ezSharedString lives.
  ///       Once the original string is destroyed, all iterators to them will point into invalid memory.
  ezStringIterator GetLast(ezUInt32 uiNumCharacters) const; // [tested]

private:

  /// \brief Increases the refcount of m_pShared.
  void Acquire();

  /// \brief Decreases the refcount of m_pShared.
  void Release();

  bool TryOverwrite(const char* szString, const char* szStringEnd = ezMaxStringEnd);

  /// \brief Used to properly initialize empty strings.
  static ezInternal::ezSharedStringBase s_EmptyString;

  /// \brief Points to the object that actually holds the data.
  ezInternal::ezSharedStringBase* m_pShared;
};

#include <Foundation/Strings/Implementation/SharedString_inl.h>
