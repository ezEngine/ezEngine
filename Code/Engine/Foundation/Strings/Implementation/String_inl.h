#pragma once

template<ezUInt16 Size>
ezHybridString<Size>::ezHybridString()
{
  Clear();
}

template<ezUInt16 Size>
ezHybridString<Size>::ezHybridString(const ezHybridString& rhs)
{
  *this = rhs;
}

template<ezUInt16 Size>
ezHybridString<Size>::ezHybridString(const char* rhs)
{
  *this = rhs;
}

template<ezUInt16 Size>
ezHybridString<Size>::ezHybridString(const wchar_t* rhs)
{
  *this = rhs;
}

template<ezUInt16 Size>
ezHybridString<Size>::ezHybridString(const ezStringIterator& rhs)
{
  *this = rhs;
}

template<ezUInt16 Size>
ezHybridString<Size>::~ezHybridString()
{
}

template<ezUInt16 Size>
void ezHybridString<Size>::Clear()
{
  m_Data.SetCount(1);
  m_Data[0] = '\0';
  m_uiCharacterCount = 0;
}

template<ezUInt16 Size>
EZ_FORCE_INLINE const char* ezHybridString<Size>::GetData() const
{
  return &m_Data[0];
}

template<ezUInt16 Size>
EZ_FORCE_INLINE ezUInt32 ezHybridString<Size>::GetElementCount() const
{
  return m_Data.GetCount() - 1;
}

template<ezUInt16 Size>
EZ_FORCE_INLINE ezUInt32 ezHybridString<Size>::GetCharacterCount() const
{
  return m_uiCharacterCount;
}

template<ezUInt16 Size>
void ezHybridString<Size>::operator=(const char* szString)
{
  ezUInt32 uiElementCount = 0;
  ezStringUtils::GetCharacterAndElementCount(szString, m_uiCharacterCount, uiElementCount);
  m_Data.SetCount(uiElementCount + 1);
  ezStringUtils::Copy(&m_Data[0], uiElementCount + 1, szString);
}

template<ezUInt16 Size>
void ezHybridString<Size>::operator=(const ezHybridString& rhs)
{
  if (this == &rhs)
    return;

  m_uiCharacterCount = rhs.m_uiCharacterCount;
  m_Data = rhs.m_Data;
}

template<ezUInt16 Size>
void ezHybridString<Size>::operator=(const wchar_t* szString)
{
  ezStringUtf8 sConversion(szString);
  *this = sConversion.GetData();
}

template<ezUInt16 Size>
void ezHybridString<Size>::operator=(const ezStringIterator& rhs)
{
  m_Data.SetCount(rhs.GetElementCount() + 1);
  ezStringUtils::Copy(&m_Data[0], m_Data.GetCount() + 1, rhs.GetData(), rhs.GetEnd());
  m_uiCharacterCount = ezStringUtils::GetCharacterCount(GetData());
}

template<ezUInt16 Size>
ezStringIterator ezHybridString<Size>::GetIteratorFront() const
{
  return ezStringIterator(GetData(), GetData() + m_Data.GetCount() - 1, GetData(), m_uiCharacterCount + 1 == m_Data.GetCount());
}

template<ezUInt16 Size>
ezStringIterator ezHybridString<Size>::GetIteratorBack() const
{
  ezStringIterator it (GetData(), GetData() + m_Data.GetCount() - 1, GetData() + m_Data.GetCount() - 1, m_uiCharacterCount + 1 == m_Data.GetCount());
  it.ResetToBack();
  return it;
}

template<ezUInt16 Size>
ezStringIterator ezHybridString<Size>::GetSubString(ezUInt32 uiFirstCharacter, ezUInt32 uiNumCharacters) const
{
  EZ_ASSERT(uiFirstCharacter < m_uiCharacterCount, "The string only has %i characters, cannot start a sub-string at character %i.", m_uiCharacterCount, uiFirstCharacter);
  EZ_ASSERT(uiFirstCharacter + uiNumCharacters <= m_uiCharacterCount, "The string only has %i characters, cannot get a sub-string up to character %i.", m_uiCharacterCount, uiFirstCharacter + uiNumCharacters);

  const char* szStart = GetData();
  ezUnicodeUtils::MoveToNextUtf8(szStart, uiFirstCharacter);

  const char* szEnd = szStart;
  ezUnicodeUtils::MoveToNextUtf8(szEnd, uiNumCharacters);

  // we can actually determine whether at least the substring is only ASCII, yeah!
  return ezStringIterator(szStart, szEnd, szStart, (ezUInt32) (szEnd - szStart) == uiNumCharacters);
}

template<ezUInt16 Size>
ezStringIterator ezHybridString<Size>::GetFirst(ezUInt32 uiNumCharacters) const
{
  return GetSubString(0, uiNumCharacters);
}

template<ezUInt16 Size>
ezStringIterator ezHybridString<Size>::GetLast(ezUInt32 uiNumCharacters) const
{
  EZ_ASSERT(uiNumCharacters < m_uiCharacterCount, "The string only contains %i characters, cannot return the last %i characters.", m_uiCharacterCount, uiNumCharacters);
  return GetSubString(m_uiCharacterCount - uiNumCharacters, uiNumCharacters);
}
