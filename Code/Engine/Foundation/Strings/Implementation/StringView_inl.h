#pragma once

EZ_ALWAYS_INLINE constexpr ezStringView::ezStringView() = default;

EZ_ALWAYS_INLINE ezStringView::ezStringView(char* pStart)
  : m_pStart(pStart)
  , m_pEnd(pStart + ezStringUtils::GetStringElementCount(pStart))
{
}

template <typename T>
constexpr EZ_ALWAYS_INLINE ezStringView::ezStringView(T pStart, typename std::enable_if<std::is_same<T, const char*>::value, int>::type*)
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

EZ_ALWAYS_INLINE ezStringView::ezStringView(const char* pStart, const char* pEnd)
{
  EZ_ASSERT_DEV(pStart <= pEnd, "It should start BEFORE it ends.");

  m_pStart = pStart;
  m_pEnd = pEnd;
}

constexpr EZ_ALWAYS_INLINE ezStringView::ezStringView(const char* pStart, ezUInt32 uiLength)
  : m_pStart(pStart)
  , m_pEnd(pStart + uiLength)
{
}

template <size_t N>
EZ_ALWAYS_INLINE ezStringView::ezStringView(const char (&str)[N])
  : m_pStart(str)
  , m_pEnd(str + N - 1)
{
  static_assert(N > 0, "Not a string literal");
  EZ_ASSERT_DEBUG(str[N - 1] == '\0', "Not a string literal. Manually cast to 'const char*' if you are trying to pass a const char fixed size array.");
}

template <size_t N>
EZ_ALWAYS_INLINE ezStringView::ezStringView(char (&str)[N])
{
  m_pStart = str;
  m_pEnd = m_pStart + ezStringUtils::GetStringElementCount(str, str + N);
}

inline void ezStringView::operator++()
{
  if (!IsValid())
    return;

  ezUnicodeUtils::MoveToNextUtf8(m_pStart, m_pEnd);
}

inline void ezStringView::operator+=(ezUInt32 d)
{
  ezUnicodeUtils::MoveToNextUtf8(m_pStart, m_pEnd, d);
}
EZ_ALWAYS_INLINE bool ezStringView::IsValid() const
{
  return (m_pStart != nullptr) && (m_pStart < m_pEnd);
}

EZ_ALWAYS_INLINE void ezStringView::SetStartPosition(const char* szCurPos)
{
  EZ_ASSERT_DEV((szCurPos >= m_pStart) && (szCurPos <= m_pEnd), "New start position must still be inside the view's range.");

  m_pStart = szCurPos;
}

EZ_ALWAYS_INLINE bool ezStringView::IsEqual(const ezStringView& sOther) const
{
  return ezStringUtils::IsEqualN(m_pStart, sOther.m_pStart, static_cast<ezUInt32>(-1), m_pEnd, sOther.m_pEnd);
}

EZ_ALWAYS_INLINE bool ezStringView::IsEqual_NoCase(const ezStringView& sOther) const
{
  return ezStringUtils::IsEqualN_NoCase(m_pStart, sOther.m_pStart, static_cast<ezUInt32>(-1), m_pEnd, sOther.m_pEnd);
}

EZ_ALWAYS_INLINE void ezStringView::Trim(const char* szTrimChars)
{
  return Trim(szTrimChars, szTrimChars);
}

EZ_ALWAYS_INLINE void ezStringView::Trim(const char* szTrimCharsStart, const char* szTrimCharsEnd)
{
  if (IsValid())
  {
    ezStringUtils::Trim(m_pStart, m_pEnd, szTrimCharsStart, szTrimCharsEnd);
  }
}

constexpr EZ_ALWAYS_INLINE ezStringView operator"" _ezsv(const char* pString, size_t len)
{
  return ezStringView(pString, static_cast<ezUInt32>(len));
}

template <typename Container>
void ezStringView::Split(bool bReturnEmptyStrings, Container& Output, const char* szSeparator1, const char* szSeparator2 /*= nullptr*/, const char* szSeparator3 /*= nullptr*/, const char* szSeparator4 /*= nullptr*/, const char* szSeparator5 /*= nullptr*/, const char* szSeparator6 /*= nullptr*/) const
{
  Output.Clear();

  if (IsEmpty())
    return;

  const ezUInt32 uiParams = 6;

  const char* Seps[uiParams] = {szSeparator1, szSeparator2, szSeparator3, szSeparator4, szSeparator5, szSeparator6};

  ezUInt32 SepLen[uiParams];

  for (ezInt32 i = 0; i < uiParams; ++i)
    SepLen[i] = ezStringUtils::GetStringElementCount(Seps[i]);

  const char* szReadPos = GetStartPointer();

  while (true)
  {
    const char* szFoundPos = ezUnicodeUtils::GetMaxStringEnd<char>();
    ezInt32 iFoundSeparator = 0;

    for (ezInt32 i = 0; i < uiParams; ++i)
    {
      const char* szFound = ezStringUtils::FindSubString(szReadPos, Seps[i], GetEndPointer());

      if ((szFound != nullptr) && (szFound < szFoundPos))
      {
        szFoundPos = szFound;
        iFoundSeparator = i;
      }
    }

    // nothing found
    if (szFoundPos == ezUnicodeUtils::GetMaxStringEnd<char>())
    {
      const ezUInt32 uiLen = ezStringUtils::GetStringElementCount(szReadPos, GetEndPointer());

      if (bReturnEmptyStrings || (uiLen > 0))
        Output.PushBack(ezStringView(szReadPos, szReadPos + uiLen));

      return;
    }

    if (bReturnEmptyStrings || (szFoundPos > szReadPos))
      Output.PushBack(ezStringView(szReadPos, szFoundPos));

    szReadPos = szFoundPos + SepLen[iFoundSeparator];
  }
}
