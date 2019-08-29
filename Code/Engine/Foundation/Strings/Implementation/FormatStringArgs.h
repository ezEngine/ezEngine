#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/StringView.h>

class ezStringBuilder;
class ezVariant;
class ezAngle;
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
  inline ezArgHumanReadable(const double value, const ezUInt64 base, const char* const* const suffixes, ezUInt32 suffixCount)
    : m_Value(value)
    , m_Base(base)
    , m_Suffixes(suffixes)
    , m_SuffixCount(suffixCount)
  {
  }

  inline ezArgHumanReadable(const ezInt64 value, const ezUInt64 base, const char* const* const suffixes, ezUInt32 suffixCount)
    : ezArgHumanReadable(static_cast<double>(value), base, suffixes, suffixCount)
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
  const char* const * const m_Suffixes;
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
struct ezArgErrorCode
{
  inline explicit ezArgErrorCode(ezUInt32 errorCode)
    : m_ErrorCode(errorCode)
  {
  }

  ezUInt32 m_ErrorCode;
};
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgErrorCode& arg);
#endif

EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgI& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, ezInt64 arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, ezInt32 arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgU& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, ezUInt64 arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, ezUInt32 arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgF& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, double arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, bool arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const char* arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezStringBuilder& arg);
EZ_FOUNDATION_DLL const ezStringView& BuildString(char* tmp, ezUInt32 uiLength, const ezStringView& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgC& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgP& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, ezResult arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezVariant& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezAngle& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgHumanReadable& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezTime& arg);


