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

    constexpr auto Size = sizeof...(ARGS);
    InvokeReplace(sb, std::make_index_sequence<Size>{});

    return sb;
  }

private:

  // helper function to expand the tuple to variadic template arguments
  template<std::size_t... index>
  void InvokeReplace(ezStringBuilder& sb, std::index_sequence<index...>) const
  {
    char tmp[256];
    ReplaceString<0, ARGS...>(tmp, sb, std::get<index>(m_Arguments)...);
  }

  // Version without parameters to end the recursion
  template<ezInt32 N, typename... ARGN>
  void ReplaceString(char* tmp, ezStringBuilder& sb) const {}

  template<ezInt32 N, typename ARG0, typename... ARGN>
  void ReplaceString(char* tmp, ezStringBuilder& sb, ARG0 arg0, ARGN... args) const
  {
    EZ_CHECK_AT_COMPILETIME_MSG(N < 10, "Maximum number of format arguments reached");

    // using a free function allows to overload with various different argument types
    const ezStringView res = BuildString(tmp, 255, std::get<N>(m_Arguments));

    // replace all occurrances of {N} with the formatted argument
    switch (N)
    {
    case 0: sb.ReplaceAll("{0}", res); break;
    case 1: sb.ReplaceAll("{1}", res); break;
    case 2: sb.ReplaceAll("{2}", res); break;
    case 3: sb.ReplaceAll("{3}", res); break;
    case 4: sb.ReplaceAll("{4}", res); break;
    case 5: sb.ReplaceAll("{5}", res); break;
    case 6: sb.ReplaceAll("{6}", res); break;
    case 7: sb.ReplaceAll("{7}", res); break;
    case 8: sb.ReplaceAll("{8}", res); break;
    case 9: sb.ReplaceAll("{9}", res); break;
    }

    // Recurse, chip off one argument
    ReplaceString<N + 1, ARGN...>(tmp, sb, args...);
  }

  // stores the arguments
  std::tuple<ARGS...> m_Arguments;
};

#endif
