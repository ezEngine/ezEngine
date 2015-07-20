#pragma once

#include <ThirdParty/utf8/utf8.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/Implementation/StringBase.h>

/// \brief ezStringView represent a read-only sub-string of a larger string, as it can store a dedicated string end position.
/// It derives from ezStringBase and thus provides a large set of functions for search and comparisons.
///
/// Attention: ezStringView does not store string data itself. It only stores pointers into memory. For example,
/// when you get an ezStringView to an ezStringBuilder, the ezStringView instance will point to the exact same memory,
/// enabling you to iterate over it (read-only).
/// That means that an ezStringView is only valid as long as its source data is not modified. Once you make any kind
/// of modification to the source data, you should not continue using the ezStringView to that data anymore,
/// as it might now point into invalid memory.
class EZ_FOUNDATION_DLL ezStringView : public ezStringBase<ezStringView>
{
public:
  /// \brief Default constructor creates an invalid view.
  ezStringView();

  /// \brief Creates a string view starting at the given position, ending at the next '\0' terminator.
  ezStringView(const char* pStart); // [tested]

  /// \brief Creates a string view for the range from pStart to pEnd.
  ezStringView(const char* pStart, const char* pEnd); // [tested]

  /// \brief Advances the start to the next character, unless the end of the range was reached.
  void operator++(); // [tested]

  /// \brief Advances the start forwards by d characters. Does not move it beyond the range's end.
  void operator+=(ezUInt32 d); // [tested]

  /// \brief Returns the first pointed to character in Utf32 encoding.
  ezUInt32 GetCharacter() const; // [tested]

  /// \brief Returns true, if the current string pointed to is non empty.
  bool IsValid() const; // [tested]

  // no implicit conversion to ezStringParamImpl or const char* because the string view may not be zero terminated
  //operator ezStringParamImpl () const;
  //operator const char* () const { return GetData(); }

  /// \brief Returns the string. May not be zero-terminated.
  const char* GetData() const { return m_pStart; } // [tested]

  /// \brief Returns the number of bytes from the start position up to its end.
  ///
  /// \note Note that the element count (bytes) may be larger than the number of characters in that string, due to Utf8 encoding.
  ezUInt32 GetElementCount() const { return (ezUInt32) (m_pEnd - m_pStart); } // [tested]

  /// \brief Allows to set the start position to a different value.
  ///
  /// Must be between the current start and end range.
  void SetStartPosition(const char* szCurPos); // [tested]

  /// \brief Returns the start of the view range.
  const char* GetStartPosition() const { return m_pStart; } // [tested]

  /// \brief Returns the end of the view range. This will point to the byte AFTER the last character.
  ///
  /// That means it might point to the '\0' terminator, UNLESS the view only represents a sub-string of a larger string.
  /// Accessing the value at 'GetEnd' has therefore no real use.
  const char* GetEndPosition() const { return m_pEnd; } // [tested]

  using ezStringBase<ezStringView>::IsEqual;

  /// \brief Compares this string view with the other string view for equality.
  bool IsEqual(const ezStringView& sOther) const;

  using ezStringBase<ezStringView>::IsEqual_NoCase;

  /// \brief Compares this string view with the other string view for equality.
  bool IsEqual_NoCase(const ezStringView& sOther) const;

  /// \brief Shrinks the view range by uiShrinkCharsFront characters at the front and by uiShrinkCharsBack characters at the back.
  ///
  /// Thus reduces the range of the view to a smaller sub-string.
  /// The current position is clamped to the new start of the range.
  /// The new end position is clamped to the new start of the range.
  /// If more characters are removed from the range, than it actually contains, the view range will become 'empty' 
  /// and its state will be set to invalid, however no error or assert will be triggered.
  void Shrink(ezUInt32 uiShrinkCharsFront, ezUInt32 uiShrinkCharsBack); // [tested]

  /// \brief Removes all characters from the start and end that appear in the given strings by adjusting the begin and end of the view.
  void Trim(const char* szTrimChars); // [tested]

  /// \brief Removes all characters from the start and/or end that appear in the given strings by adjusting the begin and end of the view.
  void Trim(const char* szTrimCharsStart, const char* szTrimCharsEnd); // [tested]

private:
  const char* m_pStart;
  const char* m_pEnd;
};



#include <Foundation/Strings/Implementation/StringView_inl.h>

