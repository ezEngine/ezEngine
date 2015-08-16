#pragma once

#include <Foundation/Basics.h>
#include <ThirdParty/utf8/utf8.h>

// Minus 1 wraps around to the maximum unsigned integer value, which is different on 32 Bit and 64 Bit
#define ezMaxStringEnd (const char*)-1

/// \brief Helper functions to work with Unicode.
class EZ_FOUNDATION_DLL ezUnicodeUtils
{
public:
  /// \brief Byte Order Mark for Little Endian Utf16 strings.
  static const ezUInt16 Utf16BomLE = 0xfffe;

  /// \brief Byte Order Mark for Big Endian Utf16 strings.
  static const ezUInt16 Utf16BomBE = 0xfeff;

  /// \brief Returns whether a character is a pure ASCII character (only the first 7 Bits are used)
  static bool IsASCII(ezUInt32 uiChar);// [tested]

  /// \brief Checks whether the given byte is a byte in a UTF-8 multi-byte sequence.
  static bool IsUtf8ContinuationByte(char uiByte); // [tested]

  /// \brief Returns the number of bytes that a UTF-8 sequence is in length, which is encoded in the first byte of the sequence.
  static ezUInt32 GetUtf8SequenceLength(char uiFirstByte); // [tested]

  /// \brief Converts the UTF-8 character that starts at pFirstChar into a UTF-32 character.
  static ezUInt32 ConvertUtf8ToUtf32(const char* pFirstChar); // [tested]

  /// \brief Computes how many bytes the character would require, if encoded in UTF-8.
  static ezUInt32 GetSizeForCharacterInUtf8(ezUInt32 uiCharacter); // [tested]

  /// \brief Moves the given string pointer ahead to the next Utf8 character sequence.
  ///
  /// The string may point to an invalid position (in between a character sequence).
  /// It may not point to a zero terminator already.
  static void MoveToNextUtf8(const char*& szUtf8, ezUInt32 uiNumCharacters = 1); // [tested]

  /// \brief Moves the given string pointer backwards to the previous Utf8 character sequence.
  ///
  /// The string may point to an invalid position (in between a character sequence), or even the \0 terminator, 
  /// as long as there is a valid string before it (and the user knows when to stop).
  static void MoveToPriorUtf8(const char*& szUtf8, ezUInt32 uiNumCharacters = 1); // [tested]

  /// \brief Returns false if the given string does not contain a completely valid Utf8 string.
  static bool IsValidUtf8(const char* szString, const char* szStringEnd = ezMaxStringEnd);

  /// \brief If the given string starts with a Utf8 Bom, the pointer is incremented behind the Bom, and the function returns true.
  ///
  /// Otherwise the pointer is unchanged and false is returned.
  static bool SkipUtf8Bom(const char*& szUtf8); // [tested]

  /// \brief If the given string starts with a Utf16 little endian Bom, the pointer is incremented behind the Bom, and the function returns true.
  ///
  /// Otherwise the pointer is unchanged and false is returned.
  static bool SkipUtf16BomLE(const ezUInt16*& szUtf16); // [tested]

  /// \brief If the given string starts with a Utf16 big endian Bom, the pointer is incremented behind the Bom, and the function returns true.
  ///
  /// Otherwise the pointer is unchanged and false is returned.
  static bool SkipUtf16BomBE(const ezUInt16*& szUtf16); // [tested]

  /// \brief Decodes the next character from the given Utf8 sequence to Utf32 and increments the iterator as far as necessary.
  template <typename ByteIterator>
  static ezUInt32 DecodeUtf8ToUtf32(ByteIterator& szUtf8Iterator); // [tested]

  /// \brief Decodes the next character from the given Utf16 sequence to Utf32 and increments the iterator as far as necessary.
  template <typename UInt16Iterator>
  static ezUInt32 DecodeUtf16ToUtf32(UInt16Iterator& szUtf16Iterator); // [tested]

  /// \brief Decodes the next character from the given wchar_t sequence to Utf32 and increments the iterator as far as necessary.
  template <typename WCharIterator>
  static ezUInt32 DecodeWCharToUtf32(WCharIterator& szWCharIterator); // [tested]

  /// \brief Encodes the given Utf32 character to Utf8 and writes as many bytes to the output iterator, as necessary.
  template <typename ByteIterator>
  static void EncodeUtf32ToUtf8(ezUInt32 uiUtf32, ByteIterator& szUtf8Output); // [tested]

  /// \brief Encodes the given Utf32 character to Utf16 and writes as many bytes to the output iterator, as necessary.
  template <typename UInt16Iterator>
  static void EncodeUtf32ToUtf16(ezUInt32 uiUtf32, UInt16Iterator& szUtf16Output); // [tested]

  /// \brief Encodes the given Utf32 character to wchar_t and writes as many bytes to the output iterator, as necessary.
  template <typename WCharIterator>
  static void EncodeUtf32ToWChar(ezUInt32 uiUtf32, WCharIterator& szWCharOutput); // [tested]

  /// \brief [internal] Small helper class to append bytes to some arbitrary container. Used for Utf8 string building.
  template <typename IntType, typename Container>
  struct UtfInserter
  {
    EZ_FORCE_INLINE UtfInserter(Container* pContainer) { m_pContainer = pContainer; }
    EZ_FORCE_INLINE void operator++() {}
    EZ_FORCE_INLINE UtfInserter& operator++(int) { return *this; }
    EZ_FORCE_INLINE void operator= (IntType rhs) { m_pContainer->PushBack(rhs); }
    EZ_FORCE_INLINE UtfInserter& operator*() { return *this; }

    Container* m_pContainer;
  };
};


#include <Foundation/Strings/Implementation/UnicodeUtils_inl.h>

