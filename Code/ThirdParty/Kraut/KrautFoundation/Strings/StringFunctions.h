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

    //! Returns 0 if both strings are equal. A negative value if szString1 is smaller than szString2 and a positive value otherwise.
    static aeInt32 CompareAlphabetically(const char* szString1, const char* szString2);
    //! Returns 0 if the first n characters of both strings are equal. A negative value if szString1 is smaller than szString2 and a positive value otherwise.
    static aeInt32 CompareAlphabetically(const char* szString1, const char* szString2, aeUInt32 uiCharsToCompare);

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

    //! Searches for the first occurance of szStringToFind in szSource. Returns the index or -1 if nothing is found.
    static aeInt32 FindFirstStringPos(const char* szSource, const char* szStringToFind, aeUInt32 uiStartPos = 0);

    //! Returns true if szString starts with the string given in szStartsWith.
    static bool StartsWith(const char* szString, const char* szStartsWith);
    //! Returns true if szString ends with the string given in szEndsWith.
    static bool EndsWith(const char* szString, const char* szEndsWith);
  };
} // namespace AE_NS_FOUNDATION

#include "Inline/StringFunctions.inl"

#endif
