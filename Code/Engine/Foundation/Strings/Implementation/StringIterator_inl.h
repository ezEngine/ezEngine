#pragma once

inline ezStringIterator::ezStringIterator()
{
  m_pFirst = nullptr;
  m_pEnd = nullptr;
  m_pCurrent = nullptr;
  m_bValid = false;
  m_bIsPureASCII = true;
}

inline ezStringIterator::ezStringIterator(const char* pCurrent, bool bIsPureASCII)
{
  m_pFirst = pCurrent;
  m_pEnd = (pCurrent == nullptr) ? nullptr : (pCurrent + ezStringUtils::GetStringElementCount(pCurrent));
  m_pCurrent = pCurrent;
  m_bValid = (pCurrent < m_pEnd);
  m_bIsPureASCII = bIsPureASCII;
}

inline ezStringIterator::ezStringIterator(const char* pFirst, const char* pEnd, const char* pCurrent, bool bIsPureASCII)
{
  EZ_ASSERT(pFirst <= pEnd, "It should start BEFORE it ends.");
  EZ_ASSERT(pCurrent >= pFirst && pCurrent <= pEnd, "The current position must be between the start and end position.");

  m_pFirst = pFirst;
  m_pEnd = pEnd;
  m_pCurrent = pCurrent;
  m_bValid = (pCurrent >= pFirst) && (pCurrent < m_pEnd);
  m_bIsPureASCII = bIsPureASCII;
}

inline void ezStringIterator::ResetToFront()
{
  m_pCurrent = m_pFirst;
  m_bValid = (m_pCurrent < m_pEnd);
}

inline void ezStringIterator::ResetToBack()
{
  m_pCurrent = m_pEnd;

  if (m_pFirst < m_pEnd)
  {
    m_bValid = true;
    ezUnicodeUtils::MoveToPriorUtf8(m_pCurrent);
  }
  else
    m_bValid = false;
}

inline void ezStringIterator::operator++()
{
  if (!m_bValid)
    return;

  if (m_bIsPureASCII)
    ++m_pCurrent;
  else
    ezUnicodeUtils::MoveToNextUtf8(m_pCurrent);

  m_bValid = (m_pCurrent < m_pEnd);
}

inline void ezStringIterator::operator--()
{
  if (!m_bValid || (m_pCurrent <= m_pFirst))
  {
    m_bValid = false;
    return;
  }

  if (m_bIsPureASCII)
    --m_pCurrent;
  else
    ezUnicodeUtils::MoveToPriorUtf8(m_pCurrent);
}

inline void ezStringIterator::operator+=(ezUInt32 d)
{
  while (d > 0)
  {
    ++(*this);
    --d;
  }
}

inline void ezStringIterator::operator-=(ezUInt32 d)
{
  while (d > 0)
  {
    --(*this);
    --d;
  }
}

inline ezUInt32 ezStringIterator::GetCharacter() const
{
  if (!m_bValid)
    return 0;

  return ezUnicodeUtils::ConvertUtf8ToUtf32(m_pCurrent);
}

inline bool ezStringIterator::IsValid() const
{
  return m_bValid;
}

inline void ezStringIterator::SetCurrentPosition(const char* szCurPos)
{
  EZ_ASSERT((szCurPos >= m_pFirst) && (szCurPos <= m_pEnd), "New current position must still be inside the iterator's range.");

  m_pCurrent = szCurPos;
  m_bValid = (m_pCurrent < m_pEnd);
}

inline void ezStringIterator::Shrink(ezUInt32 uiShrinkCharsFront, ezUInt32 uiShrinkCharsBack)
{
  while ((m_pFirst < m_pEnd) && (uiShrinkCharsFront > 0))
  {
    ezUnicodeUtils::MoveToNextUtf8(m_pFirst, 1);
    --uiShrinkCharsFront;
  }

  while ((m_pFirst < m_pEnd) && (uiShrinkCharsBack > 0))
  {
    ezUnicodeUtils::MoveToPriorUtf8(m_pEnd, 1);
    --uiShrinkCharsBack;
  }

  m_pCurrent = ezMath::Max<const char*>(m_pFirst, m_pCurrent);

  m_bValid = (m_pCurrent < m_pEnd);
}

inline bool ezStringIterator::operator==(const char* rhs) const
{
  /// \test This is new

  const ezUInt64 uiLen = m_pEnd - m_pFirst;

  return ezStringUtils::IsEqualN(m_pFirst, rhs, (ezUInt32) uiLen) && (*(rhs + uiLen) == '\0');
}

EZ_FORCE_INLINE bool ezStringIterator::operator!=(const char* rhs) const
{
  /// \test This is new

  return !operator==(rhs);
}

inline bool ezStringIterator::operator==(const ezStringIterator& rhs) const
{
  /// \test This is new

  const ezUInt64 uiLen = m_pEnd - m_pFirst;
  return (uiLen == (ezUInt64) (rhs.m_pEnd - rhs.m_pFirst)) && ezStringUtils::IsEqualN(m_pFirst, rhs.m_pFirst, (ezUInt32) uiLen);
}

EZ_FORCE_INLINE bool ezStringIterator::operator!=(const ezStringIterator& rhs) const
{
  /// \test This is new

  return !operator==(rhs);
}
