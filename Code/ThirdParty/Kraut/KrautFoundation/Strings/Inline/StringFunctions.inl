#ifndef AE_FOUNDATION_STRINGS_STRINGFUNCTIONS_INL
#define AE_FOUNDATION_STRINGS_STRINGFUNCTIONS_INL

#include "../../Defines.h"
#include "../../Math/Math.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

namespace AE_NS_FOUNDATION
{
  // Macro to Handle nullptr-pointer strings
  #define AE_STRINGCOMPARE_HANDLE_NULL_PTRS(szString1, szString2, ret_true, ret_false)\
    if (szString1 == szString2)/* Handles the case that both are nullptr and that both are actually the same string */ \
      return (ret_true);\
    if (szString1 == nullptr) {\
      if (szString2[0] == '\0') /* if String1 is nullptr, String2 is never nullptr, otherwise the previous IF would have returned already */ \
        return (ret_true);\
      else\
        return (ret_false);\
    }\
    if (szString2 == nullptr) {\
      if (szString1[0] == '\0') /* if String2 is nullptr, String1 is never nullptr, otherwise the previous IF would have returned already */ \
        return (ret_true);\
      else\
        return (ret_false);\
    }

  inline bool aeStringFunctions::IsNullOrEmpty (const char* szString)
  {
    return ((szString == nullptr) || (szString[0] == '\0'));
  }

  inline aeUInt32 aeStringFunctions::GetLength (const char* szString)
  {
    // Handle nullptr-pointer strings and empty strings (which should be faster than just calling strlen)
    if (IsNullOrEmpty (szString))
      return (0);

    return ((aeUInt32) strlen (szString));
  }

  inline bool aeStringFunctions::CompareEqual (const char* szString1, const char* szString2)
  {
    AE_STRINGCOMPARE_HANDLE_NULL_PTRS (szString1, szString2, true, false);

    return (strcmp (szString1, szString2) == 0);
  }

  inline bool aeStringFunctions::CompareEqual (const char* szString1, const char* szString2, aeUInt32 uiCharsToCompare)
  {
    AE_STRINGCOMPARE_HANDLE_NULL_PTRS (szString1, szString2, true, false);

    return (strncmp (szString1, szString2, uiCharsToCompare) == 0);
  }

  inline aeInt32 aeStringFunctions::CompareAlphabetically (const char* szString1, const char* szString2)
  {
    AE_STRINGCOMPARE_HANDLE_NULL_PTRS (szString1, szString2, 0, 1);

    return (strcmp (szString1, szString2));
  }

  inline aeInt32 aeStringFunctions::CompareAlphabetically (const char* szString1, const char* szString2, aeUInt32 uiCharsToCompare)
  {
    AE_STRINGCOMPARE_HANDLE_NULL_PTRS (szString1, szString2, 0, 1);

    return (strncmp (szString1, szString2, uiCharsToCompare));
  }

  inline aeUInt32 aeStringFunctions::Concatenate (char* szDest, aeUInt32 uiDstSize, aeUInt32 uiDstStrLen, const char* szSource)
  {
    // Handle nullptr-pointer strings
    if (IsNullOrEmpty (szSource))
      return (uiDstStrLen);

    AE_CHECK_DEV (szDest != nullptr, "aeStringFunctions::Concatenate: Cannot concatenate to a nullptr-String.");

    aeUInt32 uiWritePos = uiDstStrLen;
    aeUInt32 uiReadPos = 0;
    while ((uiWritePos < uiDstSize-1) && (szSource[uiReadPos] != '\0'))
    {
      szDest[uiWritePos] = szSource[uiReadPos];

      ++uiReadPos;		
      ++uiWritePos;
    }

    szDest[uiWritePos] = '\0';
    return (uiWritePos);
  }

  inline aeUInt32 aeStringFunctions::Concatenate (char* szDest, aeUInt32 uiDstSize, aeUInt32 uiDstStrLen, const char* szSource, aeUInt32 uiCharsToCopy)
  {
    // Handle nullptr-pointer strings
    if (IsNullOrEmpty (szSource))
      return (uiDstStrLen);

    AE_CHECK_DEV (szDest != nullptr, "aeStringFunctions::Concatenate: Cannot concatenate to a nullptr-String.");

    aeUInt32 uiWritePos = uiDstStrLen;
    aeUInt32 uiReadPos = 0;
    while ((uiWritePos < uiDstSize-1) && (szSource[uiReadPos] != '\0') && (uiReadPos < uiCharsToCopy))
    {
      szDest[uiWritePos] = szSource[uiReadPos];

      ++uiReadPos;		
      ++uiWritePos;
    }

    szDest[uiWritePos] = '\0';
    return (uiWritePos);
  }

  inline aeUInt32 aeStringFunctions::Concatenate (char* szDest, aeUInt32 uiDstSize, const char* szSource)
  {
    // Handle nullptr-pointer strings
    if (IsNullOrEmpty (szSource))
      return (GetLength (szDest));

    return (Concatenate (szDest, uiDstSize, GetLength (szDest), szSource));
  }

  inline aeUInt32 aeStringFunctions::Concatenate (char* szDest, aeUInt32 uiDstSize, const char* szSource, aeUInt32 uiCharsToCopy)
  {
    // Handle nullptr-pointer strings
    if (IsNullOrEmpty (szSource))
      return (GetLength (szDest));

    return (Concatenate (szDest, uiDstSize, GetLength (szDest), szSource, uiCharsToCopy));
  }
}

#endif


