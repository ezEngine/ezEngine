#pragma once

inline ezStringView::ezStringView()
{
  m_pStart = nullptr;
  m_pEnd = nullptr;
}

inline ezStringView::ezStringView(const char* pStart)
{
  m_pStart = pStart;
  m_pEnd = pStart + ezStringUtils::GetStringElementCount(pStart);
}

inline ezStringView::ezStringView(const char* pStart, const char* pEnd)
{
  EZ_ASSERT_DEV(pStart <= pEnd, "It should start BEFORE it ends.");

  m_pStart = pStart;
  m_pEnd = pEnd;
}

inline void ezStringView::operator++()
{
  if (!IsValid())
    return;

  ezUnicodeUtils::MoveToNextUtf8(m_pStart);
}

inline void ezStringView::operator+=(ezUInt32 d)
{
  while (d > 0)
  {
    ++(*this);
    --d;
  }
}

inline ezUInt32 ezStringView::GetCharacter() const
{
  if (!IsValid())
    return 0;

  return ezUnicodeUtils::ConvertUtf8ToUtf32(m_pStart);
}

inline bool ezStringView::IsValid() const
{
  return (m_pStart != nullptr) && (m_pStart < m_pEnd);
}

inline void ezStringView::SetStartPosition(const char* szCurPos)
{
  EZ_ASSERT_DEV((szCurPos >= m_pStart) && (szCurPos <= m_pEnd), "New start position must still be inside the view's range.");

  m_pStart = szCurPos;
}

inline void ezStringView::Shrink(ezUInt32 uiShrinkCharsFront, ezUInt32 uiShrinkCharsBack)
{
  while (IsValid() && (uiShrinkCharsFront > 0))
  {
    ezUnicodeUtils::MoveToNextUtf8(m_pStart, 1);
    --uiShrinkCharsFront;
  }

  while (IsValid() && (uiShrinkCharsBack > 0))
  {
    ezUnicodeUtils::MoveToPriorUtf8(m_pEnd, 1);
    --uiShrinkCharsBack;
  }
}

inline bool ezStringView::IsEqual(const ezStringView& sOther) const
{
  return ezStringUtils::IsEqualN(m_pStart, sOther.m_pStart, static_cast<ezUInt32>(-1), m_pEnd, sOther.m_pEnd);
}

inline bool ezStringView::IsEqual_NoCase(const ezStringView& sOther) const
{
  return ezStringUtils::IsEqualN_NoCase(m_pStart, sOther.m_pStart, static_cast<ezUInt32>(-1), m_pEnd, sOther.m_pEnd);
}

inline void ezStringView::Trim(const char* szTrimChars)
{
  return Trim(szTrimChars, szTrimChars);
}

inline void ezStringView::Trim(const char* szTrimCharsStart, const char* szTrimCharsEnd)
{
  if (IsValid())
    ezStringUtils::Trim(m_pStart, m_pEnd, szTrimCharsStart, szTrimCharsEnd);
}