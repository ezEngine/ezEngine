#pragma once

#include <ThirdParty/utf8/utf8.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/Implementation/StringBase.h>

/// \brief ezStringIterator allows for character-wise iterating over a (read-only) Utf8 string.
///
/// It can also be used to represent a sub-string of a larger string, as it can store a dedicated string end position.
/// It derives from ezStringBase and thus provides a large set of functions for search and comparisons.
class EZ_FOUNDATION_DLL ezStringIterator : public ezStringBase<ezStringIterator>
{
public:
  /// \brief Default constructor creates an invalid iterator.
  ezStringIterator();

  /// \brief Creates a string iterator starting at the given position, ending at the next '\0' terminator.
  ///
  /// If bIsPureASCII is set to true, a little more efficient iterating method is used.
  /// By default this flag should be kept false, as it will ensure that iterating will always work.
  explicit ezStringIterator(const char* pCurrent, bool bIsPureASCII = false); // [tested]

  /// \brief Creates a string iterator for the range from pFirst to pEnd, which currently points to pCurrent.
  ///
  /// If bIsPureASCII is set to true, a little more efficient iterating method is used.
  /// By default this flag should be kept false, as it will ensure that iterating will always work.
  ezStringIterator(const char* pFirst, const char* pEnd, const char* pCurrent, bool bIsPureASCII = false); // [tested]

  /// \brief Advances the iterator to the next character, unless the end of the range was reached.
  void operator++(); // [tested]

  /// \brief Moves the iteratore backwards to the previous character, unless the start of the range was reached.
  void operator--(); // [tested]

  /// \brief Advances the iterator forwards by d characters. Does not move it beyond the range's end.
  void operator+=(ezUInt32 d); // [tested]

  /// \brief Moves the iterator backwards by d characters. Does not move it beyond the range's start.
  void operator-=(ezUInt32 d); // [tested]

  /// \brief Returns the currently pointed to character in Utf32 encoding.
  ezUInt32 GetCharacter() const; // [tested]

  /// \brief Returns true, if the current iterator position is valid and will return a valid character.
  ///
  /// It will return false, if iteration has reached one of the ends. Use 'ResetToFront', 'ResetToBack' or simply 'SetCurrentPosition'
  /// to set the iterator to a valid starting position again.
  bool IsValid() const; // [tested]

  /// \brief Returns the string, starting at the current iteration position.
  const char* GetData() const { return m_pCurrent; } // [tested]

  /// \brief Returns the number of bytes from the current iteration position up to its end.
  ///
  /// \note Note that the element count (bytes) may be larger than the number of characters in that string, due to Utf8 encoding.
  ezUInt32 GetElementCount() const { return (ezUInt32) (m_pEnd - m_pCurrent); } // [tested]

  /// \brief Allows to set the 'current' iteration position to a different value.
  ///
  /// Must be between the iterators start and end range.
  void SetCurrentPosition(const char* szCurPos); // [tested]

  /// \brief Returns whether this sub-string represents a simple ASCII string, ie. whether the byte count and the character count will be identical.
  bool IsPureASCII() const { return m_bIsPureASCII; } // [tested]

  /// \brief Returns the start of the iterator range.
  const char* GetStart() const { return m_pFirst; } // [tested]

  /// \brief Returns the end of the iterator range. This will point to the byte AFTER the last character.
  ///
  /// That means it might point to the '\0' terminator, UNLESS the iterator only represents a sub-string of a larger string.
  /// Accessing the value at 'GetEnd' has therefore no real use.
  const char* GetEnd() const { return m_pEnd; } // [tested]

  /// \brief Shrinks the iterator range by uiShrinkCharsFront characters at the front and by uiShrinkCharsBack characters at the back.
  ///
  /// Thus reduces the range of the iterator to a smalle sub-string.
  /// The current position is clamped to the new start of the range.
  /// The new end position is clamped to the new start of the range.
  /// If more characters are removed from the range, than it actually contains, the iterator range will become 'empty' 
  /// and its state will be set to invalid, however no error or assert will be triggered.
  void Shrink(ezUInt32 uiShrinkCharsFront, ezUInt32 uiShrinkCharsBack); // [tested]

  /// \brief Resets the current position to the first character and flags the iterator as valid again (unless its range is empty).
  void ResetToFront(); // [tested]

  /// \brief Resets the current position to the last character and flags the iterator as valid again (unless its range is empty).
  void ResetToBack(); // [tested]

  /// \brief Compares the two stringiterators for equality. Both do not need to be null terminated, but must have the same length.
  bool operator==(const ezStringIterator& rhs) const;

  /// \brief Compares the two stringiterators for equality. Both do not need to be null terminated, but must have the same length.
  bool operator!=(const ezStringIterator& rhs) const;

private:
  bool m_bValid;
  bool m_bIsPureASCII;
  const char* m_pFirst;
  const char* m_pEnd;
  const char* m_pCurrent;
};

#include <Foundation/Strings/Implementation/StringIterator_inl.h>

