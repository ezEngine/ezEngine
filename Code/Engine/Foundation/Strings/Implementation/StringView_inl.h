#pragma once

inline constexpr ezStringView::ezStringView() = default;

inline ezStringView::ezStringView(char* pStart)
  : m_pStart(pStart)
  , m_pEnd(pStart + ezStringUtils::GetStringElementCount(pStart))
{
}

template <typename T>
constexpr inline ezStringView::ezStringView(T pStart, typename std::enable_if<std::is_same<T, const char*>::value, int>::type*)
  : m_pStart(pStart)
  , m_pEnd(pStart + ezStringUtils::GetStringElementCount(pStart))
{
}

template <typename T>
EZ_ALWAYS_INLINE ezStringView::ezStringView(const T&& str, typename std::enable_if<std::is_same<T, const char*>::value == false && std::is_convertible<T, const char*>::value, int>::type*)
{
  m_pStart = str;
  m_pEnd = m_pStart + ezStringUtils::GetStringElementCount(m_pStart);
}

inline ezStringView::ezStringView(const char* pStart, const char* pEnd)
{
  EZ_ASSERT_DEV(pStart <= pEnd, "It should start BEFORE it ends.");

  m_pStart = pStart;
  m_pEnd = pEnd;
}

constexpr inline ezStringView::ezStringView(const char* pStart, ezUInt32 uiLength)
  : m_pStart(pStart)
  , m_pEnd(pStart + uiLength)
{
}

template <size_t N>
inline ezStringView::ezStringView(const char (&str)[N])
  : m_pStart(str)
  , m_pEnd(str + N - 1)
{
  static_assert(N > 0, "Not a string literal");
  EZ_ASSERT_DEBUG(str[N - 1] == '\0', "Not a string literal. Manually cast to 'const char*' if you are trying to pass a const char fixed size array.");
}

template <size_t N>
inline ezStringView::ezStringView(char (&str)[N])
{
  m_pStart = str;
  m_pEnd = m_pStart + ezStringUtils::GetStringElementCount(str, str + N);
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

constexpr ezStringView operator"" _ezsv(const char* pString, size_t len)
{
  return ezStringView(pString, static_cast<ezUInt32>(len));
}