#define _CRT_SECURE_NO_WARNINGS 1

#include "../StringFunctions.h"
#include "../../Math/Math.h"
#include "../../Memory/Memory.h"

#pragma warning(disable:4996)

namespace AE_NS_FOUNDATION
{
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

  bool aeStringFunctions::StartsWith(const char* szString, const char* szStartsWith)
  {
    return (CompareEqual(szString, szStartsWith, GetLength(szStartsWith)));
  }

  bool aeStringFunctions::EndsWith(const char* szString, const char* szEndsWith)
  {
    const aeUInt32 uiLength1 = GetLength(szString);
    const aeUInt32 uiLength2 = GetLength(szEndsWith);

    if (uiLength1 < uiLength2)
      return (false);

    return (CompareEqual(&szString[uiLength1 - uiLength2], szEndsWith, uiLength2));
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
