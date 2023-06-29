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

EZ_ALWAYS_INLINE bool ezStringView::IsEmpty() const
{
  return m_pStart == m_pEnd || ezStringUtils::IsNullOrEmpty(m_pStart);
}

EZ_ALWAYS_INLINE bool ezStringView::IsEqual(ezStringView sOther) const
{
  return ezStringUtils::IsEqual(m_pStart, sOther.GetStartPointer(), m_pEnd, sOther.GetEndPointer());
}

EZ_ALWAYS_INLINE bool ezStringView::IsEqual_NoCase(ezStringView sOther) const
{
  return ezStringUtils::IsEqual_NoCase(m_pStart, sOther.GetStartPointer(), m_pEnd, sOther.GetEndPointer());
}

EZ_ALWAYS_INLINE bool ezStringView::StartsWith(ezStringView sStartsWith) const
{
  return ezStringUtils::StartsWith(m_pStart, sStartsWith.GetStartPointer(), m_pEnd, sStartsWith.GetEndPointer());
}

EZ_ALWAYS_INLINE bool ezStringView::StartsWith_NoCase(ezStringView sStartsWith) const
{
  return ezStringUtils::StartsWith_NoCase(m_pStart, sStartsWith.GetStartPointer(), m_pEnd, sStartsWith.GetEndPointer());
}

EZ_ALWAYS_INLINE bool ezStringView::EndsWith(ezStringView sEndsWith) const
{
  return ezStringUtils::EndsWith(m_pStart, sEndsWith.GetStartPointer(), m_pEnd, sEndsWith.GetEndPointer());
}

EZ_ALWAYS_INLINE bool ezStringView::EndsWith_NoCase(ezStringView sEndsWith) const
{
  return ezStringUtils::EndsWith_NoCase(m_pStart, sEndsWith.GetStartPointer(), m_pEnd, sEndsWith.GetEndPointer());
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

constexpr EZ_ALWAYS_INLINE ezStringView operator"" _ezsv(const char* pString, size_t uiLen)
{
  return ezStringView(pString, static_cast<ezUInt32>(uiLen));
}

template <typename Container>
void ezStringView::Split(bool bReturnEmptyStrings, Container& ref_output, const char* szSeparator1, const char* szSeparator2 /*= nullptr*/, const char* szSeparator3 /*= nullptr*/, const char* szSeparator4 /*= nullptr*/, const char* szSeparator5 /*= nullptr*/, const char* szSeparator6 /*= nullptr*/) const
{
  ref_output.Clear();

  if (IsEmpty())
    return;

  const ezUInt32 uiParams = 6;

  const ezStringView seps[uiParams] = {szSeparator1, szSeparator2, szSeparator3, szSeparator4, szSeparator5, szSeparator6};

  const char* szReadPos = GetStartPointer();

  while (true)
  {
    const char* szFoundPos = ezUnicodeUtils::GetMaxStringEnd<char>();
    ezUInt32 uiFoundSeparator = 0;

    for (ezUInt32 i = 0; i < uiParams; ++i)
    {
      const char* szFound = ezStringUtils::FindSubString(szReadPos, seps[i].GetStartPointer(), GetEndPointer(), seps[i].GetEndPointer());

      if ((szFound != nullptr) && (szFound < szFoundPos))
      {
        szFoundPos = szFound;
        uiFoundSeparator = i;
      }
    }

    // nothing found
    if (szFoundPos == ezUnicodeUtils::GetMaxStringEnd<char>())
    {
      const ezUInt32 uiLen = ezStringUtils::GetStringElementCount(szReadPos, GetEndPointer());

      if (bReturnEmptyStrings || (uiLen > 0))
        ref_output.PushBack(ezStringView(szReadPos, szReadPos + uiLen));

      return;
    }

    if (bReturnEmptyStrings || (szFoundPos > szReadPos))
      ref_output.PushBack(ezStringView(szReadPos, szFoundPos));

    szReadPos = szFoundPos + seps[uiFoundSeparator].GetElementCount();
  }
}

EZ_ALWAYS_INLINE bool operator==(ezStringView lhs, ezStringView rhs)
{
  return lhs.IsEqual(rhs);
}

EZ_ALWAYS_INLINE bool operator!=(ezStringView lhs, ezStringView rhs)
{
  return !lhs.IsEqual(rhs);
}

EZ_ALWAYS_INLINE bool operator<(ezStringView lhs, ezStringView rhs)
{
  return lhs.Compare(rhs) < 0;
}

EZ_ALWAYS_INLINE bool operator<=(ezStringView lhs, ezStringView rhs)
{
  return lhs.Compare(rhs) <= 0;
}

EZ_ALWAYS_INLINE bool operator>(ezStringView lhs, ezStringView rhs)
{
  return lhs.Compare(rhs) > 0;
}

EZ_ALWAYS_INLINE bool operator>=(ezStringView lhs, ezStringView rhs)
{
  return lhs.Compare(rhs) >= 0;
}
