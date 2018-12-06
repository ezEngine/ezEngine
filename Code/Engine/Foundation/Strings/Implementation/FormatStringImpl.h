#pragma once

#include <array>
#include <tuple>
#include <utility>

template <typename... ARGS>
class ezFormatStringImpl : public ezFormatString
{
public:
  ezFormatStringImpl(const char* szFormat, ARGS&&... args)
      : m_Arguments(std::forward<ARGS>(args)...)
  {
    m_szString = szFormat;
  }

  /// \brief Generates the formatted text. Make sure to only call this function once and only when the formatted string is really needed.
  ///
  /// Requires an ezStringBuilder as storage, ie. writes the formatted text into it. Additionally it returns a const char* to that
  /// string builder data for convenience.
  virtual const char* GetText(ezStringBuilder& sb) const override
  {
    if (ezStringUtils::IsNullOrEmpty(m_szString))
    {
      return "";
    }

    ezStringView param[10];

    char tmp[10][64];
    ReplaceString<0>(tmp, param);

    const char* szString = m_szString;

    int iLastParam = -1;

    SBClear(sb);
    while (*szString != '\0')
    {
      if (*szString == '%')
      {
        if (*(szString + 1) == '%')
        {
          SBAppendView(sb, "%");
        }
        else
        {
          EZ_ASSERT_DEBUG(false, "Single percentage signs are not allowed in ezFormatString. Did you forgot to migrate a printf-style "
                                 "string? Use double percentage signs for the actual character.");
        }

        szString += 2;
      }
      else if (*szString == '{' && *(szString + 1) >= '0' && *(szString + 1) <= '9' && *(szString + 2) == '}')
      {
        iLastParam = *(szString + 1) - '0';
        SBAppendView(sb, param[iLastParam]);

        szString += 3;
      }
      else if (*szString == '{' && *(szString + 1) == '}')
      {
        ++iLastParam;
        EZ_ASSERT_DEV(iLastParam < 10, "Too many placeholders in format string");

        if (iLastParam < 10)
        {
          SBAppendView(sb, param[iLastParam]);
        }

        szString += 2;
      }
      else
      {
        const ezUInt32 character = ezUnicodeUtils::DecodeUtf8ToUtf32(szString);
        SBAppendChar(sb, character);
      }
    }

    return SBReturn(sb);
  }

private:
  template <ezInt32 N>
  typename std::enable_if<sizeof...(ARGS) != N>::type ReplaceString(char tmp[10][64], ezStringView* pViews) const
  {
    EZ_CHECK_AT_COMPILETIME_MSG(N < 10, "Maximum number of format arguments reached");

    // using a free function allows to overload with various different argument types
    pViews[N] = BuildString(tmp[N], 63, std::get<N>(m_Arguments));

    // Recurse, chip off one argument
    ReplaceString<N + 1>(tmp, pViews);
  }

  // Recursion end if we reached the number of arguments.
  template <ezInt32 N>
  typename std::enable_if<sizeof...(ARGS) == N>::type ReplaceString(char tmp[10][64], ezStringView* pViews) const
  {
  }


  // stores the arguments
  std::tuple<ARGS...> m_Arguments;
};
