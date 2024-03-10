#pragma once

template <ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  Clear();
}

template <ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(const ezHybridStringBase& rhs, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(ezHybridStringBase&& rhs, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  operator=(std::move(rhs));
}

template <ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(const char* rhs, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(const wchar_t* rhs, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(const ezStringView& rhs, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <ezUInt16 Size>
ezHybridStringBase<Size>::~ezHybridStringBase() = default;

template <ezUInt16 Size>
void ezHybridStringBase<Size>::Clear()
{
  m_Data.SetCountUninitialized(1);
  m_Data[0] = '\0';
}

template <ezUInt16 Size>
EZ_ALWAYS_INLINE const char* ezHybridStringBase<Size>::GetData() const
{
  EZ_ASSERT_DEBUG(!m_Data.IsEmpty(), "ezHybridString has been corrupted, the array can never be empty. This can happen when you access a "
                                     "string that was previously std::move'd into another string.");

  return &m_Data[0];
}

template <ezUInt16 Size>
EZ_ALWAYS_INLINE ezUInt32 ezHybridStringBase<Size>::GetElementCount() const
{
  return m_Data.GetCount() - 1;
}

template <ezUInt16 Size>
EZ_ALWAYS_INLINE ezUInt32 ezHybridStringBase<Size>::GetCharacterCount() const
{
  return ezStringUtils::GetCharacterCount(GetData());
}

template <ezUInt16 Size>
void ezHybridStringBase<Size>::operator=(const char* szString)
{
  ezUInt32 uiElementCount = ezStringUtils::GetStringElementCount(szString);

  if (szString + uiElementCount < m_Data.GetData() || szString >= m_Data.GetData() + m_Data.GetCount())
  {
    // source string is outside our own memory, so no overlapped copy
  }
  else
  {
    // source string overlaps with our own memory -> we can't increase the size of our memory, as that might invalidate the source data
    EZ_ASSERT_DEBUG(uiElementCount < m_Data.GetCount(), "Invalid copy of overlapping string data.");
  }

  m_Data.SetCountUninitialized(uiElementCount + 1);
  ezStringUtils::Copy(&m_Data[0], uiElementCount + 1, szString);
}

template <ezUInt16 Size>
void ezHybridStringBase<Size>::operator=(const ezHybridStringBase& rhs)
{
  if (this == &rhs)
    return;

  m_Data = rhs.m_Data;
}

template <ezUInt16 Size>
void ezHybridStringBase<Size>::operator=(ezHybridStringBase&& rhs)
{
  if (this == &rhs)
    return;

  m_Data = std::move(rhs.m_Data);
}

template <ezUInt16 Size>
void ezHybridStringBase<Size>::operator=(const wchar_t* szString)
{
  ezStringUtf8 sConversion(szString, m_Data.GetAllocator());
  *this = sConversion.GetData();
}

template <ezUInt16 Size>
void ezHybridStringBase<Size>::operator=(const ezStringView& rhs)
{
  EZ_ASSERT_DEBUG(rhs.GetStartPointer() < m_Data.GetData() || rhs.GetStartPointer() >= m_Data.GetData() + m_Data.GetCount(),
    "Can't assign string a value that points to ourself!");

  m_Data.SetCountUninitialized(rhs.GetElementCount() + 1);
  ezStringUtils::Copy(&m_Data[0], m_Data.GetCount(), rhs.GetStartPointer(), rhs.GetEndPointer());
}

template <ezUInt16 Size>
ezStringView ezHybridStringBase<Size>::GetSubString(ezUInt32 uiFirstCharacter, ezUInt32 uiNumCharacters) const
{
  const char* szStart = GetData();
  if (ezUnicodeUtils::MoveToNextUtf8(szStart, uiFirstCharacter).Failed())
    return {};                                                           // szStart was moved too far, the result is just an empty string

  const char* szEnd = szStart;
  ezUnicodeUtils::MoveToNextUtf8(szEnd, uiNumCharacters).IgnoreResult(); // if it fails, szEnd just points to the end of this string

  return ezStringView(szStart, szEnd);
}

template <ezUInt16 Size>
ezStringView ezHybridStringBase<Size>::GetFirst(ezUInt32 uiNumCharacters) const
{
  return GetSubString(0, uiNumCharacters);
}

template <ezUInt16 Size>
ezStringView ezHybridStringBase<Size>::GetLast(ezUInt32 uiNumCharacters) const
{
  const ezUInt32 uiMaxCharacterCount = GetCharacterCount();
  EZ_ASSERT_DEV(uiNumCharacters < uiMaxCharacterCount, "The string only contains {0} characters, cannot return the last {1} characters.",
    uiMaxCharacterCount, uiNumCharacters);
  return GetSubString(uiMaxCharacterCount - uiNumCharacters, uiNumCharacters);
}


template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE ezHybridString<Size, A>::ezHybridString()
  : ezHybridStringBase<Size>(A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE ezHybridString<Size, A>::ezHybridString(ezAllocator* pAllocator)
  : ezHybridStringBase<Size>(pAllocator)
{
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE ezHybridString<Size, A>::ezHybridString(const ezHybridString<Size, A>& other)
  : ezHybridStringBase<Size>(other, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE ezHybridString<Size, A>::ezHybridString(const ezHybridStringBase<Size>& other)
  : ezHybridStringBase<Size>(other, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE ezHybridString<Size, A>::ezHybridString(ezHybridString<Size, A>&& other)
  : ezHybridStringBase<Size>(std::move(other), A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE ezHybridString<Size, A>::ezHybridString(ezHybridStringBase<Size>&& other)
  : ezHybridStringBase<Size>(std::move(other), A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE ezHybridString<Size, A>::ezHybridString(const char* rhs)
  : ezHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE ezHybridString<Size, A>::ezHybridString(const wchar_t* rhs)
  : ezHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE ezHybridString<Size, A>::ezHybridString(const ezStringView& rhs)
  : ezHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE void ezHybridString<Size, A>::operator=(const ezHybridString<Size, A>& rhs)
{
  ezHybridStringBase<Size>::operator=(rhs);
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE void ezHybridString<Size, A>::operator=(const ezHybridStringBase<Size>& rhs)
{
  ezHybridStringBase<Size>::operator=(rhs);
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE void ezHybridString<Size, A>::operator=(ezHybridString<Size, A>&& rhs)
{
  ezHybridStringBase<Size>::operator=(std::move(rhs));
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE void ezHybridString<Size, A>::operator=(ezHybridStringBase<Size>&& rhs)
{
  ezHybridStringBase<Size>::operator=(std::move(rhs));
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE void ezHybridString<Size, A>::operator=(const char* rhs)
{
  ezHybridStringBase<Size>::operator=(rhs);
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE void ezHybridString<Size, A>::operator=(const wchar_t* rhs)
{
  ezHybridStringBase<Size>::operator=(rhs);
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE void ezHybridString<Size, A>::operator=(const ezStringView& rhs)
{
  ezHybridStringBase<Size>::operator=(rhs);
}

#if EZ_ENABLED(EZ_INTEROP_STL_STRINGS)

template <ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(const std::string_view& rhs, ezAllocator* pAllocator)
{
  *this = rhs;
}

template <ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(const std::string& rhs, ezAllocator* pAllocator)
{
  *this = rhs;
}

template <ezUInt16 Size>
void ezHybridStringBase<Size>::operator=(const std::string_view& rhs)
{
  if (rhs.empty())
  {
    Clear();
  }
  else
  {
    m_Data.SetCountUninitialized(((ezUInt32)rhs.size() + 1));
    ezStringUtils::Copy(&m_Data[0], m_Data.GetCount(), rhs.data(), rhs.data() + rhs.size());
  }
}

template <ezUInt16 Size>
void ezHybridStringBase<Size>::operator=(const std::string& rhs)
{
  *this = std::string_view(rhs);
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE ezHybridString<Size, A>::ezHybridString(const std::string_view& rhs)
  : ezHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE ezHybridString<Size, A>::ezHybridString(const std::string& rhs)
  : ezHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE void ezHybridString<Size, A>::operator=(const std::string_view& rhs)
{
  ezHybridStringBase<Size>::operator=(rhs);
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE void ezHybridString<Size, A>::operator=(const std::string& rhs)
{
  ezHybridStringBase<Size>::operator=(rhs);
}

#endif

#include <Foundation/Strings/Implementation/AllStrings_inl.h>
