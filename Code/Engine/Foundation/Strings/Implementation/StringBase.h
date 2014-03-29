#pragma once

#include <Foundation/Strings/StringUtils.h>

/// Base class which marks a class as containing string data
class ezThisIsAString
{
};

/// Base class for strings, which implements all read-only string functions.
template <typename Derived>
class ezStringBase : public ezThisIsAString
{
public:
  /// Returns whether the string is an empty string.
  bool IsEmpty() const; // [tested]

  /// Returns true, if this string starts with the given string.
  bool StartsWith(const char* szStartsWith) const; // [tested]

  /// Returns true, if this string starts with the given string. Case insensitive.
  bool StartsWith_NoCase(const char* szStartsWith) const; // [tested]

  /// Returns true, if this string ends with the given string.
  bool EndsWith(const char* szEndsWith) const; // [tested]

  /// Returns true, if this string ends with the given string. Case insensitive.
  bool EndsWith_NoCase(const char* szEndsWith) const; // [tested]

  /// Returns a pointer to the first occurrence of szStringToFind, or NULL if none was found.
  /// To find the next occurrence, use an ezStringIterator which points to the next position and call FindSubString again.
  const char* FindSubString(const char* szStringToFind, const char* szStartSearchAt = NULL) const; // [tested]

  /// Returns a pointer to the first occurrence of szStringToFind, or NULL if none was found. Case insensitive.
  /// To find the next occurrence, use an ezStringIterator which points to the next position and call FindSubString again.
  const char* FindSubString_NoCase(const char* szStringToFind, const char* szStartSearchAt = NULL) const; // [tested]

  /// Returns a pointer to the last occurrence of szStringToFind, or NULL if none was found.
  /// szStartSearchAt allows to start searching at the end of the string (if it is NULL) or at an earlier position.
  const char* FindLastSubString(const char* szStringToFind, const char* szStartSearchAt = NULL) const; // [tested]

  /// Returns a pointer to the last occurrence of szStringToFind, or NULL if none was found. Case insensitive.
  /// szStartSearchAt allows to start searching at the end of the string (if it is NULL) or at an earlier position.
  const char* FindLastSubString_NoCase(const char* szStringToFind, const char* szStartSearchAt = NULL) const; // [tested]

  /// Searches for the word szSearchFor. If IsDelimiterCB returns true for both characters in front and back of the word, the position is returned. Otherwise NULL.
  const char* FindWholeWord(const char* szSearchFor, ezStringUtils::EZ_CHARACTER_FILTER IsDelimiterCB, const char* szStartSearchAt = NULL); // [tested]

  /// Searches for the word szSearchFor. If IsDelimiterCB returns true for both characters in front and back of the word, the position is returned. Otherwise NULL. Ignores case.
  const char* FindWholeWord_NoCase(const char* szSearchFor, ezStringUtils::EZ_CHARACTER_FILTER IsDelimiterCB, const char* szStartSearchAt = NULL); // [tested]

  /// Compares this string with the other one. Returns 0 for equality, -1 if this string is 'smaller', 1 otherwise.
  ezInt32 Compare(const char* pString2) const; // [tested]

  /// Compares up to a given number of characters of this string with the other one. Returns 0 for equality, -1 if this string is 'smaller', 1 otherwise.
  ezInt32 CompareN(const char* pString2, ezUInt32 uiCharsToCompare) const; // [tested]

  /// Compares this string with the other one. Returns 0 for equality, -1 if this string is 'smaller', 1 otherwise. Case insensitive.
  ezInt32 Compare_NoCase(const char* pString2) const; // [tested]

  /// Compares up to a given number of characters of this string with the other one. Returns 0 for equality, -1 if this string is 'smaller', 1 otherwise. Case insensitive.
  ezInt32 CompareN_NoCase(const char* pString2, ezUInt32 uiCharsToCompare) const; // [tested]

  /// Compares this string with the other string for equality.
  bool IsEqual(const char* pString2) const; // [tested]

  /// Compares up to a given number of characters of this string with the other string for equality. Case insensitive.
  bool IsEqualN(const char* pString2, ezUInt32 uiCharsToCompare) const; // [tested]

  /// Compares this string with the other string for equality.
  bool IsEqual_NoCase(const char* pString2) const; // [tested]

  /// Compares up to a given number of characters of this string with the other string for equality. Case insensitive.
  bool IsEqualN_NoCase(const char* pString2, ezUInt32 uiCharsToCompare) const; // [tested]

private:
  const char* InternalGetData() const;
  const char* InternalGetDataEnd() const;
  ezUInt32 InternalGetElementCount() const;

private: // friends

  template <typename DerivedLhs, typename DerivedRhs>
  friend bool operator== (const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs);

  template <typename DerivedLhs, typename DerivedRhs>
  friend bool operator!= (const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs);

  template <typename DerivedLhs, typename DerivedRhs>
  friend bool operator< (const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs);

  template <typename DerivedLhs, typename DerivedRhs>
  friend bool operator> (const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs);

  template <typename DerivedLhs, typename DerivedRhs>
  friend bool operator<= (const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs);

  template <typename DerivedLhs, typename DerivedRhs>
  friend bool operator>= (const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs);

  template <typename D, bool isString>
  friend struct ezHashHelperImpl;
};


#include <Foundation/Strings/Implementation/StringBase_inl.h>

