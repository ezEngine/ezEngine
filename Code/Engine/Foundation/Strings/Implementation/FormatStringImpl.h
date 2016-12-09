#pragma once

#if (__cplusplus >= 201402L || _MSC_VER >= 1900)

#include <tuple>
#include <utility>
#include <array>

template<typename ... ARGS>
class ezFormatStringImpl : public ezFormatString
{
public:
  ezFormatStringImpl(const char* szFormat, ARGS... args)
    : m_Arguments(args...)
  {
    m_szString = szFormat;
  }

  /// \brief Generates the formatted text. Make sure to only call this function once and only when the formatted string is really needed.
  ///
  /// Requires an ezStringBuilder as storage, ie. writes the formatted text into it. Additionally it returns a const char* to that
  /// string builder data for convenience.
  virtual const char* GetText(ezStringBuilder& sb) const override
  {
    sb = m_szString;

    char tmp[256];
    ReplaceString<0>(tmp, sb);

    return sb;
  }

private:

  template<ezInt32 N>
  typename std::enable_if<sizeof...(ARGS) != N>::type ReplaceString(char* tmp, ezStringBuilder& sb) const
  {
    EZ_CHECK_AT_COMPILETIME_MSG(N < 10, "Maximum number of format arguments reached");

    // using a free function allows to overload with various different argument types
    const ezStringView res = BuildString(tmp, 255, std::get<N>(m_Arguments));

    // replace all occurrances of {N} with the formatted argument
    switch (N)
    {
    case 0: ReplaceAll(sb, "{0}", res); break;
    case 1: ReplaceAll(sb, "{1}", res); break;
    case 2: ReplaceAll(sb, "{2}", res); break;
    case 3: ReplaceAll(sb, "{3}", res); break;
    case 4: ReplaceAll(sb, "{4}", res); break;
    case 5: ReplaceAll(sb, "{5}", res); break;
    case 6: ReplaceAll(sb, "{6}", res); break;
    case 7: ReplaceAll(sb, "{7}", res); break;
    case 8: ReplaceAll(sb, "{8}", res); break;
    case 9: ReplaceAll(sb, "{9}", res); break;
    }

    // Recurse, chip off one argument
    ReplaceString<N + 1>(tmp, sb);
  }

  // Recursion end if we reached the number of arguments.
  template<ezInt32 N>
  typename std::enable_if<sizeof...(ARGS) == N>::type ReplaceString(char* tmp, ezStringBuilder& sb) const
  {
  }


  // stores the arguments
  std::tuple<ARGS...> m_Arguments;
};

#endif
