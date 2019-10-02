#pragma once

#include <Foundation/Strings/Implementation/FormatStringArgs.h>

class ezStringBuilder;
struct ezStringView;

class EZ_FOUNDATION_DLL ezFormatString
{
public:
  EZ_ALWAYS_INLINE ezFormatString() { m_szString = nullptr; }
  EZ_ALWAYS_INLINE ezFormatString(const char* szString) { m_szString = szString; }
  ezFormatString(const ezStringBuilder& s);
  virtual ~ezFormatString() {}

  /// \brief Generates the formatted text. Make sure to only call this function once and only when the formatted string is really needed.
  ///
  /// Requires an ezStringBuilder as storage, ie. POTENTIALLY writes the formatted text into it.
  /// However, if no formatting is required, it may not touch the string builder at all and just return a string directly.
  ///
  /// \note Do not assume that the result is stored in \a sb. Always only use the return value. The string builder is only used
  /// when necessary.
  virtual const char* GetText(ezStringBuilder& sb) const { return m_szString; }

protected:
  // out of line function so that we don't need to include ezStringBuilder here, to break include dependency cycle
  static void SBAppendView(ezStringBuilder& sb, const ezStringView& sub);
  static void SBClear(ezStringBuilder& sb);
  static void SBAppendChar(ezStringBuilder& sb, ezUInt32 uiChar);
  static const char* SBReturn(ezStringBuilder& sb);

  const char* m_szString;
};

#include <Foundation/Strings/Implementation/FormatStringImpl.h>

template <typename... ARGS>
EZ_ALWAYS_INLINE ezFormatStringImpl<ARGS...> ezFmt(const char* szFormat, ARGS&&... args)
{
  return ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...);
}

