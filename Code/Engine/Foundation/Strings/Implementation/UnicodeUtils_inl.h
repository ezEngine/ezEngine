#pragma once

/*
You can classify bytes in a UTF-8 stream as follows:
  With the high bit set to 0, it's a single byte value.
  With the two high bits set to 10, it's a continuation byte (the second, third or fourth byte in a UTF-8 multi-byte sequence).
  Otherwise, it's the first byte of a multi-byte sequence and the number of leading 1 bits indicates how many bytes there are in total for this sequence (110... means two bytes, 1110... means three bytes, etc).
*/

EZ_FORCE_INLINE bool ezUnicodeUtils::IsUtf8ContinuationByte(char uiByte)
{
  // check whether the two upper bits are set to '10'
  return (uiByte & 0xC0) == 0x80;
}

EZ_FORCE_INLINE bool ezUnicodeUtils::IsASCII(ezUInt32 uiChar)
{
  return (uiChar <= 127);
}

inline ezUInt32 ezUnicodeUtils::GetUtf8SequenceLength(char uiFirstByte)
{
  const ezUInt32 uiBit7 = uiFirstByte & EZ_BIT(7);
  const ezUInt32 uiBit6 = uiFirstByte & EZ_BIT(6);
  const ezUInt32 uiBit5 = uiFirstByte & EZ_BIT(5);
  const ezUInt32 uiBit4 = uiFirstByte & EZ_BIT(4);

  if (uiBit7 == 0) // ASCII character '0xxxxxxx'
    return 1;

  EZ_IGNORE_UNUSED(uiBit6);
  EZ_ASSERT_DEV(uiBit6 != 0, "Invalid Leading UTF-8 Byte.");

  if (uiBit5 == 0) // '110xxxxx'
    return 2;
  if (uiBit4 == 0) // '1110xxxx'
    return 3;

  // '1111xxxx'
  return 4;
}

template <typename ByteIterator>
ezUInt32 ezUnicodeUtils::DecodeUtf8ToUtf32(ByteIterator& szUtf8Iterator)
{
  return utf8::unchecked::next(szUtf8Iterator);
}

template <typename UInt16Iterator>
ezUInt32 ezUnicodeUtils::DecodeUtf16ToUtf32(UInt16Iterator& szUtf16Iterator)
{
  // internally this will move the iterator to the next character, so either 1 wchar further or two
  return utf8::utf16to32_next(szUtf16Iterator, szUtf16Iterator + 2); // +2 because a character could be encoded with two wchars
}

template <typename WCharIterator>
ezUInt32 ezUnicodeUtils::DecodeWCharToUtf32(WCharIterator& szWCharIterator)
{
  if (sizeof(wchar_t) == 2)
  {
    return DecodeUtf16ToUtf32(szWCharIterator);
  }
  else // sizeof(wchar_t) == 4
  {
    const ezUInt32 uiResult = *szWCharIterator;
    ++szWCharIterator;
    return uiResult;
  }
}

template <typename ByteIterator>
void ezUnicodeUtils::EncodeUtf32ToUtf8(ezUInt32 uiUtf32, ByteIterator& szUtf8Output)
{
  szUtf8Output = utf8::unchecked::utf32to8(&uiUtf32, &uiUtf32 + 1, szUtf8Output);
}

template <typename UInt16Iterator>
void ezUnicodeUtils::EncodeUtf32ToUtf16(ezUInt32 uiUtf32, UInt16Iterator& szUtf16Output)
{
  szUtf16Output = utf8::utf32to16(&uiUtf32, &uiUtf32 + 1, szUtf16Output);
}

template <typename WCharIterator>
void ezUnicodeUtils::EncodeUtf32ToWChar(ezUInt32 uiUtf32, WCharIterator& szWCharOutput)
{
  if (sizeof(wchar_t) == 2)
  {
    EncodeUtf32ToUtf16(uiUtf32, szWCharOutput);
  }
  else
  {
    *szWCharOutput = uiUtf32;
    ++szWCharOutput;
  }
}

inline ezUInt32 ezUnicodeUtils::ConvertUtf8ToUtf32(const char* pFirstChar)
{
  return utf8::unchecked::peek_next(pFirstChar);
}

inline ezUInt32 ezUnicodeUtils::GetSizeForCharacterInUtf8(ezUInt32 uiCharacter)
{
  // Basically implements this: http://en.wikipedia.org/wiki/Utf8#Description

  if (uiCharacter <= 0x0000007f)
    return 1;

  if (uiCharacter <= 0x000007ff) 
    return 2;

  if (uiCharacter <= 0x0000ffff)
    return 3;

  // UTF-8 can use up to 6 bytes to encode a code point
  // however some committee agreed that never more than 4 bytes are used (no need for more than 21 Bits)
  // this implementation assumes in several places, that the UTF-8 encoding never uses more than 4 bytes

  EZ_ASSERT_DEV(uiCharacter <= 0x0010ffff, "Invalid Unicode Codepoint %u", uiCharacter);
  return 4;
}

inline bool ezUnicodeUtils::IsValidUtf8(const char* szString, const char* szStringEnd)
{
  if (szStringEnd == ezMaxStringEnd)
    szStringEnd = szString + strlen(szString);

  return utf8::is_valid(szString, szStringEnd);
}

inline bool ezUnicodeUtils::SkipUtf8Bom(const char*& szUtf8)
{
  EZ_ASSERT_DEBUG(szUtf8 != nullptr, "This function expects non nullptr pointers");

  if (utf8::starts_with_bom(szUtf8, szUtf8 + 4))
  {
    szUtf8 += 3;
    return true;
  }

  return false;
}

inline bool ezUnicodeUtils::SkipUtf16BomLE(const ezUInt16*& szUtf16)
{
  EZ_ASSERT_DEBUG(szUtf16 != nullptr, "This function expects non nullptr pointers");

  if (*szUtf16 == ezUnicodeUtils::Utf16BomLE)
  {
    ++szUtf16;
    return true;
  }

  return false;
}

inline bool ezUnicodeUtils::SkipUtf16BomBE(const ezUInt16*& szUtf16)
{
  EZ_ASSERT_DEBUG(szUtf16 != nullptr, "This function expects non nullptr pointers");

  if (*szUtf16 == ezUnicodeUtils::Utf16BomBE)
  {
    ++szUtf16;
    return true;
  }

  return false;
}

inline void ezUnicodeUtils::MoveToNextUtf8(const char*& szUtf8, ezUInt32 uiNumCharacters)
{
  EZ_ASSERT_DEBUG(szUtf8 != nullptr, "Bad programmer!");

  while (uiNumCharacters > 0)
  {
    EZ_ASSERT_DEV(*szUtf8 != '\0', "The given string must not point to the zero terminator.");

    do
    {
      ++szUtf8;
    }
    while (IsUtf8ContinuationByte(*szUtf8));

    --uiNumCharacters;
  }
}

inline void ezUnicodeUtils::MoveToPriorUtf8(const char*& szUtf8, ezUInt32 uiNumCharacters)
{
  EZ_ASSERT_DEBUG(szUtf8 != nullptr, "Bad programmer!");

  while (uiNumCharacters > 0)
  {
    do
    {
      --szUtf8;
    }
    while (IsUtf8ContinuationByte(*szUtf8));

    --uiNumCharacters;
  }
}


