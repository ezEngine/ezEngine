#pragma once

#include <Foundation/Math/Math.h>

EZ_FORCE_INLINE ezInt32 ezStringUtils::CompareChars(ezUInt32 uiCharacter1, ezUInt32 uiCharacter2)
{
  return (ezInt32) uiCharacter1 - (ezInt32) uiCharacter2;
}

inline ezInt32 ezStringUtils::CompareChars_NoCase(ezUInt32 uiCharacter1, ezUInt32 uiCharacter2)
{
  return (ezInt32) ToUpperChar(uiCharacter1) - (ezInt32) ToUpperChar(uiCharacter2);
}

inline ezInt32 ezStringUtils::CompareChars(const char* szUtf8Char1, const char* szUtf8Char2)
{
  return CompareChars(ezUnicodeUtils::ConvertUtf8ToUtf32(szUtf8Char1), ezUnicodeUtils::ConvertUtf8ToUtf32(szUtf8Char2));
}

inline ezInt32 ezStringUtils::CompareChars_NoCase(const char* szUtf8Char1, const char* szUtf8Char2)
{
  return CompareChars_NoCase(ezUnicodeUtils::ConvertUtf8ToUtf32(szUtf8Char1), ezUnicodeUtils::ConvertUtf8ToUtf32(szUtf8Char2));
}

template <typename T>
EZ_FORCE_INLINE bool ezStringUtils::IsNullOrEmpty(const T* pString)
{
  return (pString == nullptr) || (pString[0] == '\0');
}

template <typename T>
EZ_FORCE_INLINE bool ezStringUtils::IsNullOrEmpty(const T* pString, const T* pStringEnd)
{
  return (pString == nullptr) || (pString[0] == '\0') || pString == pStringEnd;
}

template <typename T>
EZ_FORCE_INLINE void ezStringUtils::UpdateStringEnd(const T* szStringStart, const T*& szStringEnd)
{
  if (szStringEnd != ezMaxStringEnd)
    return;

  szStringEnd = szStringStart + GetStringElementCount(szStringStart, ezMaxStringEnd);
}


template <typename T>
ezUInt32 ezStringUtils::GetStringElementCount(const T* pString, const T* pStringEnd)
{
  if (IsNullOrEmpty(pString))
    return 0;

  if (pStringEnd != (const T*) ezMaxStringEnd)
    return (ezUInt32) (pStringEnd - pString);

  ezUInt32 uiCount = 0;
  while ((*pString != '\0') && (pString < pStringEnd))
  {
    ++pString;
    ++uiCount;
  }

  return uiCount;
}

inline ezUInt32 ezStringUtils::GetCharacterCount(const char* szUtf8, const char* pStringEnd)
{
  if (IsNullOrEmpty(szUtf8))
    return 0;

  ezUInt32 uiCharacters = 0;

  while ((*szUtf8 != '\0') && (szUtf8 < pStringEnd))
  {
    // skip all the Utf8 continuation bytes
    if (!ezUnicodeUtils::IsUtf8ContinuationByte(*szUtf8))
      ++uiCharacters;

    ++szUtf8;
  }

  return uiCharacters;
}

inline void ezStringUtils::GetCharacterAndElementCount(const char* szUtf8, ezUInt32& uiCharacterCount, ezUInt32& uiElementCount, const char* pStringEnd)
{
  uiCharacterCount = 0;
  uiElementCount = 0;

  if (IsNullOrEmpty(szUtf8))
    return;

  while ((*szUtf8 != '\0') && (szUtf8 < pStringEnd))
  {
    // skip all the Utf8 continuation bytes
    if (!ezUnicodeUtils::IsUtf8ContinuationByte(*szUtf8))
      ++uiCharacterCount;

    ++szUtf8;
    ++uiElementCount;
  }
}

EZ_FORCE_INLINE bool ezStringUtils::IsEqual(const char* pString1, const char* pString2, const char* pString1End, const char* pString2End)
{
  return ezStringUtils::Compare(pString1, pString2, pString1End, pString2End) == 0;
}

EZ_FORCE_INLINE bool ezStringUtils::IsEqualN(const char* pString1, const char* pString2, ezUInt32 uiCharsToCompare, const char* pString1End, const char* pString2End)
{
  return ezStringUtils::CompareN(pString1, pString2, uiCharsToCompare, pString1End, pString2End) == 0;
}

EZ_FORCE_INLINE bool ezStringUtils::IsEqual_NoCase(const char* pString1, const char* pString2, const char* pString1End, const char* pString2End)
{
  return ezStringUtils::Compare_NoCase(pString1, pString2, pString1End, pString2End) == 0;
}

EZ_FORCE_INLINE bool ezStringUtils::IsEqualN_NoCase(const char* pString1, const char* pString2, ezUInt32 uiCharsToCompare, const char* pString1End, const char* pString2End)
{
  return ezStringUtils::CompareN_NoCase(pString1, pString2, uiCharsToCompare, pString1End, pString2End) == 0;
}

