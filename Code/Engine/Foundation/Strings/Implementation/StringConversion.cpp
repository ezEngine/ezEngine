#include <Foundation/PCH.h>
#include <Foundation/Strings/StringConversion.h>

// **************** ezStringWChar ****************

void ezStringWChar::operator=(const char* szUtf8)
{
  EZ_ASSERT_DEV(ezUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

  m_Data.Clear();

  if (szUtf8 != nullptr)
  {

    // skip any Utf8 Byte Order Mark
    ezUnicodeUtils::SkipUtf8Bom(szUtf8);

    while (*szUtf8 != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = ezUnicodeUtils::DecodeUtf8ToUtf32(szUtf8);

      // encode utf32 to wchar_t
      ezUnicodeUtils::UtfInserter<wchar_t, ezHybridArray<wchar_t, BufferSize> > tempInserter(&m_Data);
      ezUnicodeUtils::EncodeUtf32ToWChar(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void ezStringWChar::operator=(const ezUInt16* szUtf16)
{
  m_Data.Clear();

  if (szUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    ezUnicodeUtils::SkipUtf16BomLE(szUtf16);
    EZ_ASSERT_DEV(!ezUnicodeUtils::SkipUtf16BomBE(szUtf16), "Utf-16 Big Endian is currently not supported.");

    while (*szUtf16 != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = ezUnicodeUtils::DecodeUtf16ToUtf32(szUtf16);

      // encode utf32 to wchar_t
      ezUnicodeUtils::UtfInserter<wchar_t, ezHybridArray<wchar_t, BufferSize> > tempInserter(&m_Data);
      ezUnicodeUtils::EncodeUtf32ToWChar(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void ezStringWChar::operator=(const ezUInt32* szUtf32)
{
  m_Data.Clear();

  if (szUtf32 != nullptr)
  {

    while (*szUtf32 != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = *szUtf32;
      ++szUtf32;

      // encode utf32 to wchar_t
      ezUnicodeUtils::UtfInserter<wchar_t, ezHybridArray<wchar_t, BufferSize> > tempInserter(&m_Data);
      ezUnicodeUtils::EncodeUtf32ToWChar(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void ezStringWChar::operator=(const wchar_t* szWChar)
{
  m_Data.Clear();

  if (szWChar != nullptr)
  {

    while (*szWChar != '\0')
    {
      m_Data.PushBack(*szWChar);
      ++szWChar;
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}



// **************** ezStringUtf8 ****************

void ezStringUtf8::operator=(const char* szUtf8)
{
  EZ_ASSERT_DEV(ezUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

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


void ezStringUtf8::operator=(const ezUInt16* szUtf16)
{
  m_Data.Clear();

  if (szUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    ezUnicodeUtils::SkipUtf16BomLE(szUtf16);
    EZ_ASSERT_DEV(!ezUnicodeUtils::SkipUtf16BomBE(szUtf16), "Utf-16 Big Endian is currently not supported.");

    while (*szUtf16 != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = ezUnicodeUtils::DecodeUtf16ToUtf32(szUtf16);

      // encode utf32 to wchar_t
      ezUnicodeUtils::UtfInserter<char, ezHybridArray<char, BufferSize> > tempInserter(&m_Data);
      ezUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void ezStringUtf8::operator=(const ezUInt32* szUtf32)
{
  m_Data.Clear();

  if (szUtf32 != nullptr)
  {
    while (*szUtf32 != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = *szUtf32;
      ++szUtf32;

      // encode utf32 to wchar_t
      ezUnicodeUtils::UtfInserter<char, ezHybridArray<char, BufferSize> > tempInserter(&m_Data);
      ezUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void ezStringUtf8::operator=(const wchar_t* szWChar)
{
  m_Data.Clear();

  if (szWChar != nullptr)
  {
    while (*szWChar != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = ezUnicodeUtils::DecodeWCharToUtf32(szWChar);

      // encode utf32 to wchar_t
      ezUnicodeUtils::UtfInserter<char, ezHybridArray<char, BufferSize> > tempInserter(&m_Data);
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
  EZ_ASSERT_DEV(ezUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

  m_Data.Clear();

  if (szUtf8 != nullptr)
  {
    // skip any Utf8 Byte Order Mark
    ezUnicodeUtils::SkipUtf8Bom(szUtf8);

    while (*szUtf8 != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = ezUnicodeUtils::DecodeUtf8ToUtf32(szUtf8);

      // encode utf32 to wchar_t
      ezUnicodeUtils::UtfInserter<ezUInt16, ezHybridArray<ezUInt16, BufferSize> > tempInserter(&m_Data);
      ezUnicodeUtils::EncodeUtf32ToUtf16(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void ezStringUtf16::operator=(const ezUInt16* szUtf16)
{
  m_Data.Clear();

  if (szUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    ezUnicodeUtils::SkipUtf16BomLE(szUtf16);
    EZ_ASSERT_DEV(!ezUnicodeUtils::SkipUtf16BomBE(szUtf16), "Utf-16 Big Endian is currently not supported.");

    while (*szUtf16 != '\0')
    {
      m_Data.PushBack(*szUtf16);
      ++szUtf16;
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void ezStringUtf16::operator=(const ezUInt32* szUtf32)
{
  m_Data.Clear();

  if (szUtf32 != nullptr)
  {
    while (*szUtf32 != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = *szUtf32;
      ++szUtf32;

      // encode utf32 to wchar_t
      ezUnicodeUtils::UtfInserter<ezUInt16, ezHybridArray<ezUInt16, BufferSize> > tempInserter(&m_Data);
      ezUnicodeUtils::EncodeUtf32ToUtf16(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void ezStringUtf16::operator=(const wchar_t* szWChar)
{
  m_Data.Clear();

  if (szWChar != nullptr)
  {
    while (*szWChar != '\0')
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = ezUnicodeUtils::DecodeWCharToUtf32(szWChar);

      // encode utf32 to wchar_t
      ezUnicodeUtils::UtfInserter<ezUInt16, ezHybridArray<ezUInt16, BufferSize> > tempInserter(&m_Data);
      ezUnicodeUtils::EncodeUtf32ToUtf16(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}






// **************** ezStringUtf32 ****************

void ezStringUtf32::operator=(const char* szUtf8)
{
  EZ_ASSERT_DEV(ezUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

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


void ezStringUtf32::operator=(const ezUInt16* szUtf16)
{
  m_Data.Clear();

  if (szUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    ezUnicodeUtils::SkipUtf16BomLE(szUtf16);
    EZ_ASSERT_DEV(!ezUnicodeUtils::SkipUtf16BomBE(szUtf16), "Utf-16 Big Endian is currently not supported.");

    while (*szUtf16 != '\0')
    {
      // decode utf16 to utf32
      m_Data.PushBack(ezUnicodeUtils::DecodeUtf16ToUtf32(szUtf16));
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void ezStringUtf32::operator=(const ezUInt32* szUtf32)
{
  m_Data.Clear();

  if (szUtf32 != nullptr)
  {
    while (*szUtf32 != '\0')
    {
      m_Data.PushBack(*szUtf32);
      ++szUtf32;
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void ezStringUtf32::operator=(const wchar_t* szWChar)
{
  m_Data.Clear();

  if (szWChar != nullptr)
  {
    while (*szWChar != '\0')
    {
      // decode wchar_t to utf32
      m_Data.PushBack(ezUnicodeUtils::DecodeWCharToUtf32(szWChar));
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

ezStringHString::ezStringHString()
{
}

ezStringHString::ezStringHString(const char* szUtf8)
{
  *this = szUtf8;
}

ezStringHString::ezStringHString(const ezUInt16* szUtf16)
{
  *this = szUtf16;
}

ezStringHString::ezStringHString(const ezUInt32* szUtf32)
{
  *this = szUtf32;
}

ezStringHString::ezStringHString(const wchar_t* szWChar)
{
  *this = szWChar;
}

void ezStringHString::operator=(const char* szUtf8)
{
  m_Data.Set(ezStringWChar(szUtf8).GetData());
}

void ezStringHString::operator=(const ezUInt16* szUtf16)
{
  m_Data.Set(ezStringWChar(szUtf16).GetData());
}

void ezStringHString::operator=(const ezUInt32* szUtf32)
{
  m_Data.Set(ezStringWChar(szUtf32).GetData());
}

void ezStringHString::operator=(const wchar_t* szWChar)
{
  m_Data.Set(ezStringWChar(szWChar).GetData());
}

#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_StringConversion);

