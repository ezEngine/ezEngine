#pragma once

#include <ThirdParty/utf8/utf8.h>
#include <Foundation/Basics.h>
#include <Foundation/Strings/UnicodeUtils.h>
#include <Foundation/Threading/AtomicInteger.h>

// Minus 1 wraps around to the maximum unsigned integer value, which is different on 32 Bit and 64 Bit
#define ezMaxStringEnd (const char*)-1

/// \brief Helper functions to work with UTF-8 strings (which include pure ASCII strings)
class EZ_FOUNDATION_DLL ezStringUtils
{
public:

  /// \brief Returns true, if the given string is a nullptr pointer or a string that immediately terminates with a '\0' character.
  template <typename T>
  static bool IsNullOrEmpty(const T* pString); // [tested]

  /// \brief Recomputes the end pointer of a string (\a szStringEnd), if that is currently set to ezMaxStringEnd. Otherwise does nothing.
  template <typename T>
  static void UpdateStringEnd(const T* szStringStart, const T*& szStringEnd);

  /// \brief Returns the number of elements of type T that the string contains, until it hits an element that is zero OR until it hits the end pointer.
  ///
  /// Equal to the string length, if used with pure ASCII strings.
  /// Equal to the amount of bytes in a string, if used on non-ASCII (i.e. UTF-8) strings.
  /// Equal to the number of characters in a string, if used with UTF-32 strings.
  template <typename T>
  static ezUInt32 GetStringElementCount(const T* pString, const T* pStringEnd = (const T*) -1); // [tested]

  /// \brief Returns the number of characters (not Bytes!) in a Utf8 string (excluding the zero terminator), until it hits zero or the end pointer.
  static ezUInt32 GetCharacterCount(const char* szUtf8, const char* pStringEnd = ezMaxStringEnd); // [tested]

  /// \brief Returns both the number of characters and the number of bytes in a Utf8 string, until it hits zero or the end pointer.
  static void GetCharacterAndElementCount(const char* szUtf8, ezUInt32& uiCharacterCount, ezUInt32& uiElementCount, const char* pStringEnd = ezMaxStringEnd); // [tested]

  /// \brief Copies the string from szSource into the given buffer, which can hold at least uiDstSize bytes.
  ///
  /// The string will always be \0 terminated.
  /// Multi-byte UTF-8 characters will only be copied, if they can fit completely into szDest. 
  /// I.e. they will be truncated at a character boundary.
  /// Returns the number of bytes that were copied into szDest, excluding the terminating \0
  static ezUInt32 Copy(char* szDest, ezUInt32 uiDstSize, const char* szSource, const char* pSourceEnd = ezMaxStringEnd); // [tested]

  /// \brief Copies up to uiCharsToCopy characters into the given buffer, which can hold at least uiDstSize bytes.
  ///
  /// The string will always be \0 terminated.
  /// Multi-byte UTF-8 characters will only be copied, if they can fit completely into szDest.
  ///  I.e. they will be truncated at a character boundary.
  /// Returns the number of bytes that were copied into szDest, excluding the terminating \0
  static ezUInt32 CopyN(char* szDest, ezUInt32 uiDstSize, const char* szSource, ezUInt32 uiCharsToCopy, const char* pSourceEnd = ezMaxStringEnd); // [tested]

  /// \brief Returns the upper case code point for uiChar.
  static ezUInt32 ToUpperChar(ezUInt32 uiChar); // [tested]

  /// \brief Returns the lower case code point for uiChar.
  static ezUInt32 ToLowerChar(ezUInt32 uiChar); // [tested]

  /// \brief Converts a (UTF-8) string in-place to upper case.
  ///
  /// Returns the new string length in bytes (it might shrink, but never grow), excluding the \0 terminator.
  static ezUInt32 ToUpperString(char* szString); // [tested]

  /// \brief Converts a (UTF-8) string in-place to lower case.
  ///
  /// Returns the new string length in bytes (it might shrink, but never grow), excluding the \0 terminator.
  static ezUInt32 ToLowerString(char* szString); // [tested]

  /// \brief Compares the two code points for equality. 
  ///
  /// Returns a negative number, if uiCharacter1 is smaller than uiCharacter2.
  /// Returns a positive number, if uiCharacter1 is larger than uiCharacter2.
  /// Returns 0 if both are equal.
  static ezInt32 CompareChars(ezUInt32 uiCharacter1, ezUInt32 uiCharacter2); // [tested]

  /// \brief Compares the first character of each utf8 string for equality.
  ///
  /// Returns a negative number, if szUtf8Char1 is smaller than szUtf8Char2.
  /// Returns a positive number, if szUtf8Char1 is larger than szUtf8Char2.
  /// Returns 0 if both are equal.
  static ezInt32 CompareChars(const char* szUtf8Char1, const char* szUtf8Char2); // [tested]

  /// \brief Compares the two code points for equality, case-insensitive.
  ///
  /// Returns a negative number, if uiCharacter1 is smaller than uiCharacter2.
  /// Returns a positive number, if uiCharacter1 is larger than uiCharacter2.
  /// Returns 0 if both are equal.
  static ezInt32 CompareChars_NoCase(ezUInt32 uiCharacter1, ezUInt32 uiCharacter2); // [tested]

  /// \brief Compares the first character of each utf8 string for equality, case-insensitive.
  ///
  /// Returns a negative number, if szUtf8Char1 is smaller than szUtf8Char2.
  /// Returns a positive number, if szUtf8Char1 is larger than szUtf8Char2.
  /// Returns 0 if both are equal.
  static ezInt32 CompareChars_NoCase(const char* szUtf8Char1, const char* szUtf8Char2); // [tested]

  /// \brief Returns true, if the two given strings are identical (bitwise).
  static bool IsEqual(const char* pString1, const char* pString2, const char* pString1End = ezMaxStringEnd, const char* pString2End = ezMaxStringEnd); // [tested]

  /// \brief Returns true, if the two given strings are identical (bitwise) up to the n-th character.
  ///
  /// This function will handle UTF-8 strings properly.
  static bool IsEqualN(const char* pString1, const char* pString2, ezUInt32 uiCharsToCompare, const char* pString1End = ezMaxStringEnd, const char* pString2End = ezMaxStringEnd); // [tested]

  /// \brief Returns true, if the two given strings are identical (case-insensitive).
  static bool IsEqual_NoCase(const char* pString1, const char* pString2, const char* pString1End = ezMaxStringEnd, const char* pString2End = ezMaxStringEnd); // [tested]

  /// \brief Returns true, if the two given strings are identical (case-insensitive) up to the n-th character.
  ///
  /// This function will handle UTF-8 strings properly.
  static bool IsEqualN_NoCase(const char* pString1, const char* pString2, ezUInt32 uiCharsToCompare, const char* pString1End = ezMaxStringEnd, const char* pString2End = ezMaxStringEnd); // [tested]

  /// \brief Compares two strings for equality.
  ///
  /// Returns a negative number if the pString1 is 'smaller' or shorter than pString2.
  /// Returns a positive number, if pString1 is 'larger' or longer than pString1.
  /// Returns 0 for equal strings.
  /// Works with UTF-8 strings as well.
  static ezInt32 Compare(const char* pString1, const char* pString2, const char* pString1End = ezMaxStringEnd, const char* pString2End = ezMaxStringEnd); // [tested]

  /// \brief Compares the first uiCharsToCompare characters of the two strings for equality. 
  ///
  /// Returns a negative number if the pString1 is 'smaller' or shorter than pString2.
  /// Returns a positive number, if pString1 is 'larger' or longer than pString1.
  /// Returns 0 for equal strings.
  /// Works with UTF-8 strings as well.
  static ezInt32 CompareN(const char* pString1, const char* pString2, ezUInt32 uiCharsToCompare, const char* pString1End = ezMaxStringEnd, const char* pString2End = ezMaxStringEnd); // [tested]

  /// \brief Compares two strings for equality, case-insensitive.
  ///
  /// Returns a negative number if the pString1 is 'smaller' or shorter than pString2.
  /// Returns a positive number, if pString1 is 'larger' or longer than pString1.
  /// Returns 0 for equal strings.
  /// Works with UTF-8 strings as well.
  static ezInt32 Compare_NoCase(const char* pString1, const char* pString2, const char* pString1End = ezMaxStringEnd, const char* pString2End = ezMaxStringEnd); // [tested]

  /// \brief Compares the first uiCharsToCompare characters of the two strings for equality, case-insensitive.
  ///
  /// Returns a negative number if the pString1 is 'smaller' or shorter than pString2.
  /// Returns a positive number, if pString1 is 'larger' or longer than pString1.
  /// Returns 0 for equal strings.
  /// Works with UTF-8 strings as well.
  static ezInt32 CompareN_NoCase(const char* pString1, const char* pString2, ezUInt32 uiCharsToCompare, const char* pString1End = ezMaxStringEnd, const char* pString2End = ezMaxStringEnd); // [tested]


  /// \brief Creates a formated string in szDst. uiDstSize defines how many bytes szDst can hold. 
  ///
  /// Returns the number of bytes that would have been required to output the entire string (excluding the 0 terminator).\n
  /// Returns -1 if an error occurred. In this case it might also write a more detailed error message to the destination string itself.
  /// szDst may be nullptr.\n
  /// uiDstSize may be zero.\n
  /// This can be used to query how much storage is required, then allocate it and call snprintf again to fill it.\n
  /// Formatting works exactly like printf, except that it additionally supports outputting binary with the 'b' modifier and it will
  /// output float NaN and Infinity as proper text.
  static ezInt32 snprintf(char* szDst, ezUInt32 uiDstSize, const char* szFormat, ...); // [tested]

  /// \brief Creates a formated string in szDst. uiDstSize defines how many bytes szDst can hold. 
  ///
  /// Returns the number of bytes that would have been required to output the entire string (excluding the 0 terminator).\n
  /// Returns -1 if an error occurred. In this case it might also write a more detailed error message to the destination string itself.
  /// szDst may be nullptr.\n
  /// uiDstSize may be zero.\n
  /// This can be used to query how much storage is required, then allocate it and call snprintf again to fill it.\n
  /// Formatting works exactly like printf, except that it additionally supports outputting binary with the 'b' modifier and it will
  /// output float NaN and Infinity as proper text.
  static ezInt32 vsnprintf(char* szDst, ezUInt32 uiDstSize, const char* szFormat, va_list ap); // [tested]

  /// \brief Returns true if szString starts with the string given in szStartsWith.
  static bool StartsWith(const char* szString, const char* szStartsWith, const char* pStringEnd = ezMaxStringEnd); // [tested]

  /// \brief Returns true if szString starts with the string given in szStartsWith. Ignores case.
  static bool StartsWith_NoCase(const char* szString, const char* szStartsWith, const char* pStringEnd = ezMaxStringEnd); // [tested]

  /// \brief Returns true if szString ends with the string given in szEndsWith.
  static bool EndsWith(const char* szString, const char* szEndsWith, const char* pStringEnd = ezMaxStringEnd); // [tested]

  /// \brief Returns true if szString ends with the string given in szEndsWith. Ignores case.
  static bool EndsWith_NoCase(const char* szString, const char* szEndsWith, const char* pStringEnd = ezMaxStringEnd); // [tested]


  /// \brief Searches for the first occurrence of szStringToFind in szSource. 
  static const char* FindSubString(const char* szSource, const char* szStringToFind, const char* pSourceEnd = ezMaxStringEnd); // [tested]

  /// \brief Searches for the first occurrence of szStringToFind in szSource. Ignores case.
  static const char* FindSubString_NoCase(const char* szSource, const char* szStringToFind, const char* pSourceEnd = ezMaxStringEnd); // [tested]

  /// \brief Searches for the last occurrence of szStringToFind in szSource before szStartSearchAt.
  static const char* FindLastSubString(const char* szSource, const char* szStringToFind, const char* szStartSearchAt = nullptr, const char* pSourceEnd = ezMaxStringEnd); // [tested]

  /// \brief Searches for the last occurrence of szStringToFind in szSource before szStartSearchAt. Ignores case.
  static const char* FindLastSubString_NoCase(const char* szSource, const char* szStringToFind, const char* szStartSearchAt = nullptr, const char* pSourceEnd = ezMaxStringEnd); // [tested]

  /// \brief Function Definition for a function that determines whether a (Utf32) character belongs to a certain category of characters.
  typedef bool (*EZ_CHARACTER_FILTER)(ezUInt32 uiChar);

  /// \brief Starts at szString and advances to the next character for which SkipCharacterCB returns false;
  ///
  /// If \a bAlwaysSkipFirst is false and szString points to a character that does not fulfill the filter, this function will
  /// return immediately and nothing will change.
  /// If \a bAlwaysSkipFirst is true, the first character will always be skipped, regardless what it is (unless it is the zero terminator).
  /// The latter is useful to skip an entire word and get to the next word in a string.\n
  static const char* SkipCharacters(const char* szString, EZ_CHARACTER_FILTER SkipCharacterCB, bool bAlwaysSkipFirst = false); // [tested]

  /// \brief Returns the position in szString at which \a IsDelimiterCB returns true.
  ///
  /// This is basically the inverse of SkipCharacters. SkipCharacters advances over all characters that fulfill the filter, 
  /// FindWordEnd advances over all characters that do not fulfill it.
  static const char* FindWordEnd(const char* szString, EZ_CHARACTER_FILTER IsDelimiterCB, bool bAlwaysSkipFirst = true); // [tested]

  /// \brief A default word delimiter function that returns true for ' ' (space), '\r' (carriage return), '\n' (newline), '\t' (tab) and '\v' (vertical tab)
  static bool IsWhiteSpace(ezUInt32 uiChar); // [tested]

  /// \brief A default word delimiter function for English text.
  static bool IsWordDelimiter_English(ezUInt32 uiChar); // [tested]

  /// \brief A default word delimiter function for identifiers in C code. 
  static bool IsIdentifierDelimiter_C_Code(ezUInt32 uiChar); // [tested]

  /// \brief Searches szString for the word szSearchFor. If IsDelimiterCB returns true for both characters in front and back of the word, the position is returned. Otherwise nullptr.
  static const char* FindWholeWord(const char* szString, const char* szSearchFor, EZ_CHARACTER_FILTER IsDelimiterCB, const char* pStringEnd = ezMaxStringEnd); // [tested]

  /// \brief Searches szString for the word szSearchFor. If IsDelimiterCB returns true for both characters in front and back of the word, the position is returned. Otherwise nullptr. Ignores case.
  static const char* FindWholeWord_NoCase(const char* szString, const char* szSearchFor, EZ_CHARACTER_FILTER IsDelimiterCB, const char* pStringEnd = ezMaxStringEnd); // [tested]

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  static void AddUsedStringLength(ezUInt32 uiLength);
  static void PrintStringLengthStatistics();
  static ezAtomicInteger32 g_MaxUsedStringLength;
  static ezAtomicInteger32 g_UsedStringLengths[256];
#else
  EZ_FORCE_INLINE static void AddUsedStringLength(ezUInt32 uiLength) { }
  EZ_FORCE_INLINE static void PrintStringLengthStatistics() { }
#endif
};


#include <Foundation/Strings/Implementation/StringUtils_inl.h>

