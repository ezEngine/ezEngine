#pragma once

EZ_ALWAYS_INLINE ezInt32 ezStringUtils::CompareChars(ezUInt32 uiCharacter1, ezUInt32 uiCharacter2)
{
  return (ezInt32)uiCharacter1 - (ezInt32)uiCharacter2;
}

inline ezInt32 ezStringUtils::CompareChars_NoCase(ezUInt32 uiCharacter1, ezUInt32 uiCharacter2)
{
  return (ezInt32)ToUpperChar(uiCharacter1) - (ezInt32)ToUpperChar(uiCharacter2);
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
EZ_ALWAYS_INLINE constexpr bool ezStringUtils::IsNullOrEmpty(const T* pString)
{
  return (pString == nullptr) || (pString[0] == '\0');
}

template <typename T>
EZ_ALWAYS_INLINE bool ezStringUtils::IsNullOrEmpty(const T* pString, const T* pStringEnd)
{
  return (pString == nullptr) || pString == pStringEnd || (pString[0] == '\0');
}

template <typename T>
EZ_ALWAYS_INLINE void ezStringUtils::UpdateStringEnd(const T* pStringStart, const T*& ref_pStringEnd)
{
  if (ref_pStringEnd != ezUnicodeUtils::GetMaxStringEnd<T>())
    return;

  ref_pStringEnd = pStringStart + GetStringElementCount(pStringStart, ezUnicodeUtils::GetMaxStringEnd<T>());
}

template <typename T>
constexpr ezUInt32 ezStringUtils::GetStringElementCount(const T* pString)
{
  // can't use strlen here as long as it's not constexpr (C++ 23)

  if (pString == nullptr)
    return 0;

  ezUInt32 uiCount = 0;
  while (*pString != '\0')
  {
    ++pString;
    ++uiCount;
  }

  return uiCount;
}

template <typename T>
ezUInt32 ezStringUtils::GetStringElementCount(const T* pString, const T* pStringEnd)
{
  if (IsNullOrEmpty(pString))
    return 0;

  if (pStringEnd != ezUnicodeUtils::GetMaxStringEnd<T>())
    return (ezUInt32)(pStringEnd - pString);

  ezUInt32 uiCount = 0;
  while ((pString < pStringEnd) && (*pString != '\0'))
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

  while ((szUtf8 < pStringEnd) && (*szUtf8 != '\0'))
  {
    // skip all the Utf8 continuation bytes
    if (!ezUnicodeUtils::IsUtf8ContinuationByte(*szUtf8))
      ++uiCharacters;

    ++szUtf8;
  }

  return uiCharacters;
}

inline void ezStringUtils::GetCharacterAndElementCount(const char* szUtf8, ezUInt32& ref_uiCharacterCount, ezUInt32& ref_uiElementCount, const char* pStringEnd)
{
  ref_uiCharacterCount = 0;
  ref_uiElementCount = 0;

  if (IsNullOrEmpty(szUtf8))
    return;

  while (szUtf8 < pStringEnd)
  {
    char uiByte = *szUtf8;
    if (uiByte == '\0')
    {
      break;
    }

    // skip all the Utf8 continuation bytes
    if (!ezUnicodeUtils::IsUtf8ContinuationByte(uiByte))
      ++ref_uiCharacterCount;

    ++szUtf8;
    ++ref_uiElementCount;
  }
}

EZ_ALWAYS_INLINE bool ezStringUtils::IsEqual(const char* pString1, const char* pString2, const char* pString1End, const char* pString2End)
{
  return ezStringUtils::Compare(pString1, pString2, pString1End, pString2End) == 0;
}

EZ_ALWAYS_INLINE bool ezStringUtils::IsEqualN(
  const char* pString1, const char* pString2, ezUInt32 uiCharsToCompare, const char* pString1End, const char* pString2End)
{
  return ezStringUtils::CompareN(pString1, pString2, uiCharsToCompare, pString1End, pString2End) == 0;
}

EZ_ALWAYS_INLINE bool ezStringUtils::IsEqual_NoCase(const char* pString1, const char* pString2, const char* pString1End, const char* pString2End)
{
  return ezStringUtils::Compare_NoCase(pString1, pString2, pString1End, pString2End) == 0;
}

EZ_ALWAYS_INLINE bool ezStringUtils::IsEqualN_NoCase(
  const char* pString1, const char* pString2, ezUInt32 uiCharsToCompare, const char* pString1End, const char* pString2End)
{
  return ezStringUtils::CompareN_NoCase(pString1, pString2, uiCharsToCompare, pString1End, pString2End) == 0;
}

EZ_ALWAYS_INLINE bool ezStringUtils::IsDecimalDigit(ezUInt32 uiChar)
{
  return (uiChar >= '0' && uiChar <= '9');
}

EZ_ALWAYS_INLINE bool ezStringUtils::IsHexDigit(ezUInt32 uiChar)
{
  return IsDecimalDigit(uiChar) || (uiChar >= 'A' && uiChar <= 'F') || (uiChar >= 'a' && uiChar <= 'f');
}
