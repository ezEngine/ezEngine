#pragma once

template<ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(ezAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  Clear();
}

template<ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(const ezHybridStringBase& rhs, ezAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = rhs;
}

template<ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(ezHybridStringBase&& rhs, ezAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  operator=(std::move(rhs));
}

template<ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(const char* rhs, ezAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = rhs;
}

template<ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(const wchar_t* rhs, ezAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = rhs;
}

template<ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(const ezStringView& rhs, ezAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = rhs;
}

template<ezUInt16 Size>
ezHybridStringBase<Size>::~ezHybridStringBase()
{
}

template<ezUInt16 Size>
ezHybridStringBase<Size>::operator ezStringView() const
{ 
  return ezStringView(GetData(), GetData() + m_Data.GetCount() - 1);
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
  EZ_ASSERT_DEBUG(!m_Data.IsEmpty(), "ezHybridString has been corrupted, the array can never be empty.");

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
void ezHybridStringBase<Size>::operator=(ezHybridStringBase&& rhs)
{
  if (this == &rhs)
    return;

  m_uiCharacterCount = rhs.m_uiCharacterCount;
  m_Data = std::move(rhs.m_Data);
}

template<ezUInt16 Size>
void ezHybridStringBase<Size>::operator=(const wchar_t* szString)
{
  ezStringUtf8 sConversion(szString, m_Data.GetAllocator());
  *this = sConversion.GetData();
}

template<ezUInt16 Size>
void ezHybridStringBase<Size>::operator=(const ezStringView& rhs)
{
  m_Data.SetCount(rhs.GetElementCount() + 1);
  ezStringUtils::Copy(&m_Data[0], m_Data.GetCount() + 1, rhs.GetData(), rhs.GetEndPosition());
  m_uiCharacterCount = ezStringUtils::GetCharacterCount(GetData());
}

template<ezUInt16 Size>
ezStringView ezHybridStringBase<Size>::GetSubString(ezUInt32 uiFirstCharacter, ezUInt32 uiNumCharacters) const
{
  EZ_ASSERT_DEV(uiFirstCharacter < m_uiCharacterCount, "The string only has %i characters, cannot start a sub-string at character %i.", m_uiCharacterCount, uiFirstCharacter);
  EZ_ASSERT_DEV(uiFirstCharacter + uiNumCharacters <= m_uiCharacterCount, "The string only has %i characters, cannot get a sub-string up to character %i.", m_uiCharacterCount, uiFirstCharacter + uiNumCharacters);

  const char* szStart = GetData();
  ezUnicodeUtils::MoveToNextUtf8(szStart, uiFirstCharacter);

  const char* szEnd = szStart;
  ezUnicodeUtils::MoveToNextUtf8(szEnd, uiNumCharacters);

  return ezStringView(szStart, szEnd);
}

template<ezUInt16 Size>
ezStringView ezHybridStringBase<Size>::GetFirst(ezUInt32 uiNumCharacters) const
{
  return GetSubString(0, uiNumCharacters);
}

template<ezUInt16 Size>
ezStringView ezHybridStringBase<Size>::GetLast(ezUInt32 uiNumCharacters) const
{
  EZ_ASSERT_DEV(uiNumCharacters < m_uiCharacterCount, "The string only contains %i characters, cannot return the last %i characters.", m_uiCharacterCount, uiNumCharacters);
  return GetSubString(m_uiCharacterCount - uiNumCharacters, uiNumCharacters);
}


template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString() : 
  ezHybridStringBase<Size>(A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString(ezAllocatorBase* pAllocator) : 
  ezHybridStringBase<Size>(pAllocator)
{
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString(const ezHybridString<Size, A>& other) :
  ezHybridStringBase<Size>(other, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString(const ezHybridStringBase<Size>& other) :
  ezHybridStringBase<Size>(other, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString(ezHybridString<Size, A>&& other) :
  ezHybridStringBase<Size>(std::move(other), A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString(ezHybridStringBase<Size>&& other) :
  ezHybridStringBase<Size>(std::move(other), A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString(const char* rhs) :
  ezHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString(const wchar_t* rhs) :
  ezHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString(const ezStringView& rhs) :
  ezHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE void ezHybridString<Size, A>::operator=(const ezHybridString<Size, A>& rhs)
{
  ezHybridStringBase<Size>::operator=(rhs);
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE void ezHybridString<Size, A>::operator=(const ezHybridStringBase<Size>& rhs)
{
  ezHybridStringBase<Size>::operator=(rhs);
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE void ezHybridString<Size, A>::operator=(ezHybridString<Size, A>&& rhs)
{
  ezHybridStringBase<Size>::operator=(std::move(rhs));
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE void ezHybridString<Size, A>::operator=(ezHybridStringBase<Size>&& rhs)
{
  ezHybridStringBase<Size>::operator=(std::move(rhs));
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE void ezHybridString<Size, A>::operator=(const char* rhs)
{
  ezHybridStringBase<Size>::operator=(rhs);
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE void ezHybridString<Size, A>::operator=(const wchar_t* rhs)
{
  ezHybridStringBase<Size>::operator=(rhs);
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE void ezHybridString<Size, A>::operator=(const ezStringView& rhs)
{
  ezHybridStringBase<Size>::operator=(rhs);
}

#include <Foundation/Strings/Implementation/AllStrings_inl.h>

