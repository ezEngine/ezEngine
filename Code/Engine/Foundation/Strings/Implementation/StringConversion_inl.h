#pragma once

#include <Foundation/ThirdParty/utf8/utf8.h>

#include <Foundation/Strings/UnicodeUtils.h>

// **************** ezStringWChar ****************

inline ezStringWChar::ezStringWChar(ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline ezStringWChar::ezStringWChar(const ezUInt16* pUtf16, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf16;
}

inline ezStringWChar::ezStringWChar(const ezUInt32* pUtf32, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf32;
}

inline ezStringWChar::ezStringWChar(const wchar_t* pWChar, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pWChar;
}

inline ezStringWChar::ezStringWChar(ezStringView sUtf8, ezAllocator* pAllocator /*= ezFoundation::GetDefaultAllocator()*/)
  : m_Data(pAllocator)
{
  *this = sUtf8;
}


// **************** ezStringUtf8 ****************

inline ezStringUtf8::ezStringUtf8(ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline ezStringUtf8::ezStringUtf8(const char* szUtf8, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf8;
}

inline ezStringUtf8::ezStringUtf8(const ezUInt16* pUtf16, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf16;
}

inline ezStringUtf8::ezStringUtf8(const ezUInt32* pUtf32, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf32;
}

inline ezStringUtf8::ezStringUtf8(const wchar_t* pWChar, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pWChar;
}

// **************** ezStringUtf16 ****************

inline ezStringUtf16::ezStringUtf16(ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline ezStringUtf16::ezStringUtf16(const char* szUtf8, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf8;
}

inline ezStringUtf16::ezStringUtf16(const ezUInt16* pUtf16, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf16;
}

inline ezStringUtf16::ezStringUtf16(const ezUInt32* pUtf32, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf32;
}

inline ezStringUtf16::ezStringUtf16(const wchar_t* pWChar, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pWChar;
}



// **************** ezStringUtf32 ****************

inline ezStringUtf32::ezStringUtf32(ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline ezStringUtf32::ezStringUtf32(const char* szUtf8, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf8;
}

inline ezStringUtf32::ezStringUtf32(const ezUInt16* pUtf16, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf16;
}

inline ezStringUtf32::ezStringUtf32(const ezUInt32* pUtf32, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf32;
}

inline ezStringUtf32::ezStringUtf32(const wchar_t* pWChar, ezAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pWChar;
}
