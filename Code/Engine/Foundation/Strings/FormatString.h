#pragma once

#include <Foundation/Strings/Implementation/FormatStringArgs.h>

class ezStringBuilder;
struct ezStringView;

/// \brief Implements formating of strings with placeholders and formatting options.
///
/// ezFormatString can be used anywhere where a string should be formatable when passing it into a function.
/// Good examples are ezStringBuilder::Format() or ezLog::Info().
///
/// A function taking an ezFormatString can internally call ezFormatString::GetText() to retrieve he formatted result.
/// When calling such a function, one must wrap the parameter into 'ezFmt' to enable formatting options, example:
///   void MyFunc(const ezFormatString& text);
///   MyFunc(ezFmt("Cool Story {}", "Bro"));
///
/// To provide more convenience, one can add a template-function overload like this:
///   template <typename... ARGS>
///   void MyFunc(const char* szFormat, ARGS&&... args)
///   {
///     MyFunc(ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
///   }
///
/// This allows to call MyFunc() without the 'ezFmt' wrapper.
///
/// 
/// === Formatting ===
///
/// Placeholders for variables are specified using '{}'. These may use numbers from 0 to 9,
/// ie. {0}, {3}, {2}, etc. which allows to change the order or insert duplicates.
/// If no number is provided, each {} instance represents the next argument.
///
/// To specify special formatting, wrap the argument into an ezArgXY call:
///   ezArgC - for characters
///   ezArgI - for integer formatting
///   ezArgU - for unsigned integer formatting (e.g. HEX)
///   ezArgF - for floating point formatting
///   ezArgP - for pointer formatting
///   ezArgDateTime - for ezDateTime formatting options
///   ezArgErrorCode - for Windows error code formatting
///   ezArgHumanReadable - for shortening numbers with common abbreviations
///   ezArgFileSize - for representing file sizes
///
/// Example:
///   ezStringBuilder::Format("HEX: {}", ezArgU(1337, 8 /*width*/, true /*pad with zeros*/, 16 /*base16*/, true/*upper case*/));
///
/// Arbitrary other types can support special formatting even without an ezArgXY call. E.g. ezTime and ezAngle do special formatting.
/// ezArgXY calls are only necessary if formatting options are needed for a specific formatting should be enforced (e.g. ezArgErrorCode
/// would otherwise just use uint32 formatting).
///
/// To implement custom formatting see the various free standing 'BuildString' functions.
class EZ_FOUNDATION_DLL ezFormatString
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezFormatString); // pass by reference, never pass by value

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
  [[nodiscard]] virtual const char* GetText(ezStringBuilder& sb) const { return m_szString; }

  bool IsEmpty() const { return ezStringUtils::IsNullOrEmpty(m_szString); }

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

