#define _CRT_SECURE_NO_WARNINGS 1

#include "../StringFunctions.h"
#include "../../Math/Math.h"
#include "../../Memory/Memory.h"

#pragma warning(disable:4996)

namespace AE_NS_FOUNDATION
{

  aeUInt32 aeStringFunctions::Format(char* szDst, aeUInt32 uiDstSize, const char* szFormat, ...)
  {
    va_list ap;
    va_start(ap, szFormat);

    const aeUInt32 uiLength = FormatArgs(szDst, uiDstSize, szFormat, ap);

    va_end(ap);

    return (uiLength);
  }

  aeUInt32 aeStringFunctions::FormatArgs(char* szDst, aeUInt32 uiDstSize, const char* szFormat, va_list& ap)
  {
    AE_CHECK_DEV(szDst != nullptr, "aeStringFunctions::FormatArgs: Cannot work on a nullptr-String.");
    AE_CHECK_DEV(szFormat != nullptr, "aeStringFunctions::FormatArgs: Cannot work with a nullptr-String as Format.");

    return aeStringFunctions::vsnprintf(szDst, uiDstSize, szFormat, ap);
  }

  aeInt32 aeStringFunctions::FindFirstStringPos(const char* szSource, const char* szStringToFind, aeUInt32 uiStartPos)
  {
    // Handle nullptr-pointer strings
    if ((IsNullOrEmpty(szSource)) || (IsNullOrEmpty(szStringToFind)))
      return (-1);

    const aeInt32 iLen1 = aeStringFunctions::GetLength(szSource);
    const aeInt32 iLen2 = aeStringFunctions::GetLength(szStringToFind);

    if (iLen2 > iLen1)
      return (-1);

    aeInt32 iPos = uiStartPos;

    while (iPos <= (iLen1 - iLen2))
    {
      if (aeStringFunctions::CompareEqual(&szSource[iPos], szStringToFind, iLen2))
        return (iPos);

      ++iPos;
    }

    return (-1);
  }

  aeInt32 aeStringFunctions::FindFirstStringPos_NoCase(const char* szSource, const char* szStringToFind, aeUInt32 uiStartPos)
  {
    // Handle nullptr-pointer strings
    if ((IsNullOrEmpty(szSource)) || (IsNullOrEmpty(szStringToFind)))
      return (-1);

    const aeInt32 iLen1 = aeStringFunctions::GetLength(szSource);
    const aeInt32 iLen2 = aeStringFunctions::GetLength(szStringToFind);

    if (iLen2 > iLen1)
      return (-1);

    aeInt32 iPos = uiStartPos;

    while (iPos <= (iLen1 - iLen2))
    {
      if (aeStringFunctions::CompareEqual_NoCase(&szSource[iPos], szStringToFind, iLen2))
        return (iPos);

      ++iPos;
    }

    return (-1);
  }

  aeInt32 aeStringFunctions::FindLastStringPos(const char* szSource, const char* szStringToFind, aeUInt32 uiStartPos)
  {
    // Handle nullptr-pointer strings
    if ((IsNullOrEmpty(szSource)) || (IsNullOrEmpty(szStringToFind)))
      return (-1);

    const aeInt32 iLen1 = aeStringFunctions::GetLength(szSource);
    const aeInt32 iLen2 = aeStringFunctions::GetLength(szStringToFind);

    if (iLen2 > iLen1)
      return (-1);

    aeInt32 iPos = aeMath::Min<aeInt32>(uiStartPos, iLen1 - iLen2);

    while (iPos >= 0)
    {
      if (aeStringFunctions::CompareEqual(&szSource[iPos], szStringToFind, iLen2))
        return (iPos);

      --iPos;
    }

    return (-1);
  }

  aeInt32 aeStringFunctions::FindLastStringPos_NoCase(const char* szSource, const char* szStringToFind, aeUInt32 uiStartPos)
  {
    // Handle nullptr-pointer strings
    if ((IsNullOrEmpty(szSource)) || (IsNullOrEmpty(szStringToFind)))
      return (-1);

    const aeInt32 iLen1 = aeStringFunctions::GetLength(szSource);
    const aeInt32 iLen2 = aeStringFunctions::GetLength(szStringToFind);

    if (iLen2 > iLen1)
      return (-1);

    aeInt32 iPos = aeMath::Min<aeInt32>(uiStartPos, iLen1 - iLen2);

    while (iPos >= 0)
    {
      if (aeStringFunctions::CompareEqual_NoCase(&szSource[iPos], szStringToFind, iLen2))
        return (iPos);

      --iPos;
    }

    return (-1);
  }

  void aeStringFunctions::ToUpperCase(char* szString)
  {
    // Handle nullptr-pointer strings
    if (IsNullOrEmpty(szString))
      return;

    aeUInt32 len = 0;
    while (szString[len] != '\0')
    {
      szString[len] = ToUpperCase(szString[len]);
      ++len;
    }
  }

  void aeStringFunctions::ToLowerCase(char* szString)
  {
    // Handle nullptr-pointer strings
    if (IsNullOrEmpty(szString))
      return;

    aeUInt32 len = 0;
    while (szString[len] != '\0')
    {
      szString[len] = ToLowerCase(szString[len]);
      ++len;
    }
  }

  bool aeStringFunctions::StartsWith(const char* szString, const char* szStartsWith)
  {
    return (CompareEqual(szString, szStartsWith, GetLength(szStartsWith)));
  }

  bool aeStringFunctions::StartsWith_NoCase(const char* szString, const char* szStartsWith)
  {
    return (CompareEqual_NoCase(szString, szStartsWith, GetLength(szStartsWith)));
  }

  bool aeStringFunctions::EndsWith(const char* szString, const char* szEndsWith)
  {
    const aeUInt32 uiLength1 = GetLength(szString);
    const aeUInt32 uiLength2 = GetLength(szEndsWith);

    if (uiLength1 < uiLength2)
      return (false);

    return (CompareEqual(&szString[uiLength1 - uiLength2], szEndsWith, uiLength2));
  }

  bool aeStringFunctions::EndsWith_NoCase(const char* szString, const char* szEndsWith)
  {
    const aeUInt32 uiLength1 = GetLength(szString);
    const aeUInt32 uiLength2 = GetLength(szEndsWith);

    if (uiLength1 < uiLength2)
      return (false);

    return (CompareEqual_NoCase(&szString[uiLength1 - uiLength2], szEndsWith, uiLength2));
  }

  aeInt32 aeStringFunctions::FindWholeWord(const char* szString, const char* szSearchFor, AE_IS_WORD_DELIMITER IsDelimiterCB, aeUInt32 uiStartPos)
  {
    // Handle nullptr-pointer strings
    if ((IsNullOrEmpty(szString)) || (IsNullOrEmpty(szSearchFor)))
      return (-1);

    const aeUInt32 uiLen = GetLength(szSearchFor);

    aeUInt32 uiPos = uiStartPos;

    while (szString[uiPos] != '\0')
    {
      if (CompareEqual(&szString[uiPos], szSearchFor, uiLen))
      {
        if (((uiPos == 0) || (IsDelimiterCB(szString[uiPos - 1], true))) &&
            (IsDelimiterCB(szString[uiPos + uiLen], false)))
          return (uiPos);
      }

      ++uiPos;
    }

    return (-1);
  }

  aeInt32 aeStringFunctions::FindWholeWord_NoCase(const char* szString, const char* szSearchFor, AE_IS_WORD_DELIMITER IsDelimiterCB, aeUInt32 uiStartPos)
  {
    // Handle nullptr-pointer strings
    if ((IsNullOrEmpty(szString)) || (IsNullOrEmpty(szSearchFor)))
      return (-1);

    const aeUInt32 uiLen = GetLength(szSearchFor);

    aeUInt32 uiPos = uiStartPos;

    while (szString[uiPos] != '\0')
    {
      if (CompareEqual_NoCase(&szString[uiPos], szSearchFor, uiLen))
      {
        if (((uiPos == 0) || (IsDelimiterCB(szString[uiPos - 1], true))) &&
            (IsDelimiterCB(szString[uiPos + uiLen], false)))
          return (uiPos);
      }

      ++uiPos;
    }

    return (-1);
  }

  bool aeStringFunctions::IsWordDelimiter_English(char c, bool bFront)
  {
    if ((c >= 'a') && (c <= 'z'))
      return (false);
    if ((c >= 'A') && (c <= 'Z'))
      return (false);
    if ((c >= '0') && (c <= '9'))
      return (false);
    if (c == '_')
      return (false);
    if (c == '-')
      return (false);

    return (true);
  }

  bool aeStringFunctions::IsIdentifierDelimiter_C_Code(char c, bool bFront)
  {
    if ((c >= 'a') && (c <= 'z'))
      return (false);
    if ((c >= 'A') && (c <= 'Z'))
      return (false);
    if ((c >= '0') && (c <= '9'))
      return (false);
    if (c == '_')
      return (false);

    return (true);
  }


void aeStringFunctions::Copy(char* szDest, aeUInt32 uiDstSize, const char* szSource)
{
    // Handle nullptr-pointer strings
    if (IsNullOrEmpty (szSource))
    {
      szDest[0] = '\0';
      return;
    }

    AE_CHECK_DEV (uiDstSize > 0, "aeStringFunctions::Copy: Cannot copy into a Zero-Length-String.");
    AE_CHECK_DEV (szDest != nullptr, "aeStringFunctions::Copy: Cannot copy into a nullptr-String.");

    strncpy (szDest, szSource, uiDstSize - 1);
    szDest[uiDstSize - 1] = '\0';
}

void aeStringFunctions::Copy(char* szDest, aeUInt32 uiDstSize, const char* szSource, aeUInt32 uiCharsToCopy)
{
    // Handle nullptr-pointer strings
    if (IsNullOrEmpty (szSource))
    {
      szDest[0] = '\0';
      return;
    }

    AE_CHECK_DEV (uiDstSize > 0, "aeStringFunctions::Copy: Cannot copy into a Zero-Length-String.");
    AE_CHECK_DEV (szDest != nullptr, "aeStringFunctions::Copy: Cannot copy into a nullptr-String.");

    const aeUInt32 uiSize = aeMath::Min (uiCharsToCopy, uiDstSize - 1);
    strncpy (szDest, szSource, uiSize);
    szDest[uiSize] = '\0';
}

} // namespace AE_NS_FOUNDATION
