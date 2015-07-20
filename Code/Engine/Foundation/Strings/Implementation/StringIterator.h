#pragma once

#include <Foundation/Strings/StringUtils.h>

template <typename Derived>
class ezStringBase;

/// \brief STL forward iterator used by all string classes. Iterates over unicode characters.
///  The iterator starts at the first character of the string and ends at the address beyond the last character of the string.
template<class STRING>
struct ezStringIterator
{
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef ezUInt32 value_type;
  typedef ptrdiff_t difference_type;
  typedef const char* pointer;
  typedef ezUInt32 reference;

  EZ_DECLARE_POD_TYPE();

  /// \brief Constructs an invalid iterator.
  EZ_FORCE_INLINE ezStringIterator() : m_String(nullptr), m_pElement(nullptr) { } // [tested]

  /// \brief Constructs either a begin or end iterator for the given string.
  EZ_FORCE_INLINE explicit ezStringIterator(const ezStringBase<STRING>& string, bool bIsEnd) : m_String(&string), m_pElement(nullptr) // [tested]
  {
    if (bIsEnd)
    {
      m_pElement = m_String->InternalGetDataEnd();
    }
    else
    {
      m_pElement = m_String->InternalGetData();
    }
  }

  /// \brief Checks whether this iterator points to a valid element. Invalid iterators either point to end(m_String) or were never initialized.
  EZ_FORCE_INLINE bool IsValid() const { return m_pElement != nullptr && m_pElement != m_String->InternalGetDataEnd(); } // [tested]
  
  /// \brief Returns the currently pointed to character in Utf32 encoding.
  EZ_FORCE_INLINE ezUInt32 GetCharacter() const { return IsValid() ? ezUnicodeUtils::ConvertUtf8ToUtf32(m_pElement) : ezUInt32(0); } // [tested]

  /// \brief Returns the currently pointed to character in Utf32 encoding.
  EZ_FORCE_INLINE ezUInt32 Value() const { return GetCharacter(); }

  /// \brief Returns the address the iterator currently points to.
  EZ_FORCE_INLINE const char* GetData() const { return m_pElement; } // [tested]

  /// \brief Checks whether the two iterators point to the same element.
  EZ_FORCE_INLINE bool operator==(const ezStringIterator& it2) const { return (m_pElement == it2.m_pElement); } // [tested]

  /// \brief Checks whether the two iterators point to the same element.
  EZ_FORCE_INLINE bool operator!=(const ezStringIterator& it2) const { return (m_pElement != it2.m_pElement); } // [tested]

  /// \brief Returns the currently pointed to character in Utf32 encoding.
  EZ_FORCE_INLINE ezUInt32 operator*() const { return GetCharacter(); } // [tested]

  /// \brief Returns the address the iterator currently points to.
  EZ_FORCE_INLINE const char* operator->() const { return GetData(); }

  /// \brief Move to the next Utf8 character
  EZ_FORCE_INLINE ezStringIterator<STRING>& operator++() // [tested]
  {
    if (m_pElement < m_String->InternalGetDataEnd())
      ezUnicodeUtils::MoveToNextUtf8(m_pElement);
    return *this;
  }

  /// \brief Move to the previous Utf8 character
  EZ_FORCE_INLINE ezStringIterator<STRING>& operator--() // [tested]
  {
    if (m_String->InternalGetData() < m_pElement)
      ezUnicodeUtils::MoveToPriorUtf8(m_pElement);
    return *this;
  }

  /// \brief Move to the next Utf8 character
  EZ_FORCE_INLINE ezStringIterator<STRING> operator++(int) // [tested]
  {
    ezStringIterator<STRING> tmp = *this;
    ++(*this);
    return tmp;
  }

  /// \brief Move to the previous Utf8 character
  EZ_FORCE_INLINE ezStringIterator<STRING> operator--(int) // [tested]
  {
    ezStringIterator<STRING> tmp = *this;
    --(*this);
    return tmp;
  }

  /// \brief Advances the iterator forwards by d characters. Does not move it beyond the range's end.
  EZ_FORCE_INLINE void operator+=(difference_type d)  // [tested]
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
  EZ_FORCE_INLINE ezStringIterator<STRING> operator+(difference_type d) const // [tested]
  {
    ezStringIterator<STRING> it = *this;
    it += d;
    return it;
  }

  /// \brief Returns an iterator that is advanced backwards by d characters.
  EZ_FORCE_INLINE ezStringIterator<STRING> operator-(difference_type d) const // [tested]
  {
    ezStringIterator<STRING> it = *this;
    it -= d;
    return it;
  }

  /// \brief Allows to set the 'current' iteration position to a different value.
  ///
  /// Must be between the iterators start and end range.
  void SetCurrentPosition(const char* szCurPos)
  {
    const char* szEnd = m_String->InternalGetDataEnd();
    EZ_ASSERT_DEV((szCurPos >= m_String->InternalGetData()) && (szCurPos <= szEnd), "New current position must still be inside the iterator's range.");

    m_pElement = szCurPos;
  }

protected:
  const ezStringBase<STRING>* m_String;
  const char* m_pElement;
};


/// \brief STL reverse iterator used by all string classes. Iterates over unicode characters.
///  The iterator starts at the last character of the string and ends at the address before the first character of the string.
template<class STRING>
struct ezStringReverseIterator
{
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef ezUInt32 value_type;
  typedef ptrdiff_t difference_type;
  typedef const char* pointer;
  typedef ezUInt32 reference;

  EZ_DECLARE_POD_TYPE();

  /// \brief Constructs an invalid iterator.
  EZ_FORCE_INLINE ezStringReverseIterator() : m_String(nullptr), m_pElement(nullptr) { } // [tested]

  /// \brief Constructs either a rbegin or rend iterator for the given string.
  EZ_FORCE_INLINE explicit ezStringReverseIterator(const ezStringBase<STRING>& string, bool bIsEnd) : m_String(&string), m_pElement(nullptr) // [tested]
  {
    if (bIsEnd)
    {
      m_pElement = nullptr;
    }
    else
    {
      m_pElement = m_String->InternalGetDataEnd();
      ezUnicodeUtils::MoveToPriorUtf8(m_pElement);
    }
  }

  /// \brief Checks whether this iterator points to a valid element.
  EZ_FORCE_INLINE bool IsValid() const { return (m_pElement != nullptr); } // [tested]

  /// \brief Returns the currently pointed to character in Utf32 encoding.
  EZ_FORCE_INLINE ezUInt32 GetCharacter() const { return IsValid() ? ezUnicodeUtils::ConvertUtf8ToUtf32(m_pElement) : ezUInt32(0); } // [tested]

  /// \brief Returns the address the iterator currently points to.
  EZ_FORCE_INLINE const char* GetData() const { return m_pElement; } // [tested]

  /// \brief Checks whether the two iterators point to the same element.
  EZ_FORCE_INLINE bool operator==(const ezStringReverseIterator& it2) const { return (m_pElement == it2.m_pElement); } // [tested]

  /// \brief Checks whether the two iterators point to the same element.
  EZ_FORCE_INLINE bool operator!=(const ezStringReverseIterator& it2) const { return (m_pElement != it2.m_pElement); } // [tested]

  /// \brief Returns the currently pointed to character in Utf32 encoding.
  EZ_FORCE_INLINE ezUInt32 operator*() const { return GetCharacter(); } // [tested]

  /// \brief Returns the address the iterator currently points to.
  EZ_FORCE_INLINE const char* operator->() const { return GetData(); } // [tested]

  /// \brief Move to the next Utf8 character
  EZ_FORCE_INLINE ezStringReverseIterator<STRING>& operator++() // [tested]
  {
    if (m_pElement != nullptr && m_String->InternalGetData() < m_pElement)
      ezUnicodeUtils::MoveToPriorUtf8(m_pElement);
    else
      m_pElement = nullptr;
    return *this;
  }

  /// \brief Move to the previous Utf8 character
  EZ_FORCE_INLINE ezStringReverseIterator<STRING>& operator--() // [tested]
  {
    if (m_pElement != nullptr)
    {
      const char* szOldPos = m_pElement;
      ezUnicodeUtils::MoveToNextUtf8(m_pElement);
      if (m_pElement == m_String->InternalGetDataEnd())
        m_pElement = szOldPos;
    }
    else
    {
      // Set back to the first character.
      m_pElement = m_String->InternalGetData();
    }
    return *this;
  }

  /// \brief Move to the next Utf8 character
  EZ_FORCE_INLINE ezStringReverseIterator<STRING> operator++(int) // [tested]
  {
    ezStringReverseIterator<STRING> tmp = *this;
    ++(*this);
    return tmp;
  }

  /// \brief Move to the previous Utf8 character
  EZ_FORCE_INLINE ezStringReverseIterator<STRING> operator--(int) // [tested]
  {
    ezStringReverseIterator<STRING> tmp = *this;
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
  EZ_FORCE_INLINE ezStringReverseIterator<STRING> operator+(difference_type d) const // [tested]
  {
    ezStringReverseIterator<STRING> it = *this;
    it += d;
    return it;
  }

  /// \brief Returns an iterator that is advanced backwards by d characters.
  EZ_FORCE_INLINE ezStringReverseIterator<STRING> operator-(difference_type d) const // [tested]
  {
    ezStringReverseIterator<STRING> it = *this;
    it -= d;
    return it;
  }

  /// \brief Allows to set the 'current' iteration position to a different value.
  ///
  /// Must be between the iterators start and end range.
  EZ_FORCE_INLINE void SetCurrentPosition(const char* szCurPos)
  {
    const char* szBegin = m_String->InternalGetData();
    const char* szEnd = m_String->InternalGetDataEnd();
    EZ_ASSERT_DEV(szCurPos == nullptr || (szCurPos >= szBegin) && (szCurPos < szEnd), "New current position must still be inside the iterator's range.");

    m_pElement = szCurPos;
  }

protected:
  const ezStringBase<STRING>* m_String;
  const char* m_pElement;
};

