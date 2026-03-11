#include <Foundation/FoundationPCH.h>

#include <Foundation/Strings/StringConversion.h>

// **************** ezStringWChar ****************

void ezStringWChar::operator=(const ezUInt16* pUtf16)
{
  m_Data.Clear();

  if (pUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    ezUnicodeUtils::SkipUtf16BomLE(pUtf16);
    EZ_ASSERT_DEV(!ezUnicodeUtils::SkipUtf16BomBE(pUtf16), "Utf-16 Big Endian is currently not supported.");

    ezUnicodeUtils::UtfInserter<wchar_t, ezHybridArray<wchar_t, BufferSize>> tempInserter(&m_Data);

    while (*pUtf16 != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = ezUnicodeUtils::DecodeUtf16ToUtf32(pUtf16);

      // encode utf32 to wchar_t
      ezUnicodeUtils::EncodeUtf32ToWChar(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void ezStringWChar::operator=(const ezUInt32* pUtf32)
{
  m_Data.Clear();

  if (pUtf32 != nullptr)
  {
    ezUnicodeUtils::UtfInserter<wchar_t, ezHybridArray<wchar_t, BufferSize>> tempInserter(&m_Data);

    while (*pUtf32 != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = *pUtf32;
      ++pUtf32;

      // encode utf32 to wchar_t
      ezUnicodeUtils::EncodeUtf32ToWChar(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void ezStringWChar::operator=(const wchar_t* pWChar)
{
  m_Data.Clear();

  if (pWChar != nullptr)
  {

    while (*pWChar != '\0')
    {
      m_Data.PushBack(*pWChar);
      ++pWChar;
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void ezStringWChar::operator=(ezStringView sUtf8)
{
  m_Data.Clear();

  if (!sUtf8.IsEmpty())
  {
    const char* szUtf8 = sUtf8.GetStartPointer();

    EZ_ASSERT_DEV(ezUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

    // skip any Utf8 Byte Order Mark
    ezUnicodeUtils::SkipUtf8Bom(szUtf8);

    ezUnicodeUtils::UtfInserter<wchar_t, ezHybridArray<wchar_t, BufferSize>> tempInserter(&m_Data);

    while (szUtf8 < sUtf8.GetEndPointer() && *szUtf8 != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = ezUnicodeUtils::DecodeUtf8ToUtf32(szUtf8);

      // encode utf32 to wchar_t
      ezUnicodeUtils::EncodeUtf32ToWChar(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

// **************** ezStringUtf8 ****************

void ezStringUtf8::operator=(const char* szUtf8)
{
  EZ_ASSERT_DEV(
    ezUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

  m_Data.Clear();

  if (szUtf8 != nullptr)
  {
    // skip any Utf8 Byte Order Mark
    ezUnicodeUtils::SkipUtf8Bom(szUtf8);

    while (*szUtf8 != '\0')
    {
      m_Data.PushBack(*szUtf8);
      ++szUtf8;
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void ezStringUtf8::operator=(const ezUInt16* pUtf16)
{
  m_Data.Clear();

  if (pUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    ezUnicodeUtils::SkipUtf16BomLE(pUtf16);
    EZ_ASSERT_DEV(!ezUnicodeUtils::SkipUtf16BomBE(pUtf16), "Utf-16 Big Endian is currently not supported.");

    ezUnicodeUtils::UtfInserter<char, ezHybridArray<char, BufferSize>> tempInserter(&m_Data);

    while (*pUtf16 != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = ezUnicodeUtils::DecodeUtf16ToUtf32(pUtf16);

      // encode utf32 to wchar_t
      ezUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void ezStringUtf8::operator=(const ezUInt32* pUtf32)
{
  m_Data.Clear();

  if (pUtf32 != nullptr)
  {
    ezUnicodeUtils::UtfInserter<char, ezHybridArray<char, BufferSize>> tempInserter(&m_Data);

    while (*pUtf32 != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = *pUtf32;
      ++pUtf32;

      // encode utf32 to wchar_t
      ezUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void ezStringUtf8::operator=(const wchar_t* pWChar)
{
  m_Data.Clear();

  if (pWChar != nullptr)
  {
    ezUnicodeUtils::UtfInserter<char, ezHybridArray<char, BufferSize>> tempInserter(&m_Data);

    while (*pWChar != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = ezUnicodeUtils::DecodeWCharToUtf32(pWChar);

      // encode utf32 to wchar_t
      ezUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

void ezStringUtf8::operator=(const Microsoft::WRL::Wrappers::HString& hstring)
{
  ezUInt32 len = 0;
  const wchar_t* raw = hstring.GetRawBuffer(&len);

  // delegate to wchar_t operator
  *this = raw;
}

void ezStringUtf8::operator=(const HSTRING& hstring)
{
  Microsoft::WRL::Wrappers::HString tmp;
  tmp.Attach(hstring);

  ezUInt32 len = 0;
  const wchar_t* raw = tmp.GetRawBuffer(&len);

  // delegate to wchar_t operator
  *this = raw;
}

#endif


// **************** ezStringUtf16 ****************

void ezStringUtf16::operator=(const char* szUtf8)
{
  EZ_ASSERT_DEV(
    ezUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

  m_Data.Clear();

  if (szUtf8 != nullptr)
  {
    // skip any Utf8 Byte Order Mark
    ezUnicodeUtils::SkipUtf8Bom(szUtf8);

    ezUnicodeUtils::UtfInserter<ezUInt16, ezHybridArray<ezUInt16, BufferSize>> tempInserter(&m_Data);

    while (*szUtf8 != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = ezUnicodeUtils::DecodeUtf8ToUtf32(szUtf8);

      // encode utf32 to wchar_t
      ezUnicodeUtils::EncodeUtf32ToUtf16(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void ezStringUtf16::operator=(const ezUInt16* pUtf16)
{
  m_Data.Clear();

  if (pUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    ezUnicodeUtils::SkipUtf16BomLE(pUtf16);
    EZ_ASSERT_DEV(!ezUnicodeUtils::SkipUtf16BomBE(pUtf16), "Utf-16 Big Endian is currently not supported.");

    while (*pUtf16 != '\0')
    {
      m_Data.PushBack(*pUtf16);
      ++pUtf16;
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void ezStringUtf16::operator=(const ezUInt32* pUtf32)
{
  m_Data.Clear();

  if (pUtf32 != nullptr)
  {
    ezUnicodeUtils::UtfInserter<ezUInt16, ezHybridArray<ezUInt16, BufferSize>> tempInserter(&m_Data);

    while (*pUtf32 != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = *pUtf32;
      ++pUtf32;

      // encode utf32 to wchar_t
      ezUnicodeUtils::EncodeUtf32ToUtf16(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void ezStringUtf16::operator=(const wchar_t* pWChar)
{
  m_Data.Clear();

  if (pWChar != nullptr)
  {
    ezUnicodeUtils::UtfInserter<ezUInt16, ezHybridArray<ezUInt16, BufferSize>> tempInserter(&m_Data);

    while (*pWChar != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = ezUnicodeUtils::DecodeWCharToUtf32(pWChar);

      // encode utf32 to wchar_t
      ezUnicodeUtils::EncodeUtf32ToUtf16(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}



// **************** ezStringUtf32 ****************

void ezStringUtf32::operator=(const char* szUtf8)
{
  EZ_ASSERT_DEV(
    ezUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

  m_Data.Clear();

  if (szUtf8 != nullptr)
  {
    // skip any Utf8 Byte Order Mark
    ezUnicodeUtils::SkipUtf8Bom(szUtf8);

    while (*szUtf8 != '\0')
    {
      // decode utf8 to utf32
      m_Data.PushBack(ezUnicodeUtils::DecodeUtf8ToUtf32(szUtf8));
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void ezStringUtf32::operator=(const ezUInt16* pUtf16)
{
  m_Data.Clear();

  if (pUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    ezUnicodeUtils::SkipUtf16BomLE(pUtf16);
    EZ_ASSERT_DEV(!ezUnicodeUtils::SkipUtf16BomBE(pUtf16), "Utf-16 Big Endian is currently not supported.");

    while (*pUtf16 != '\0')
    {
      // decode utf16 to utf32
      m_Data.PushBack(ezUnicodeUtils::DecodeUtf16ToUtf32(pUtf16));
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void ezStringUtf32::operator=(const ezUInt32* pUtf32)
{
  m_Data.Clear();

  if (pUtf32 != nullptr)
  {
    while (*pUtf32 != '\0')
    {
      m_Data.PushBack(*pUtf32);
      ++pUtf32;
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void ezStringUtf32::operator=(const wchar_t* pWChar)
{
  m_Data.Clear();

  if (pWChar != nullptr)
  {
    while (*pWChar != '\0')
    {
      // decode wchar_t to utf32
      m_Data.PushBack(ezUnicodeUtils::DecodeWCharToUtf32(pWChar));
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}
