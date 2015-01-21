#pragma once

inline ezStringView::ezStringView()
{
  m_pStart = nullptr;
  m_pEnd = nullptr;
  m_pCurrent = nullptr;
  m_bValid = false;
}

inline ezStringView::ezStringView(const char* pStart)
{
  m_pStart = pStart;
  m_pEnd = pStart + ezStringUtils::GetStringElementCount(pStart);
  m_pCurrent = m_pStart;
  m_bValid = (m_pStart < m_pEnd);
}

inline ezStringView::ezStringView(const char* pStart, const char* pEnd)
{
  EZ_ASSERT_DEV(pStart <= pEnd, "It should start BEFORE it ends.");

  m_pStart = pStart;
  m_pEnd = pEnd;
  m_pCurrent = m_pStart;
  m_bValid = (m_pStart < m_pEnd);
}

inline void ezStringView::ResetToFront()
{
  m_pCurrent = m_pStart;
  m_bValid = (m_pCurrent < m_pEnd);
}

inline void ezStringView::ResetToBack()
{
  m_pCurrent = m_pEnd;

  if (m_pStart < m_pEnd)
  {
    m_bValid = true;
    ezUnicodeUtils::MoveToPriorUtf8(m_pCurrent);
  }
  else
    m_bValid = false;
}

inline void ezStringView::operator++()
{
  if (!m_bValid)
    return;

  ezUnicodeUtils::MoveToNextUtf8(m_pCurrent);

  m_bValid = (m_pCurrent < m_pEnd);
}

inline void ezStringView::operator--()
{
  if (!m_bValid || (m_pCurrent <= m_pStart))
  {
    m_bValid = false;
    return;
  }

  ezUnicodeUtils::MoveToPriorUtf8(m_pCurrent);
}

inline void ezStringView::operator+=(ezUInt32 d)
{
  while (d > 0)
  {
    ++(*this);
    --d;
  }
}

inline void ezStringView::operator-=(ezUInt32 d)
{
  while (d > 0)
  {
    --(*this);
    --d;
  }
}

inline ezUInt32 ezStringView::GetCharacter() const
{
  if (!m_bValid)
    return 0;

  return ezUnicodeUtils::ConvertUtf8ToUtf32(m_pCurrent);
}

inline bool ezStringView::IsValid() const
{
  return m_bValid;
}

inline void ezStringView::SetCurrentPosition(const char* szCurPos)
{
  EZ_ASSERT_DEV((szCurPos >= m_pStart) && (szCurPos <= m_pEnd), "New current position must still be inside the iterator's range.");

  m_pCurrent = szCurPos;
  m_bValid = (m_pCurrent < m_pEnd);
}

inline void ezStringView::Shrink(ezUInt32 uiShrinkCharsFront, ezUInt32 uiShrinkCharsBack)
{
  while ((m_pStart < m_pEnd) && (uiShrinkCharsFront > 0))
  {
    ezUnicodeUtils::MoveToNextUtf8(m_pStart, 1);
    --uiShrinkCharsFront;
  }

  while ((m_pStart < m_pEnd) && (uiShrinkCharsBack > 0))
  {
    ezUnicodeUtils::MoveToPriorUtf8(m_pEnd, 1);
    --uiShrinkCharsBack;
  }

  m_pCurrent = ezMath::Max<const char*>(m_pStart, m_pCurrent);

  m_bValid = (m_pCurrent < m_pEnd);
}

