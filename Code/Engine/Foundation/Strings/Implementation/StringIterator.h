#pragma once

#ifndef EZ_INCLUDING_BASICS_H
#  error "Please don't include StringIterator.h directly, but instead include Foundation/Basics.h"
#endif

/// \brief STL forward iterator used by all string classes. Iterates over unicode characters.
///  The iterator starts at the first character of the string and ends at the address beyond the last character of the string.
struct ezStringIterator
{
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = ezUInt32;
  using difference_type = std::ptrdiff_t;
  using pointer = const char*;
  using reference = ezUInt32;

  EZ_DECLARE_POD_TYPE();

  /// \brief Constructs an invalid iterator.
  EZ_ALWAYS_INLINE ezStringIterator() = default; // [tested]

  /// \brief Constructs either a begin or end iterator for the given string.
  EZ_FORCE_INLINE explicit ezStringIterator(const char* pStartPtr, const char* pEndPtr, const char* pCurPtr)
  {
    m_pStartPtr = pStartPtr;
    m_pEndPtr = pEndPtr;
    m_pCurPtr = pCurPtr;
  }

  /// \brief Checks whether this iterator points to a valid element. Invalid iterators either point to m_pEndPtr or were never initialized.
  EZ_ALWAYS_INLINE bool IsValid() const { return m_pCurPtr != nullptr && m_pCurPtr != m_pEndPtr; } // [tested]

  /// \brief Returns the currently pointed to character in Utf32 encoding.
  EZ_ALWAYS_INLINE ezUInt32 GetCharacter() const { return IsValid() ? ezUnicodeUtils::ConvertUtf8ToUtf32(m_pCurPtr) : ezUInt32(0); } // [tested]

  /// \brief Returns the currently pointed to character in Utf32 encoding.
  EZ_ALWAYS_INLINE ezUInt32 operator*() const { return GetCharacter(); } // [tested]

  /// \brief Returns the address the iterator currently points to.
  EZ_ALWAYS_INLINE const char* GetData() const { return m_pCurPtr; } // [tested]

  /// \brief Checks whether the two iterators point to the same element.
  EZ_ALWAYS_INLINE bool operator==(const ezStringIterator& it2) const { return (m_pCurPtr == it2.m_pCurPtr); } // [tested]
  EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const ezStringIterator&);

  /// \brief Advances the iterated to the next character, same as operator++, but returns how many bytes were consumed in the source string.
  EZ_ALWAYS_INLINE ezUInt32 Advance()
  {
    const char* pPrevElement = m_pCurPtr;

    if (m_pCurPtr < m_pEndPtr)
    {
      ezUnicodeUtils::MoveToNextUtf8(m_pCurPtr).AssertSuccess();
    }

    return static_cast<ezUInt32>(m_pCurPtr - pPrevElement);
  }

  /// \brief Move to the next Utf8 character
  EZ_ALWAYS_INLINE ezStringIterator& operator++() // [tested]
  {
    if (m_pCurPtr < m_pEndPtr)
    {
      ezUnicodeUtils::MoveToNextUtf8(m_pCurPtr).AssertSuccess();
    }

    return *this;
  }

  /// \brief Move to the previous Utf8 character
  EZ_ALWAYS_INLINE ezStringIterator& operator--() // [tested]
  {
    if (m_pStartPtr < m_pCurPtr)
    {
      ezUnicodeUtils::MoveToPriorUtf8(m_pCurPtr, m_pStartPtr).AssertSuccess();
    }

    return *this;
  }

  /// \brief Move to the next Utf8 character
  EZ_ALWAYS_INLINE ezStringIterator operator++(int) // [tested]
  {
    ezStringIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  /// \brief Move to the previous Utf8 character
  EZ_ALWAYS_INLINE ezStringIterator operator--(int) // [tested]
  {
    ezStringIterator tmp = *this;
    --(*this);
    return tmp;
  }

  /// \brief Advances the iterator forwards by d characters. Does not move it beyond the range's end.
  EZ_FORCE_INLINE void operator+=(difference_type d) // [tested]
  {
    while (d > 0)
    {
      ++(*this);
      --d;
    }
    while (d < 0)
    {
      --(*this);
      ++d;
    }
  }

  /// \brief Moves the iterator backwards by d characters. Does not move it beyond the range's start.
  EZ_FORCE_INLINE void operator-=(difference_type d) // [tested]
  {
    while (d > 0)
    {
      --(*this);
      --d;
    }
    while (d < 0)
    {
      ++(*this);
      ++d;
    }
  }

  /// \brief Returns an iterator that is advanced forwards by d characters.
  EZ_ALWAYS_INLINE ezStringIterator operator+(difference_type d) const // [tested]
  {
    ezStringIterator it = *this;
    it += d;
    return it;
  }

  /// \brief Returns an iterator that is advanced backwards by d characters.
  EZ_ALWAYS_INLINE ezStringIterator operator-(difference_type d) const // [tested]
  {
    ezStringIterator it = *this;
    it -= d;
    return it;
  }

  /// \brief Allows to set the 'current' iteration position to a different value.
  ///
  /// Must be between the iterators start and end range.
  void SetCurrentPosition(const char* szCurPos)
  {
    EZ_ASSERT_DEV((szCurPos >= m_pStartPtr) && (szCurPos <= m_pEndPtr), "New position must still be inside the iterator's range.");

    m_pCurPtr = szCurPos;
  }

private:
  const char* m_pStartPtr = nullptr;
  const char* m_pEndPtr = nullptr;
  const char* m_pCurPtr = nullptr;
};


/// \brief STL reverse iterator used by all string classes. Iterates over unicode characters.
///  The iterator starts at the last character of the string and ends at the address before the first character of the string.
struct ezStringReverseIterator
{
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = ezUInt32;
  using difference_type = std::ptrdiff_t;
  using pointer = const char*;
  using reference = ezUInt32;

  EZ_DECLARE_POD_TYPE();

  /// \brief Constructs an invalid iterator.
  EZ_ALWAYS_INLINE ezStringReverseIterator() = default; // [tested]

  /// \brief Constructs either a rbegin or rend iterator for the given string.
  EZ_FORCE_INLINE explicit ezStringReverseIterator(const char* pStartPtr, const char* pEndPtr, const char* pCurPtr) // [tested]
  {
    m_pStartPtr = pStartPtr;
    m_pEndPtr = pEndPtr;
    m_pCurPtr = pCurPtr;

    if (m_pStartPtr >= m_pEndPtr)
    {
      m_pCurPtr = nullptr;
    }
    else if (m_pCurPtr == m_pEndPtr)
    {
      ezUnicodeUtils::MoveToPriorUtf8(m_pCurPtr, m_pStartPtr).AssertSuccess();
    }
  }

  /// \brief Checks whether this iterator points to a valid element.
  EZ_ALWAYS_INLINE bool IsValid() const { return (m_pCurPtr != nullptr); } // [tested]

  /// \brief Returns the currently pointed to character in Utf32 encoding.
  EZ_ALWAYS_INLINE ezUInt32 GetCharacter() const { return IsValid() ? ezUnicodeUtils::ConvertUtf8ToUtf32(m_pCurPtr) : ezUInt32(0); } // [tested]

  /// \brief Returns the currently pointed to character in Utf32 encoding.
  EZ_ALWAYS_INLINE ezUInt32 operator*() const { return GetCharacter(); } // [tested]

  /// \brief Returns the address the iterator currently points to.
  EZ_ALWAYS_INLINE const char* GetData() const { return m_pCurPtr; } // [tested]

  /// \brief Checks whether the two iterators point to the same element.
  EZ_ALWAYS_INLINE bool operator==(const ezStringReverseIterator& it2) const { return (m_pCurPtr == it2.m_pCurPtr); } // [tested]
  EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const ezStringReverseIterator&);

  /// \brief Move to the next Utf8 character
  EZ_FORCE_INLINE ezStringReverseIterator& operator++() // [tested]
  {
    if (m_pCurPtr != nullptr && m_pStartPtr < m_pCurPtr)
      ezUnicodeUtils::MoveToPriorUtf8(m_pCurPtr, m_pStartPtr).AssertSuccess();
    else
      m_pCurPtr = nullptr;

    return *this;
  }

  /// \brief Move to the previous Utf8 character
  EZ_FORCE_INLINE ezStringReverseIterator& operator--() // [tested]
  {
    if (m_pCurPtr != nullptr)
    {
      const char* szOldPos = m_pCurPtr;
      ezUnicodeUtils::MoveToNextUtf8(m_pCurPtr).AssertSuccess();

      if (m_pCurPtr == m_pEndPtr)
        m_pCurPtr = szOldPos;
    }
    else
    {
      // Set back to the first character.
      m_pCurPtr = m_pStartPtr;
    }
    return *this;
  }

  /// \brief Move to the next Utf8 character
  EZ_ALWAYS_INLINE ezStringReverseIterator operator++(int) // [tested]
  {
    ezStringReverseIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  /// \brief Move to the previous Utf8 character
  EZ_ALWAYS_INLINE ezStringReverseIterator operator--(int) // [tested]
  {
    ezStringReverseIterator tmp = *this;
    --(*this);
    return tmp;
  }

  /// \brief Advances the iterator forwards by d characters. Does not move it beyond the range's end.
  EZ_FORCE_INLINE void operator+=(difference_type d) // [tested]
  {
    while (d > 0)
    {
      ++(*this);
      --d;
    }
    while (d < 0)
    {
      --(*this);
      ++d;
    }
  }

  /// \brief Moves the iterator backwards by d characters. Does not move it beyond the range's start.
  EZ_FORCE_INLINE void operator-=(difference_type d) // [tested]
  {
    while (d > 0)
    {
      --(*this);
      --d;
    }
    while (d < 0)
    {
      ++(*this);
      ++d;
    }
  }

  /// \brief Returns an iterator that is advanced forwards by d characters.
  EZ_ALWAYS_INLINE ezStringReverseIterator operator+(difference_type d) const // [tested]
  {
    ezStringReverseIterator it = *this;
    it += d;
    return it;
  }

  /// \brief Returns an iterator that is advanced backwards by d characters.
  EZ_ALWAYS_INLINE ezStringReverseIterator operator-(difference_type d) const // [tested]
  {
    ezStringReverseIterator it = *this;
    it -= d;
    return it;
  }

  /// \brief Allows to set the 'current' iteration position to a different value.
  ///
  /// Must be between the iterators start and end range.
  EZ_FORCE_INLINE void SetCurrentPosition(const char* szCurPos)
  {
    EZ_ASSERT_DEV((szCurPos == nullptr) || ((szCurPos >= m_pStartPtr) && (szCurPos < m_pEndPtr)), "New position must still be inside the iterator's range.");

    m_pCurPtr = szCurPos;
  }

private:
  const char* m_pStartPtr = nullptr;
  const char* m_pEndPtr = nullptr;
  const char* m_pCurPtr = nullptr;
};
