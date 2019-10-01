#pragma once

inline ezStringView::ezStringView() = default;

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

inline ezStringView::ezStringView(const char* pStart, ezUInt32 uiLength)
{
  m_pStart = pStart;
  m_pEnd = pStart + uiLength;
}

inline void ezStringView::operator++()
{
  if (!IsValid())
    return;

  ezUnicodeUtils::MoveToNextUtf8(m_pStart);
}

inline void ezStringView::operator+=(ezUInt32 d)
{
  ezUnicodeUtils::MoveToNextUtf8(m_pStart, d);
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

