#pragma once

template<ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(ezIAllocator* pAllocator) :
  m_Data(pAllocator)
{
  Clear();
}

template<ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(const ezHybridStringBase& rhs, ezIAllocator* pAllocator) :
  m_Data(pAllocator)
{
  *this = rhs;
}

template<ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(const char* rhs, ezIAllocator* pAllocator) :
  m_Data(pAllocator)
{
  *this = rhs;
}

template<ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(const wchar_t* rhs, ezIAllocator* pAllocator) :
  m_Data(pAllocator)
{
  *this = rhs;
}

template<ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(const ezStringIterator& rhs, ezIAllocator* pAllocator) :
  m_Data(pAllocator)
{
  *this = rhs;
}

template<ezUInt16 Size>
ezHybridStringBase<Size>::~ezHybridStringBase()
{
}

template<ezUInt16 Size>
void ezHybridStringBase<Size>::Clear()
{
  m_Data.SetCount(1);
  m_Data[0] = '\0';
  m_uiCharacterCount = 0;
}

template<ezUInt16 Size>
EZ_FORCE_INLINE const char* ezHybridStringBase<Size>::GetData() const
{
  return &m_Data[0];
}

template<ezUInt16 Size>
EZ_FORCE_INLINE ezUInt32 ezHybridStringBase<Size>::GetElementCount() const
{
  return m_Data.GetCount() - 1;
}

template<ezUInt16 Size>
EZ_FORCE_INLINE ezUInt32 ezHybridStringBase<Size>::GetCharacterCount() const
{
  return m_uiCharacterCount;
}

template<ezUInt16 Size>
void ezHybridStringBase<Size>::operator=(const char* szString)
{
  ezUInt32 uiElementCount = 0;
  ezStringUtils::GetCharacterAndElementCount(szString, m_uiCharacterCount, uiElementCount);
  m_Data.SetCount(uiElementCount + 1);
  ezStringUtils::Copy(&m_Data[0], uiElementCount + 1, szString);
}

template<ezUInt16 Size>
void ezHybridStringBase<Size>::operator=(const ezHybridStringBase& rhs)
{
  if (this == &rhs)
    return;

  m_uiCharacterCount = rhs.m_uiCharacterCount;
  m_Data = rhs.m_Data;
}

template<ezUInt16 Size>
void ezHybridStringBase<Size>::operator=(const wchar_t* szString)
{
  ezStringUtf8 sConversion(szString, m_Data.GetAllocator());
  *this = sConversion.GetData();
}

template<ezUInt16 Size>
void ezHybridStringBase<Size>::operator=(const ezStringIterator& rhs)
{
  m_Data.SetCount(rhs.GetElementCount() + 1);
  ezStringUtils::Copy(&m_Data[0], m_Data.GetCount() + 1, rhs.GetData(), rhs.GetEnd());
  m_uiCharacterCount = ezStringUtils::GetCharacterCount(GetData());
}

template<ezUInt16 Size>
ezStringIterator ezHybridStringBase<Size>::GetIteratorFront() const
{
  return ezStringIterator(GetData(), GetData() + m_Data.GetCount() - 1, GetData(), m_uiCharacterCount + 1 == m_Data.GetCount());
}

template<ezUInt16 Size>
ezStringIterator ezHybridStringBase<Size>::GetIteratorBack() const
{
  ezStringIterator it (GetData(), GetData() + m_Data.GetCount() - 1, GetData() + m_Data.GetCount() - 1, m_uiCharacterCount + 1 == m_Data.GetCount());
  it.ResetToBack();
  return it;
}

template<ezUInt16 Size>
ezStringIterator ezHybridStringBase<Size>::GetSubString(ezUInt32 uiFirstCharacter, ezUInt32 uiNumCharacters) const
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
ezStringIterator ezHybridStringBase<Size>::GetFirst(ezUInt32 uiNumCharacters) const
{
  return GetSubString(0, uiNumCharacters);
}

template<ezUInt16 Size>
ezStringIterator ezHybridStringBase<Size>::GetLast(ezUInt32 uiNumCharacters) const
{
  EZ_ASSERT(uiNumCharacters < m_uiCharacterCount, "The string only contains %i characters, cannot return the last %i characters.", m_uiCharacterCount, uiNumCharacters);
  return GetSubString(m_uiCharacterCount - uiNumCharacters, uiNumCharacters);
}


template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString() : 
  ezHybridStringBase(A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString(ezIAllocator* pAllocator) : 
  ezHybridStringBase(pAllocator)
{
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString(const ezHybridString<Size, A>& other) :
  ezHybridStringBase(other, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString(const ezHybridStringBase<Size>& other) :
  ezHybridStringBase(other, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString(const char* rhs) :
  ezHybridStringBase(rhs, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString(const wchar_t* rhs) :
  ezHybridStringBase(rhs, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString(const ezStringIterator& rhs) :
  ezHybridStringBase(rhs, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE void ezHybridString<Size, A>::operator=(const ezHybridString<Size, A>& rhs)
{
  ezHybridStringBase::operator=(rhs);
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE void ezHybridString<Size, A>::operator=(const ezHybridStringBase<Size>& rhs)
{
  ezHybridStringBase::operator=(rhs);
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE void ezHybridString<Size, A>::operator=(const char* rhs)
{
  ezHybridStringBase::operator=(rhs);
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE void ezHybridString<Size, A>::operator=(const wchar_t* rhs)
{
  ezHybridStringBase::operator=(rhs);
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE void ezHybridString<Size, A>::operator=(const ezStringIterator& rhs)
{
  ezHybridStringBase::operator=(rhs);
}
