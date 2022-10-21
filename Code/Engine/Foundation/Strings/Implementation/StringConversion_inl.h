#pragma once

#include <Foundation/ThirdParty/utf8/utf8.h>

#include <Foundation/Strings/UnicodeUtils.h>

// **************** ezStringWChar ****************

inline ezStringWChar::ezStringWChar(ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline ezStringWChar::ezStringWChar(const ezUInt16* szUtf16, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf16;
}

inline ezStringWChar::ezStringWChar(const ezUInt32* szUtf32, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf32;
}

inline ezStringWChar::ezStringWChar(const wchar_t* szWChar, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szWChar;
}

inline ezStringWChar::ezStringWChar(ezStringView sUtf8, ezAllocatorBase* pAllocator /*= ezFoundation::GetDefaultAllocator()*/)
{
  *this = sUtf8;
}


// **************** ezStringUtf8 ****************

inline ezStringUtf8::ezStringUtf8(ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline ezStringUtf8::ezStringUtf8(const char* szUtf8, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf8;
}

inline ezStringUtf8::ezStringUtf8(const ezUInt16* szUtf16, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf16;
}

inline ezStringUtf8::ezStringUtf8(const ezUInt32* szUtf32, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf32;
}

inline ezStringUtf8::ezStringUtf8(const wchar_t* szWChar, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szWChar;
}

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

inline ezStringUtf8::ezStringUtf8(
  const Microsoft::WRL::Wrappers::HString& hstring, ezAllocatorBase* pAllocator /*= ezFoundation::GetDefaultAllocator()*/)
  : m_Data(pAllocator)
{
  *this = hstring;
}

inline ezStringUtf8::ezStringUtf8(const HSTRING& hstring, ezAllocatorBase* pAllocator /*= ezFoundation::GetDefaultAllocator()*/)
{
  *this = hstring;
}

#endif

// **************** ezStringUtf16 ****************

inline ezStringUtf16::ezStringUtf16(ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline ezStringUtf16::ezStringUtf16(const char* szUtf8, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf8;
}

inline ezStringUtf16::ezStringUtf16(const ezUInt16* szUtf16, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf16;
}

inline ezStringUtf16::ezStringUtf16(const ezUInt32* szUtf32, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf32;
}

inline ezStringUtf16::ezStringUtf16(const wchar_t* szWChar, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szWChar;
}



// **************** ezStringUtf32 ****************

inline ezStringUtf32::ezStringUtf32(ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline ezStringUtf32::ezStringUtf32(const char* szUtf8, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf8;
}

inline ezStringUtf32::ezStringUtf32(const ezUInt16* szUtf16, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf16;
}

inline ezStringUtf32::ezStringUtf32(const ezUInt32* szUtf32, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf32;
}

inline ezStringUtf32::ezStringUtf32(const wchar_t* szWChar, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szWChar;
}
