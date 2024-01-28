#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>

template <ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(const ezStringBuilder& rhs, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <ezUInt16 Size>
ezHybridStringBase<Size>::ezHybridStringBase(ezStringBuilder&& rhs, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = std::move(rhs);
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE ezHybridString<Size, A>::ezHybridString(const ezStringBuilder& rhs)
  : ezHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE ezHybridString<Size, A>::ezHybridString(ezStringBuilder&& rhs)
  : ezHybridStringBase<Size>(std::move(rhs), A::GetAllocator())
{
}

template <ezUInt16 Size>
void ezHybridStringBase<Size>::operator=(const ezStringBuilder& rhs)
{
  m_Data = rhs.m_Data;
}

template <ezUInt16 Size>
void ezHybridStringBase<Size>::operator=(ezStringBuilder&& rhs)
{
  m_Data = std::move(rhs.m_Data);
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE void ezHybridString<Size, A>::operator=(const ezStringBuilder& rhs)
{
  ezHybridStringBase<Size>::operator=(rhs);
}

template <ezUInt16 Size, typename A>
EZ_ALWAYS_INLINE void ezHybridString<Size, A>::operator=(ezStringBuilder&& rhs)
{
  ezHybridStringBase<Size>::operator=(std::move(rhs));
}

template <ezUInt16 Size>
void ezHybridStringBase<Size>::ReadAll(ezStreamReader& inout_stream)
{
  Clear();

  ezHybridArray<ezUInt8, 1024 * 4> Bytes(m_Data.GetAllocator());
  ezUInt8 Temp[1024];

  while (true)
  {
    const ezUInt32 uiRead = (ezUInt32)inout_stream.ReadBytes(Temp, 1024);

    if (uiRead == 0)
      break;

    Bytes.PushBackRange(ezArrayPtr<ezUInt8>(Temp, uiRead));
  }

  Bytes.PushBack('\0');

  *this = (const char*)&Bytes[0];
}
