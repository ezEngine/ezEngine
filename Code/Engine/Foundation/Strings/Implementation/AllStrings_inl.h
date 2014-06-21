#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>

template<ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(const ezStringBuilder& rhs, ezAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = rhs;
}

template<ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(ezStringBuilder&& rhs, ezAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = std::move(rhs);
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString(const ezStringBuilder& rhs) :
  ezHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE ezHybridString<Size, A>::ezHybridString(ezStringBuilder&& rhs) :
  ezHybridStringBase<Size>(std::move(rhs), A::GetAllocator())
{
}

template<ezUInt16 Size>
void ezHybridStringBase<Size>::operator=(const ezStringBuilder& rhs)
{
  m_uiCharacterCount= rhs.m_uiCharacterCount;
  m_Data = rhs.m_Data;
}

template<ezUInt16 Size>
void ezHybridStringBase<Size>::operator=(ezStringBuilder&& rhs)
{
  m_uiCharacterCount = rhs.m_uiCharacterCount;
  m_Data = std::move(rhs.m_Data);
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE void ezHybridString<Size, A>::operator=(const ezStringBuilder& rhs)
{
  ezHybridStringBase<Size>::operator=(rhs);
}

template <ezUInt16 Size, typename A>
EZ_FORCE_INLINE void ezHybridString<Size, A>::operator=(ezStringBuilder&& rhs)
{
  ezHybridStringBase<Size>::operator=(std::move(rhs));
}

template <ezUInt16 Size>
ezStringBuilder::ezStringBuilder(const ezHybridStringBase<Size>& rhs) : m_uiCharacterCount(rhs.m_uiCharacterCount), m_Data(rhs.m_Data)
{
}

template <ezUInt16 Size, typename A>
ezStringBuilder::ezStringBuilder(const ezHybridString<Size, A>& rhs) : m_uiCharacterCount(rhs.m_uiCharacterCount), m_Data(rhs.m_Data)
{
}

template <ezUInt16 Size>
void ezStringBuilder::operator=(const ezHybridStringBase<Size>& rhs)
{
  m_uiCharacterCount = rhs.m_uiCharacterCount;
  m_Data = rhs.m_Data;
}

template <ezUInt16 Size, typename A>
void ezStringBuilder::operator=(const ezHybridString<Size, A>& rhs)
{
  m_uiCharacterCount = rhs.m_uiCharacterCount;
  m_Data = rhs.m_Data;
}

template <ezUInt16 Size>
ezStringBuilder::ezStringBuilder(ezHybridStringBase<Size>&& rhs) : m_uiCharacterCount(rhs.m_uiCharacterCount), m_Data(std::move(rhs.m_Data))
{
}

template <ezUInt16 Size, typename A>
ezStringBuilder::ezStringBuilder(ezHybridString<Size, A>&& rhs) : m_uiCharacterCount(rhs.m_uiCharacterCount), m_Data(std::move(rhs.m_Data))
{
}

template <ezUInt16 Size>
void ezStringBuilder::operator=(ezHybridStringBase<Size>&& rhs)
{
  m_uiCharacterCount = rhs.m_uiCharacterCount;
  m_Data = std::move(rhs.m_Data);
}

template <ezUInt16 Size, typename A>
void ezStringBuilder::operator=(ezHybridString<Size, A>&& rhs)
{
  m_uiCharacterCount = rhs.m_uiCharacterCount;
  m_Data = std::move(rhs.m_Data);
}

