#pragma once

#ifndef EZ_INCLUDING_BASICS_H
#  error "Please don't include FormatStringArgs.h directly, but instead include Foundation/Basics.h"
#endif

class ezStringBuilder;
class ezVariant;
class ezAngle;
class ezRational;
struct ezTime;

struct ezArgI
{
  inline explicit ezArgI(ezInt64 value, ezUInt8 uiWidth = 1, bool bPadWithZeros = false, ezUInt8 uiBase = 10)
    : m_Value(value)
    , m_uiWidth(uiWidth)
    , m_bPadWithZeros(bPadWithZeros)
    , m_uiBase(uiBase)
  {
  }

  ezInt64 m_Value;
  ezUInt8 m_uiWidth;
  bool m_bPadWithZeros;
  ezUInt8 m_uiBase;
};

struct ezArgU
{
  inline explicit ezArgU(ezUInt64 value, ezUInt8 uiWidth = 1, bool bPadWithZeros = false, ezUInt8 uiBase = 10, bool bUpperCase = false)
    : m_Value(value)
    , m_uiWidth(uiWidth)
    , m_bPadWithZeros(bPadWithZeros)
    , m_bUpperCase(bUpperCase)
    , m_uiBase(uiBase)
  {
  }

  ezUInt64 m_Value;
  ezUInt8 m_uiWidth;
  bool m_bPadWithZeros;
  bool m_bUpperCase;
  ezUInt8 m_uiBase;
};

struct ezArgF
{
  inline explicit ezArgF(double value, ezInt8 iPrecision = -1, bool bScientific = false, ezUInt8 uiWidth = 1, bool bPadWithZeros = false)
    : m_Value(value)
    , m_uiWidth(uiWidth)
    , m_bPadWithZeros(bPadWithZeros)
    , m_bScientific(bScientific)
    , m_iPrecision(iPrecision)
  {
  }

  double m_Value;
  ezUInt8 m_uiWidth;
  bool m_bPadWithZeros;
  bool m_bScientific;
  ezInt8 m_iPrecision;
};

struct ezArgC
{
  inline explicit ezArgC(char value)
    : m_Value(value)
  {
  }

  char m_Value;
};

struct ezArgP
{
  inline explicit ezArgP(const void* value)
    : m_Value(value)
  {
  }

  const void* m_Value;
};


/// \brief Formats a given number such that it will be in format [0, base){suffix} with suffix
/// representing a power of base. Resulting numbers are output with a precision of 2 fractional digits
/// and fractional digits are subject to rounding, so numbers at the upper boundary of [0, base)
/// may be rounded up to the next power of base.
///
/// E.g.: For the default case base is 1000 and suffixes are the SI unit suffixes (i.e. K for kilo, M for mega etc.)
///       Thus 0 remains 0, 1 remains 1, 1000 becomes 1.00K, and 2534000 becomes 2.53M. But 999.999 will
///       end up being displayed as 1000.00K for base 1000 due to rounding.
struct ezArgHumanReadable
{
  inline ezArgHumanReadable(const double value, const ezUInt64 uiBase, const char* const* const pSuffixes, ezUInt32 uiSuffixCount)
    : m_Value(value)
    , m_Base(uiBase)
    , m_Suffixes(pSuffixes)
    , m_SuffixCount(uiSuffixCount)
  {
  }

  inline ezArgHumanReadable(const ezInt64 value, const ezUInt64 uiBase, const char* const* const pSuffixes, ezUInt32 uiSuffixCount)
    : ezArgHumanReadable(static_cast<double>(value), uiBase, pSuffixes, uiSuffixCount)
  {
  }

  inline explicit ezArgHumanReadable(const double value)
    : ezArgHumanReadable(value, 1000u, m_DefaultSuffixes, EZ_ARRAY_SIZE(m_DefaultSuffixes))
  {
  }

  inline explicit ezArgHumanReadable(const ezInt64 value)
    : ezArgHumanReadable(static_cast<double>(value), 1000u, m_DefaultSuffixes, EZ_ARRAY_SIZE(m_DefaultSuffixes))
  {
  }

  const double m_Value;
  const ezUInt64 m_Base;
  const char* const* const m_Suffixes;
  const char* const m_DefaultSuffixes[6] = {"", "K", "M", "G", "T", "P"};
  const ezUInt32 m_SuffixCount;
};

struct ezArgFileSize : public ezArgHumanReadable
{
  inline explicit ezArgFileSize(const ezUInt64 value)
    : ezArgHumanReadable(static_cast<double>(value), 1024u, m_ByteSuffixes, EZ_ARRAY_SIZE(m_ByteSuffixes))
  {
  }

  const char* const m_ByteSuffixes[6] = {"B", "KB", "MB", "GB", "TB", "PB"};
};

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
/// \brief Converts a windows HRESULT into an error code and a human-readable error message.
/// Pass in `GetLastError()` function or an HRESULT from another error source. Be careful when printing multiple values, a function could clear `GetLastError` as a side-effect so it is best to store it in a temp variable before printing a complex error message.
/// \sa https://learn.microsoft.com/en-gb/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
struct ezArgErrorCode
{
  inline explicit ezArgErrorCode(ezUInt32 uiErrorCode)
    : m_ErrorCode(uiErrorCode)
  {
  }

  ezUInt32 m_ErrorCode;
};
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgErrorCode& arg);

#endif

#if EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)
/// \brief Many Linux APIs will fill out error on failure. This converts the error into an error code and a human-readable error message.
/// Pass in the linux `errno` symbol. Be careful when printing multiple values, a function could clear `errno` as a side-effect so it is best to store it in a temp variable before printing a complex error message.
/// You may have to include #include <errno.h> use this.
/// \sa https://man7.org/linux/man-pages/man3/errno.3.html
struct ezArgErrno
{
  inline explicit ezArgErrno(ezInt32 iErrno)
    : m_iErrno(iErrno)
  {
  }

  ezInt32 m_iErrno;
};
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgErrno& arg);
#endif

/// \brief Wraps a string that may contain sensitive information, such as user file paths.
///
/// The application can specify a function to scramble this type of information. By default no such function is set.
/// A general purpose function is provided with 'BuildString_SensitiveUserData_Hash()'
///
/// \param sSensitiveInfo The information that may need to be scrambled.
/// \param szContext A custom string to identify the 'context', ie. what type of sensitive data is being scrambled.
///        This may be passed through unmodified, or can guide the scrambling function to choose how to output the sensitive data.
struct ezArgSensitive
{
  inline explicit ezArgSensitive(const ezStringView& sSensitiveInfo, const char* szContext = nullptr)
    : m_sSensitiveInfo(sSensitiveInfo)
    , m_szContext(szContext)
  {
  }

  const ezStringView m_sSensitiveInfo;
  const char* m_szContext;

  using BuildStringCallback = ezStringView (*)(char*, ezUInt32, const ezArgSensitive&);
  EZ_FOUNDATION_DLL static BuildStringCallback s_BuildStringCB;

  /// \brief Set s_BuildStringCB to this function to enable scrambling of sensitive data.
  EZ_FOUNDATION_DLL static ezStringView BuildString_SensitiveUserData_Hash(char* szTmp, ezUInt32 uiLength, const ezArgSensitive& arg);
};

EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgI& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, ezInt64 iArg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, ezInt32 iArg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgU& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, ezUInt64 uiArg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, ezUInt32 uiArg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgF& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, double fArg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, bool bArg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const char* szArg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const wchar_t* pArg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezStringBuilder& sArg);
EZ_FOUNDATION_DLL const ezStringView& BuildString(char* szTmp, ezUInt32 uiLength, const ezStringView& sArg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgC& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgP& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, ezResult arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezVariant& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezAngle& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezRational& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgHumanReadable& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezTime& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgSensitive& arg);


#if EZ_ENABLED(EZ_COMPILER_GCC) || EZ_ENABLED(EZ_COMPILER_CLANG)

// on these platforms "long int" is a different type from "long long int"

EZ_ALWAYS_INLINE ezStringView BuildString(char* szTmp, ezUInt32 uiLength, long int iArg)
{
  return BuildString(szTmp, uiLength, static_cast<ezInt64>(iArg));
}

EZ_ALWAYS_INLINE ezStringView BuildString(char* szTmp, ezUInt32 uiLength, unsigned long int uiArg)
{
  return BuildString(szTmp, uiLength, static_cast<ezUInt64>(uiArg));
}

#endif
