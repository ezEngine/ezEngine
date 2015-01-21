#include <Foundation/PCH.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Math/Math.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
#include <Foundation/Logging/Log.h>

ezAtomicInteger32 ezStringUtils::g_MaxUsedStringLength;
ezAtomicInteger32 ezStringUtils::g_UsedStringLengths[256];

void ezStringUtils::AddUsedStringLength(ezUInt32 uiLength)
{
  g_MaxUsedStringLength.Max(uiLength);

  if (uiLength > 255)
    uiLength = 255;

  g_UsedStringLengths[uiLength].Increment();
}

void ezStringUtils::PrintStringLengthStatistics()
{
  EZ_LOG_BLOCK("String Length Statistics");

  ezLog::Info("Max String Length: %i", (ezInt32) g_MaxUsedStringLength);

  ezUInt32 uiCopiedStrings = 0;
  for (ezUInt32 i = 0; i < 256; ++i)
    uiCopiedStrings += g_UsedStringLengths[i];

  ezLog::Info("Number of String Copies: %i", uiCopiedStrings);
  ezLog::Info("");

  ezUInt32 uiPercent = 0;
  ezUInt32 uiStrings = 0;
  for (ezUInt32 i = 0; i < 256; ++i)
  {
    if (100.0f * (uiStrings + g_UsedStringLengths[i]) / (float) uiCopiedStrings >= uiPercent)
    {
      ezLog::Info("%3i%% of all Strings are shorter than %3i Elements.", uiPercent, i + 1);
      uiPercent += 10;
    }

    uiStrings += g_UsedStringLengths[i];
  }
}

#endif

// Unicode ToUpper / ToLower character conversion
//  License: $(WEB www.boost.org/LICENSE_1_0.txt, Boost License 1.0).
//  Authors: $(WEB digitalmars.com, Walter Bright), Jonathan M Davis, and Kenji Hara
//  Source: $(PHOBOSSRC std/_uni.d)
ezUInt32 ezStringUtils::ToUpperChar(ezUInt32 wc)
{
  if (wc >= 'a' && wc <= 'z')
  {
    wc -= 'a' - 'A';
  }
  else if (wc >= 0x00E0)
  {
    if ((wc >= 0x00E0 && wc <= 0x00F6) ||
        (wc >= 0x00F8 && wc <= 0x00FE))
    {
      wc -= 32;
    }
    else if (wc == 0x00FF)
      wc = 0x0178;
    else if ((wc >= 0x0100 && wc < 0x0138) ||
             (wc >  0x0149 && wc < 0x0178))
    {
      if (wc == 0x0131)
      {
        // note:
        // this  maps the character into the ASCII range and thus changes its size in UTF-8 from
        // 2 bytes to 1 bytes, and this mapping is irreversible
        wc = 0x0049; // 'I'
      }
      else if (wc == 0x0130)
      {
        // note:
        // the character 0x130 maps to 'i' in ToLower, but usually would note get changed in ToUpper
        // however that means ToUpper(ToLower(0x130)) != ToUpper(0x130)
        // therefore, although this might "break the language", we ALWAYS convert it to the ASCII character
        // that would result if we would to ToUpper(ToLower(0x130))
        wc = 0x0049; // 'I'
      }
      else
        if (wc & 1)
          --wc;
    }
    else if ((wc >= 0x0139 && wc < 0x0149) ||
             (wc >  0x0178 && wc < 0x017F))
    {
      if ((wc & 1) == 0)
        --wc;
    }
    else if (wc == 0x017F)
    {
      // note:
      // this  maps the character into the ASCII range and thus changes its size in UTF-8 from
      // 2 bytes to 1 bytes, and this mapping is irreversible
      wc = 0x0053; // 'S'

      // this one character means, that for case-insensitive comparisons we always need to use ToUpper
      // and NOT ToLower, as ToLower will not convert this one character, such that two strings, one with 0x017f 
      // and one with 0x0053, will not compare equal
    }
    else if (wc >= 0x0200 && wc <= 0x0217)
    {
      if (wc & 1)
        --wc;
    }
    else if (wc >= 0x0430 && wc <= 0x044F)
      wc -= 32;
    else if ((wc >= 0x0451 && wc <= 0x045C) ||
             (wc >= 0x045E && wc <= 0x045F))
    {
      wc -= 80;
    }
    else if (wc >= 0x0460 && wc <= 0x047F)
    {
      if (wc & 1)
        --wc;
    }
    else if (wc >= 0x0561 && wc < 0x0587)
      wc -= 48;
    else if (wc >= 0xFF41 && wc <= 0xFF5A)
      wc -= 32;
  }

  return wc;
}

// Unicode ToUpper / ToLower character conversion
//  License: $(WEB www.boost.org/LICENSE_1_0.txt, Boost License 1.0).
//  Authors: $(WEB digitalmars.com, Walter Bright), Jonathan M Davis, and Kenji Hara
//  Source: $(PHOBOSSRC std/_uni.d)
ezUInt32 ezStringUtils::ToLowerChar(ezUInt32 wc)
{
  if (wc >= 'A' && wc <= 'Z')
  {
    wc += 'a' - 'A';
  }
  else if (wc >= 0x00C0)
  {
    if ((wc >= 0x00C0 && wc <= 0x00D6) ||
        (wc >= 0x00D8 && wc <= 0x00DE))
    {
      wc += 32;
    }
    else if ((wc >= 0x0100 && wc < 0x0138) ||
             (wc > 0x0149 && wc < 0x0178))
    {
      if (wc == 0x0130)
      {
        // note:
        // this  maps the character into the ASCII range and thus changes its size in UTF-8 from
        // 2 bytes to 1 bytes, and this mapping is irreversible
        wc = 0x0069; // 'i'
      }
      else if (wc == 0x0131)
      {
        // note:
        // the character 0x131 maps to 'I' in ToUpper, but usually would note get changed in ToLower
        // however that means ToLower(ToUpper(0x131)) != ToLower(0x131)
        // therefore, although this might "break the language", we ALWAYS convert it to the ASCII character
        // that would result if we would to ToLower(ToUpper(0x131))
        wc = 0x0069; // 'i'
      }
      else if ((wc & 1) == 0)
        ++wc;
    }
    else if (wc == 0x0178)
      wc = 0x00FF;
    else if ((wc >= 0x0139 && wc < 0x0149) ||
             (wc > 0x0178 && wc < 0x017F))
    {
      if (wc & 1)
        ++wc;
    }
    else if (wc >= 0x0200 && wc <= 0x0217)
    {
      if ((wc & 1) == 0)
        ++wc;
    }
    else if ((wc >= 0x0401 && wc <= 0x040C) ||
             (wc >= 0x040E && wc <= 0x040F))
    {
      wc += 80;
    }
    else if (wc >= 0x0410 && wc <= 0x042F)
      wc += 32;
    else if (wc >= 0x0460 && wc <= 0x047F)
    {
      if ((wc & 1) == 0)
        ++wc;
    }
    else if (wc >= 0x0531 && wc <= 0x0556)
      wc += 48;
    else if (wc >= 0x10A0 && wc <= 0x10C5)
      wc += 48;
    else if (wc >= 0xFF21 && wc <= 0xFF3A)
      wc += 32;
  }

  return wc;
}


ezUInt32 ezStringUtils::ToUpperString(char* pString)
{
  char* pWriteStart = pString;
  const char* pReadStart = pString;

  while (*pReadStart != '\0')
  {
    const ezUInt32 uiChar = ezUnicodeUtils::DecodeUtf8ToUtf32(pReadStart);
    const ezUInt32 uiCharUpper = ezStringUtils::ToUpperChar(uiChar);
    pWriteStart = utf8::unchecked::utf32to8(&uiCharUpper, &uiCharUpper + 1, pWriteStart);
  }

  *pWriteStart = '\0';

  const ezUInt32 uiNewStringLength = (ezUInt32) (pWriteStart - pString);
  return uiNewStringLength;
}

ezUInt32 ezStringUtils::ToLowerString(char* pString)
{
  char* pWriteStart = pString;
  const char* pReadStart = pString;

  while (*pReadStart != '\0')
  {
    const ezUInt32 uiChar = ezUnicodeUtils::DecodeUtf8ToUtf32(pReadStart);
    const ezUInt32 uiCharUpper = ezStringUtils::ToLowerChar(uiChar);
    pWriteStart = utf8::unchecked::utf32to8(&uiCharUpper, &uiCharUpper + 1, pWriteStart);
  }

  *pWriteStart = '\0';

  const ezUInt32 uiNewStringLength = (ezUInt32) (pWriteStart - pString);
  return uiNewStringLength;
}

// Macro to Handle nullptr-pointer strings
#define EZ_STRINGCOMPARE_HANDLE_NULL_PTRS(szString1, szString2, ret_equal, ret_str2_larger, ret_str1_larger, szString1End, szString2End)\
  if (szString1 == szString2) /* Handles the case that both are nullptr and that both are actually the same string */ \
    {\
    if ((szString1 == nullptr) || (szString1End == szString2End)) /* if both are nullptr, ignore the end pointer, otherwise the strings are equal, if both end pointers are also the same */ \
      return (ret_equal);\
    }\
  if (szString1 == nullptr)\
    {\
    if (szString2[0] == '\0') /* if String1 is nullptr, String2 is never nullptr, otherwise the previous IF would have returned already */ \
      return (ret_equal);\
        else\
      return (ret_str2_larger);\
    }\
  if (szString2 == nullptr) \
    {\
    if (szString1[0] == '\0') /* if String2 is nullptr, String1 is never nullptr, otherwise the previous IF would have returned already */ \
      return (ret_equal);\
        else\
      return (ret_str1_larger);\
    }

#define ToSignedInt(c) ((ezInt32) ((unsigned char) c))

ezInt32 ezStringUtils::Compare(const char* pString1, const char* pString2, const char* pString1End, const char* pString2End)
{
  EZ_STRINGCOMPARE_HANDLE_NULL_PTRS(pString1, pString2, 0, -1, 1, pString1End, pString2End);

  while ((*pString1 != '\0') && (*pString2 != '\0') && (pString1 < pString1End) && (pString2 < pString2End))
  {
    if (*pString1 != *pString2)
      return ToSignedInt(*pString1) - ToSignedInt(*pString2);

    ++pString1;
    ++pString2;
  }

  if (pString1 >= pString1End)
  {
    if (pString2 >= pString2End)
      return 0;

    return -ToSignedInt(*pString2); // either also '\0' (end) or not 0, thus 'smaller' than pString1 (negated)
  }
  else
  {
    if (pString2 >= pString2End)
      return ToSignedInt(*pString1); // either also '\0' (end) or not 0, thus 'larger' than pString2

    return ToSignedInt(*pString1) - ToSignedInt(*pString2);
  }
}

ezInt32 ezStringUtils::CompareN(const char* pString1, const char* pString2, ezUInt32 uiCharsToCompare, const char* pString1End, const char* pString2End)
{
  if (uiCharsToCompare == 0)
    return 0;

  EZ_STRINGCOMPARE_HANDLE_NULL_PTRS(pString1, pString2, 0, -1, 1, pString1End, pString2End);

  while ((*pString1 != '\0') && (*pString2 != '\0') && (uiCharsToCompare > 0) && (pString1 < pString1End) && (pString2 < pString2End))
  {
    if (*pString1 != *pString2)
      return ToSignedInt(*pString1) - ToSignedInt(*pString2);

    if (!ezUnicodeUtils::IsUtf8ContinuationByte(*pString1))
      --uiCharsToCompare;

    ++pString1;
    ++pString2;
  }

  if (uiCharsToCompare == 0)
    return 0;

  if (pString1 >= pString1End)
  {
    if (pString2 >= pString2End)
      return 0; // both reached their end pointer

    return -ToSignedInt(*pString2); // either also '\0' (end) or not 0, thus 'smaller' than pString1 (negated)
  }
  else
  {
    if (pString2 >= pString2End)
      return ToSignedInt(*pString1); // either also '\0' (end) or not 0, thus 'larger' than pString2

    return ToSignedInt(*pString1) - ToSignedInt(*pString2);
  }
}

ezInt32 ezStringUtils::Compare_NoCase(const char* pString1, const char* pString2, const char* pString1End, const char* pString2End)
{
  EZ_STRINGCOMPARE_HANDLE_NULL_PTRS(pString1, pString2, 0, -1, 1, pString1End, pString2End);

  while ((*pString1 != '\0') && (*pString2 != '\0') && (pString1 < pString1End) && (pString2 < pString2End))
  {
    // utf8::next will already advance the iterators
    const ezUInt32 uiChar1 = ezUnicodeUtils::DecodeUtf8ToUtf32(pString1);
    const ezUInt32 uiChar2 = ezUnicodeUtils::DecodeUtf8ToUtf32(pString2);

    const ezInt32 iComparison = CompareChars_NoCase(uiChar1, uiChar2);

    if (iComparison != 0)
      return iComparison;
  }


  if (pString1 >= pString1End)
  {
    if (pString2 >= pString2End)
      return 0; // both reached their end pointer

    return -ToSignedInt(*pString2); // either also '\0' (end) or not 0, thus 'smaller' than pString1 (negated)
  }
  else
  {
    if (pString2 >= pString2End)
      return ToSignedInt(*pString1); // either also '\0' (end) or not 0, thus 'larger' than pString2

    // none of them has reached their end pointer, but at least one is 0, so no need to ToUpper
    return ToSignedInt(*pString1) - ToSignedInt(*pString2);
  }
}

ezInt32 ezStringUtils::CompareN_NoCase(const char* pString1, const char* pString2, ezUInt32 uiCharsToCompare, const char* pString1End, const char* pString2End)
{
  if (uiCharsToCompare == 0)
    return 0;

  EZ_STRINGCOMPARE_HANDLE_NULL_PTRS(pString1, pString2, 0, -1, 1, pString1End, pString2End);

  while ((*pString1 != '\0') && (*pString2 != '\0') && (uiCharsToCompare > 0) && (pString1 < pString1End) && (pString2 < pString2End))
  {
    // utf8::next will already advance the iterators
    const ezUInt32 uiChar1 = ezUnicodeUtils::DecodeUtf8ToUtf32(pString1);
    const ezUInt32 uiChar2 = ezUnicodeUtils::DecodeUtf8ToUtf32(pString2);

    const ezInt32 iComparison = CompareChars_NoCase(uiChar1, uiChar2);

    if (iComparison != 0)
      return iComparison;

    --uiCharsToCompare;
  }

  if (uiCharsToCompare == 0)
    return 0;

  if (pString1 >= pString1End)
  {
    if (pString2 >= pString2End)
      return 0; // both reached their end pointer

    return -ToSignedInt(*pString2); // either also '\0' (end) or not 0, thus 'smaller' than pString1 (negated)
  }
  else
  {
    if (pString2 >= pString2End)
      return ToSignedInt(*pString1); // either also '\0' (end) or not 0, thus 'larger' than pString2

    // none of them has reached their end pointer, but at least one is 0, so no need to ToUpper
    return ToSignedInt(*pString1) - ToSignedInt(*pString2);
  }
}

ezUInt32 ezStringUtils::Copy(char* szDest, ezUInt32 uiDstSize, const char* szSource, const char* pSourceEnd)
{
  EZ_ASSERT_DEBUG(szDest != nullptr && uiDstSize > 0, "Invalid output buffer.");

  if (IsNullOrEmpty(szSource))
  {
    ezStringUtils::AddUsedStringLength(0);
    szDest[0] = '\0';
    return 0;
  }

  ezUInt32 uiSourceLen = static_cast<ezUInt32>((pSourceEnd == ezMaxStringEnd) ? strlen(szSource) : pSourceEnd - szSource);
  ezUInt32 uiBytesToCopy = ezMath::Min(uiDstSize - 1, uiSourceLen);

  // simply copy all bytes
  memcpy(szDest, szSource, uiBytesToCopy);

  // We might have copied half of a UTF8 character so fix this now
  char* szLastCharacterPos = szDest + uiBytesToCopy;
  const char* szLastByteNotCopied = szSource + uiBytesToCopy;

  // did we cut of a UTF8 character?
  if (ezUnicodeUtils::IsUtf8ContinuationByte(*szLastByteNotCopied))
  {
    // if so fix it
    szLastByteNotCopied--; szLastCharacterPos--;
    while (ezUnicodeUtils::IsUtf8ContinuationByte(*szLastByteNotCopied))
    {
      szLastByteNotCopied--; szLastCharacterPos--;
    }
  }

  // this will actually overwrite the last byte that we wrote into the output buffer 
  *szLastCharacterPos = '\0';

  const ezUInt32 uiLength = (ezUInt32)(szLastCharacterPos - szDest);

  ezStringUtils::AddUsedStringLength(uiLength);
  return uiLength;
}

ezUInt32 ezStringUtils::CopyN(char* szDest, ezUInt32 uiDstSize, const char* szSource, ezUInt32 uiCharsToCopy, const char* pSourceEnd)
{
  EZ_ASSERT_DEBUG(szDest != nullptr && uiDstSize > 0, "Invalid output buffer.");

  if (IsNullOrEmpty(szSource))
  {
    ezStringUtils::AddUsedStringLength(0);
    szDest[0] = '\0';
    return 0;
  }

  const char* szStartPos = szDest;

  char* szLastCharacterPos = szDest;

  ezInt32 iCharsCopied = -1;

  while (uiDstSize > 0)
  {
    if ((*szSource == '\0') || (szSource >= pSourceEnd))
    {
      szLastCharacterPos = szDest;
      break;
    }

    if (!ezUnicodeUtils::IsUtf8ContinuationByte(*szSource))
    {
      // if this is not a continuation byte, we have copied another character into the output buffer
      ++iCharsCopied;

      // keep track of where we started writing the current character
      // because if the output buffer is not large enough, to hold the entire UTF-8 sequence, we must truncate the string
      // at the last character boundary
      szLastCharacterPos = szDest;

      // if we successfully copied enough characters, the only thing left is to terminate the string
      if (iCharsCopied == uiCharsToCopy)
        break;
    }

    *szDest = *szSource;
    ++szSource;
    ++szDest;
    --uiDstSize;
  }

  // this will actually overwrite the last byte that we wrote into the output buffer
  *szLastCharacterPos = '\0';

  const ezUInt32 uiLength = (ezUInt32) (szLastCharacterPos - szStartPos);

  ezStringUtils::AddUsedStringLength(uiLength);
  return uiLength;
}

bool ezStringUtils::StartsWith(const char* szString, const char* szStartsWith, const char* pStringEnd)
{
  if (IsNullOrEmpty(szStartsWith))
    return true;
  if (IsNullOrEmpty(szString))
    return false;

  while ((*szString != '\0') && (szString < pStringEnd))
  {
    // if we have reached the end of the StartsWith string, the other string DOES start with it
    if (*szStartsWith == '\0')
      return true;

    if (*szStartsWith != *szString)
      return false;

    ++szString;
    ++szStartsWith;
  }

  // if both are equally long, this comparison will return true
  return (*szStartsWith == '\0');
}

bool ezStringUtils::StartsWith_NoCase(const char* szString, const char* szStartsWith, const char* pStringEnd)
{
  if (IsNullOrEmpty(szStartsWith))
    return true;
  if (IsNullOrEmpty(szString))
    return false;

  while ((*szString != '\0') && (szString < pStringEnd))
  {
    // if we have reached the end of the StartsWith string, the other string DOES start with it
    if (*szStartsWith == '\0')
      return true;

    if (ezStringUtils::CompareChars_NoCase(szStartsWith, szString) != 0)
      return false;

    ezUnicodeUtils::MoveToNextUtf8(szString);
    ezUnicodeUtils::MoveToNextUtf8(szStartsWith);
  }

  // if both are equally long, this comparison will return true
  return (*szStartsWith == '\0');
}

bool ezStringUtils::EndsWith(const char* szString, const char* szEndsWith, const char* pStringEnd)
{
  if (IsNullOrEmpty(szEndsWith))
    return true;
  if (IsNullOrEmpty(szString))
    return false;

  const ezUInt32 uiLength1 = ezStringUtils::GetStringElementCount(szString, pStringEnd);
  const ezUInt32 uiLength2 = ezStringUtils::GetStringElementCount(szEndsWith);

  if (uiLength1 < uiLength2)
    return false;

  return IsEqual(&szString[uiLength1 - uiLength2], szEndsWith, pStringEnd);
}

bool ezStringUtils::EndsWith_NoCase(const char* szString, const char* szEndsWith, const char* pStringEnd)
{
  if (IsNullOrEmpty(szEndsWith))
    return true;
  if (IsNullOrEmpty(szString))
    return false;

  const ezUInt32 uiLength1 = ezStringUtils::GetStringElementCount(szString, pStringEnd);
  const ezUInt32 uiLength2 = ezStringUtils::GetStringElementCount(szEndsWith);

  const char* pCur1 = szString + uiLength1; // points to \0
  const char* pCur2 = szEndsWith + uiLength2; // points to \0

  while (pCur1 > szString)
  {
    // if that string has reached its beginning, all comparisons succeeded and szString does end with szEndsWith
    if (pCur2 <= szEndsWith)
      return true;

    // move to the previous character
    ezUnicodeUtils::MoveToPriorUtf8(pCur1);
    ezUnicodeUtils::MoveToPriorUtf8(pCur2);

    if (ezStringUtils::CompareChars_NoCase(pCur1, pCur2) != 0)
      return false;
  }

  // we have reached the beginning of szString
  // but we may have simultaneously reached the beginning of szEndsWith
  // so if pCur2 has reached the beginning of szEndsWith, both strings are equal, otherwise szString does not end with szEndsWith
  return (pCur2 <= szEndsWith);
}

const char* ezStringUtils::FindSubString(const char* szSource, const char* szStringToFind, const char* pSourceEnd)
{
  // Handle nullptr-pointer strings
  if ((IsNullOrEmpty(szSource)) || (IsNullOrEmpty(szStringToFind)))
    return nullptr;

  const char* pCurPos = &szSource[0];

  while ((*pCurPos != '\0') && (pCurPos < pSourceEnd))
  {
    if (ezStringUtils::StartsWith(pCurPos, szStringToFind, pSourceEnd))
      return pCurPos;

    ezUnicodeUtils::MoveToNextUtf8(pCurPos);
  }

  return nullptr;
}

const char* ezStringUtils::FindSubString_NoCase(const char* szSource, const char* szStringToFind, const char* pSourceEnd)
{
  // Handle nullptr-pointer strings
  if ((IsNullOrEmpty(szSource)) || (IsNullOrEmpty(szStringToFind)))
    return nullptr;

  const char* pCurPos = &szSource[0];

  while ((*pCurPos != '\0') && (pCurPos < pSourceEnd))
  {
    if (ezStringUtils::StartsWith_NoCase(pCurPos, szStringToFind, pSourceEnd))
      return pCurPos;

    ezUnicodeUtils::MoveToNextUtf8(pCurPos);
  }

  return nullptr;
}


const char* ezStringUtils::FindLastSubString(const char* szSource, const char* szStringToFind, const char* szStartSearchAt, const char* pSourceEnd)
{
  // Handle nullptr-pointer strings
  if ((IsNullOrEmpty(szSource)) || (IsNullOrEmpty(szStringToFind)))
    return nullptr;

  // get the last element (actually the \0 terminator)
  if (szStartSearchAt == nullptr)
    szStartSearchAt = szSource + ezStringUtils::GetStringElementCount(szSource, pSourceEnd);

  // while we haven't reached the stars .. erm, start
  while (szStartSearchAt > szSource)
  {
    ezUnicodeUtils::MoveToPriorUtf8(szStartSearchAt);

    if (ezStringUtils::StartsWith(szStartSearchAt, szStringToFind, pSourceEnd))
      return szStartSearchAt;
  }

  return nullptr;
}

const char* ezStringUtils::FindLastSubString_NoCase(const char* szSource, const char* szStringToFind, const char* szStartSearchAt, const char* pSourceEnd)
{
  // Handle nullptr-pointer strings
  if ((IsNullOrEmpty(szSource)) || (IsNullOrEmpty(szStringToFind)))
    return nullptr;

  if (szStartSearchAt == nullptr)
    szStartSearchAt = szSource + ezStringUtils::GetStringElementCount(szSource, pSourceEnd);

  while (szStartSearchAt > szSource)
  {
    ezUnicodeUtils::MoveToPriorUtf8(szStartSearchAt);

    if (ezStringUtils::StartsWith_NoCase(szStartSearchAt, szStringToFind, pSourceEnd))
      return szStartSearchAt;
  }

  return nullptr;
}


const char* ezStringUtils::FindWholeWord(const char* szString, const char* szSearchFor, EZ_CHARACTER_FILTER IsDelimiterCB, const char* pStringEnd)
{
  // Handle nullptr-pointer strings
  if ((IsNullOrEmpty(szString)) || (IsNullOrEmpty(szSearchFor)))
    return nullptr;

  const ezUInt32 uiSearchedWordLength = GetStringElementCount(szSearchFor);

  const char* pPrevPos = nullptr;
  const char* pCurPos = szString;

  while ((*pCurPos != '\0') && (pCurPos < pStringEnd))
  {
    if (StartsWith(pCurPos, szSearchFor, pStringEnd)) // yay, we found a substring, now make sure it is a 'whole word'
    {
      if (((szString == pCurPos) || // the start of the string is always a word delimiter
        (IsDelimiterCB(ezUnicodeUtils::ConvertUtf8ToUtf32(pPrevPos)/* front */))) && // make sure the character before this substring is a word delimiter
        ((pCurPos + uiSearchedWordLength >= pStringEnd) || // the end of the string is also always a delimiter
        (IsDelimiterCB(ezUnicodeUtils::ConvertUtf8ToUtf32(pCurPos + uiSearchedWordLength)/* back */)))) // and the character after it, as well
        return pCurPos;
    }

    pPrevPos = pCurPos;
    ezUnicodeUtils::MoveToNextUtf8(pCurPos);
  }

  return nullptr;
}

const char* ezStringUtils::FindWholeWord_NoCase(const char* szString, const char* szSearchFor, EZ_CHARACTER_FILTER IsDelimiterCB, const char* pStringEnd)
{
  // Handle nullptr-pointer strings
  if ((IsNullOrEmpty(szString)) || (IsNullOrEmpty(szSearchFor)))
    return nullptr;

  const ezUInt32 uiSearchedWordLength = GetStringElementCount(szSearchFor);

  const char* pPrevPos = nullptr;
  const char* pCurPos = szString;

  while ((*pCurPos != '\0') && (pCurPos < pStringEnd))
  {
    if (StartsWith_NoCase(pCurPos, szSearchFor, pStringEnd)) // yay, we found a substring, now make sure it is a 'whole word'
    {
      if (((szString == pCurPos) || // the start of the string is always a word delimiter
        (IsDelimiterCB(ezUnicodeUtils::ConvertUtf8ToUtf32(pPrevPos)/* front */))) && // make sure the character before this substring is a word delimiter
        (IsDelimiterCB(ezUnicodeUtils::ConvertUtf8ToUtf32(pCurPos + uiSearchedWordLength)/* back */))) // and the character after it, as well
        return pCurPos;
    }

    pPrevPos = pCurPos;
    ezUnicodeUtils::MoveToNextUtf8(pCurPos);
  }

  return nullptr;
}

const char* ezStringUtils::SkipCharacters(const char* szString, EZ_CHARACTER_FILTER SkipCharacterCB, bool bAlwaysSkipFirst)
{
  EZ_ASSERT_DEBUG(szString != nullptr, "Invalid string");

  while (*szString != '\0')
  {
    if (!bAlwaysSkipFirst && !SkipCharacterCB(ezUnicodeUtils::ConvertUtf8ToUtf32(szString)))
      break;

    bAlwaysSkipFirst = false;
    ezUnicodeUtils::MoveToNextUtf8(szString);
  }

  return szString;
}

const char* ezStringUtils::FindWordEnd(const char* szString, EZ_CHARACTER_FILTER IsDelimiterCB, bool bAlwaysSkipFirst)
{
  EZ_ASSERT_DEBUG(szString != nullptr, "Invalid string");

  while (*szString != '\0')
  {
    if (!bAlwaysSkipFirst && IsDelimiterCB(ezUnicodeUtils::ConvertUtf8ToUtf32(szString)))
      break;

    bAlwaysSkipFirst = false;
    ezUnicodeUtils::MoveToNextUtf8(szString);
  }

  return szString;
}

bool ezStringUtils::IsWhiteSpace(ezUInt32 c)
{
  return (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v');
}

bool ezStringUtils::IsWordDelimiter_English(ezUInt32 uiChar)
{
  if ((uiChar >= 'a') && (uiChar <= 'z'))
    return false;
  if ((uiChar >= 'A') && (uiChar <= 'Z'))
    return false;
  if ((uiChar >= '0') && (uiChar <= '9'))
    return false;
  if (uiChar == '_')
    return false;
  if (uiChar == '-')
    return false;

  return true;
}

bool ezStringUtils::IsIdentifierDelimiter_C_Code(ezUInt32 uiChar)
{
  if ((uiChar >= 'a') && (uiChar <= 'z'))
    return false;
  if ((uiChar >= 'A') && (uiChar <= 'Z'))
    return false;
  if ((uiChar >= '0') && (uiChar <= '9'))
    return false;
  if (uiChar == '_')
    return false;

  return true;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_StringUtils);
