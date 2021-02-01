#ifndef AE_FOUNDATION_STRINGS_STRINGFUNCTIONS_H
#define AE_FOUNDATION_STRINGS_STRINGFUNCTIONS_H

#include "../Basics/Checks.h"
#include "Declarations.h"
#include <stdarg.h>

namespace AE_NS_FOUNDATION
{
  //! This static class contains many functions for basic string operations on c-strings.
  /*! Many functions are simply wrappers for the default c-lib functions. This serves as an
      abstraction to be able to pick the most optimized function on each platform.
      Also the function names are much more readable.
  */
  struct AE_FOUNDATION_DLL aeStringFunctions
  {
    //! Returns true if the given string is a nullptr pointer or an empty string ("").
    static bool IsNullOrEmpty(const char* szString);

    //! Returns the length of the given string.
    static aeUInt32 GetLength(const char* szString);

    //! Returns true if both strings are equal.
    static bool CompareEqual(const char* szString1, const char* szString2);
    //! Returns true if the first n characters of both strings are equal.
    static bool CompareEqual(const char* szString1, const char* szString2, aeUInt32 uiCharsToCompare);
    //! Returns true if both strings are equal. Ignores the case of characters.
    static bool CompareEqual_NoCase(const char* szString1, const char* szString2);
    //! Returns true if the first n characters of both strings are equal. Ignores the case of characters.
    static bool CompareEqual_NoCase(const char* szString1, const char* szString2, aeUInt32 uiCharsToCompare);

    //! Returns 0 if both strings are equal. A negative value if szString1 is smaller than szString2 and a positive value otherwise.
    static aeInt32 CompareAlphabetically(const char* szString1, const char* szString2);
    //! Returns 0 if the first n characters of both strings are equal. A negative value if szString1 is smaller than szString2 and a positive value otherwise.
    static aeInt32 CompareAlphabetically(const char* szString1, const char* szString2, aeUInt32 uiCharsToCompare);
    //! Returns 0 if both strings are equal. A negative value if szString1 is smaller than szString2 and a positive value otherwise. Ignores the case of characters.
    static aeInt32 CompareAlphabetically_NoCase(const char* szString1, const char* szString2);
    //! Returns 0 if the first n characters of both strings are equal. A negative value if szString1 is smaller than szString2 and a positive value otherwise. Ignores the case of characters.
    static aeInt32 CompareAlphabetically_NoCase(const char* szString1, const char* szString2, aeUInt32 uiCharsToCompare);

    //! Copies szSource into szDest. uiDstSize defines how many characters szDest can hold.
    static void Copy(char* szDest, aeUInt32 uiDstSize, const char* szSource);
    //! Copies up to uiCharsToCopy characters from szSource into szDest. uiDstSize defines how many characters szDest can hold.
    static void Copy(char* szDest, aeUInt32 uiDstSize, const char* szSource, aeUInt32 uiCharsToCopy);

    //! Appends the string szSource to szDest. uiDstSize defines how many characters szDest can hold. Returns the new length of the string.
    static aeUInt32 Concatenate(char* szDest, aeUInt32 uiDstSize, const char* szSource);
    //! Appends the string szSource to szDest. uiDstSize defines how many characters szDest can hold. uiDstStrLen says how long szDest is already, which makes it unnecessary to compute its length. Returns the new length of the string.
    static aeUInt32 Concatenate(char* szDest, aeUInt32 uiDstSize, aeUInt32 uiDstStrLen, const char* szSource);
    //! Appends up to uiCharsToCopy characters from szSource to szDest. uiDstSize defines how many characters szDest can hold. Returns the new length of the string.
    static aeUInt32 Concatenate(char* szDest, aeUInt32 uiDstSize, const char* szSource, aeUInt32 uiCharsToCopy);
    //! Appends up to uiCharsToCopy characters from szSource to szDest. uiDstSize defines how many characters szDest can hold. uiDstStrLen says how long szDest is already, which makes it unnecessary to compute its length. Returns the new length of the string.
    static aeUInt32 Concatenate(char* szDest, aeUInt32 uiDstSize, aeUInt32 uiDstStrLen, const char* szSource, aeUInt32 uiCharsToCopy);

    //! Creates a formated string in szDst. uiDstSize defines how many characters szDst can hold. Similar to sprintf.
    static aeUInt32 Format(char* szDst, aeUInt32 uiDstSize, const char* szFormat, ...);
    //! Creates a formated string in szDst. uiDstSize defines how many characters szDst can hold. Similar to sprintf.
    static aeUInt32 FormatArgs(char* szDst, aeUInt32 uiDstSize, const char* szFormat, va_list& ap);

    //! Creates a formated string in szDst. uiDstSize defines how many characters szDst can hold.
    /*! Returns the number of characters that would have been required to output the entire string.\n
		szDst may be nullptr.\n
		uiDstSize may be zero.\n
		This can be used to query how much storage is required, then allocate it and call snprintf again to fill it.\n
		Formatting works exactly like printf.
	*/
    static aeInt32 snprintf(char* szDst, aeUInt32 uiDstSize, const char* szFormat, ...);
    //! Creates a formated string in szDst. uiDstSize defines how many characters szDst can hold.
    /*! Returns the number of characters that would have been required to output the entire string.\n
		szDst may be nullptr.\n
		uiDstSize may be zero.\n
		This can be used to query how much storage is required, then allocate it and call snprintf again to fill it.\n
		Formatting works exactly like printf.
	*/
    static aeInt32 vsnprintf(char* szDst, aeUInt32 uiDstSize, const char* szFormat, va_list& ap);

    //! Searches for the first occurance of szStringToFind in szSource. Returns the index or -1 if nothing is found.
    static aeInt32 FindFirstStringPos(const char* szSource, const char* szStringToFind, aeUInt32 uiStartPos = 0);
    //! Searches for the first occurance of szStringToFind in szSource. Returns the index or -1 if nothing is found. Ignores case.
    static aeInt32 FindFirstStringPos_NoCase(const char* szSource, const char* szStringToFind, aeUInt32 uiStartPos = 0);
    //! Searches for the last occurance of szStringToFind in szSource. Returns the index or -1 if nothing is found.
    static aeInt32 FindLastStringPos(const char* szSource, const char* szStringToFind, aeUInt32 uiStartPos = 0x0FFFFFFF);
    //! Searches for the last occurance of szStringToFind in szSource. Returns the index or -1 if nothing is found. Ignores case.
    static aeInt32 FindLastStringPos_NoCase(const char* szSource, const char* szStringToFind, aeUInt32 uiStartPos = 0x0FFFFFFF);

    //! Converts all characters in szString to upper case.
    static void ToUpperCase(char* szString);
    //! Converts all characters in szString to lower case.
    static void ToLowerCase(char* szString);

    //! Converts the given character to upper case.
    static char ToUpperCase(char cChar);
    //! Converts the given character to lower case.
    static char ToLowerCase(char cChar);

    //! Returns true if szString starts with the string given in szStartsWith.
    static bool StartsWith(const char* szString, const char* szStartsWith);
    //! Returns true if szString starts with the string given in szStartsWith. Ignores case.
    static bool StartsWith_NoCase(const char* szString, const char* szStartsWith);
    //! Returns true if szString ends with the string given in szEndsWith.
    static bool EndsWith(const char* szString, const char* szEndsWith);
    //! Returns true if szString ends with the string given in szEndsWith. Ignores case.
    static bool EndsWith_NoCase(const char* szString, const char* szEndsWith);

    typedef bool (*AE_IS_WORD_DELIMITER)(char c, bool bFront);

    //! A default word delimiter function for English text.
    static bool IsWordDelimiter_English(char c, bool bFront);
    //! A default word delimiter function for identifiers in C code.
    static bool IsIdentifierDelimiter_C_Code(char c, bool bFront);

    //! Searches szString after position uiStartPos for the word szSearchFor. If IsDelimiterCB returns true for both characters in front and back of the word, the position is returned. Otherwise -1.
    static aeInt32 FindWholeWord(const char* szString, const char* szSearchFor, AE_IS_WORD_DELIMITER IsDelimiterCB, aeUInt32 uiStartPos = 0);
    //! Searches szString after position uiStartPos for the word szSearchFor. If IsDelimiterCB returns true for both characters in front and back of the word, the position is returned. Otherwise -1. Ignores case.
    static aeInt32 FindWholeWord_NoCase(const char* szString, const char* szSearchFor, AE_IS_WORD_DELIMITER IsDelimiterCB, aeUInt32 uiStartPos = 0);
  };
} // namespace AE_NS_FOUNDATION

#include "Inline/StringFunctions.inl"

#endif
