#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Rational.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Variant.h>

ezFormatString::ezFormatString(const ezStringBuilder& s)
{
  m_sString = s.GetView();
}

const char* ezFormatString::GetTextCStr(ezStringBuilder& out_sString) const
{
  out_sString = m_sString;
  return out_sString.GetData();
}

ezStringView ezFormatString::BuildFormattedText(ezStringBuilder& ref_sStorage, ezStringView* pArgs, ezUInt32 uiNumArgs) const
{
  ezStringView sString = m_sString;

  ezUInt32 uiLastParam = ezInvalidIndex;

  ref_sStorage.Clear();
  while (!sString.IsEmpty())
  {
    if (sString.StartsWith("%"))
    {
      if (sString.TrimWordStart("%%"))
      {
        ref_sStorage.Append("%"_ezsv);
      }
      else
      {
        EZ_ASSERT_DEBUG(false, "Single percentage signs are not allowed in ezFormatString. Did you forgot to migrate a printf-style "
                               "string? Use double percentage signs for the actual character.");
      }
    }
    else if (sString.GetElementCount() >= 3 && *sString.GetStartPointer() == '{' && *(sString.GetStartPointer() + 1) >= '0' && *(sString.GetStartPointer() + 1) <= '9' && *(sString.GetStartPointer() + 2) == '}')
    {
      uiLastParam = *(sString.GetStartPointer() + 1) - '0';
      EZ_ASSERT_DEV(uiLastParam < uiNumArgs, "Too many placeholders in format string");

      if (uiLastParam < uiNumArgs)
      {
        ref_sStorage.Append(pArgs[uiLastParam]);
      }

      sString.ChopAwayFirstCharacterAscii();
      sString.ChopAwayFirstCharacterAscii();
      sString.ChopAwayFirstCharacterAscii();
    }
    else if (sString.TrimWordStart("{}"))
    {
      ++uiLastParam;
      EZ_ASSERT_DEV(uiLastParam < uiNumArgs, "Too many placeholders in format string");

      if (uiLastParam < uiNumArgs)
      {
        ref_sStorage.Append(pArgs[uiLastParam]);
      }
    }
    else
    {
      const ezUInt32 character = sString.GetCharacter();
      ref_sStorage.Append(character);
      sString.ChopAwayFirstCharacterUtf8();
    }
  }

  return ref_sStorage.GetView();
}

//////////////////////////////////////////////////////////////////////////

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgI& arg)
{
  ezUInt32 writepos = 0;
  ezStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_uiBase);
  szTmp[writepos] = '\0';
  return ezStringView(szTmp, szTmp + writepos);
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, ezInt64 iArg)
{
  ezUInt32 writepos = 0;
  ezStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, iArg, 1, false, 10);
  szTmp[writepos] = '\0';
  return ezStringView(szTmp, szTmp + writepos);
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, ezInt32 iArg)
{
  return BuildString(szTmp, uiLength, (ezInt64)iArg);
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgU& arg)
{
  ezUInt32 writepos = 0;
  ezStringUtils::OutputFormattedUInt(szTmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_uiBase, arg.m_bUpperCase);
  szTmp[writepos] = '\0';
  return ezStringView(szTmp, szTmp + writepos);
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, ezUInt64 uiArg)
{
  ezUInt32 writepos = 0;
  ezStringUtils::OutputFormattedUInt(szTmp, uiLength, writepos, uiArg, 1, false, 10, false);
  szTmp[writepos] = '\0';
  return ezStringView(szTmp, szTmp + writepos);
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, ezUInt32 uiArg)
{
  return BuildString(szTmp, uiLength, (ezUInt64)uiArg);
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgF& arg)
{
  ezUInt32 writepos = 0;
  ezStringUtils::OutputFormattedFloat(szTmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_iPrecision, arg.m_bScientific);
  szTmp[writepos] = '\0';
  return ezStringView(szTmp, szTmp + writepos);
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, double fArg)
{
  ezUInt32 writepos = 0;
  ezStringUtils::OutputFormattedFloat(szTmp, uiLength, writepos, fArg, 1, false, -1, false);
  szTmp[writepos] = '\0';
  return ezStringView(szTmp, szTmp + writepos);
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, bool bArg)
{
  EZ_IGNORE_UNUSED(szTmp);
  EZ_IGNORE_UNUSED(uiLength);
  return bArg ? "true" : "false";
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const char* szArg)
{
  EZ_IGNORE_UNUSED(szTmp);
  EZ_IGNORE_UNUSED(uiLength);
  return szArg;
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const wchar_t* pArg)
{
  const char* start = szTmp;
  if (pArg != nullptr)
  {
    // Code points in UTF-8 can be up to 4 byte, so the end pointer is 3 byte "earlier" than for
    // for a single byte character. One byte for trailing zero is already accounted for in uiLength.
    const char* tmpEnd = szTmp + uiLength - 3u;
    while (*pArg != '\0' && szTmp < tmpEnd)
    {
      // decode utf8 to utf32
      const ezUInt32 uiUtf32 = ezUnicodeUtils::DecodeWCharToUtf32(pArg);

      // encode utf32 to wchar_t
      ezUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, szTmp);
    }
  }

  // Append terminator. As the extra byte for trailing zero is accounted for in uiLength, this is safe.
  *szTmp = '\0';

  return start;
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezString& sArg)
{
  EZ_IGNORE_UNUSED(szTmp);
  EZ_IGNORE_UNUSED(uiLength);
  return sArg.GetView();
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezHashedString& sArg)
{
  EZ_IGNORE_UNUSED(szTmp);
  EZ_IGNORE_UNUSED(uiLength);
  return sArg.GetView();
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezStringBuilder& sArg)
{
  EZ_IGNORE_UNUSED(szTmp);
  EZ_IGNORE_UNUSED(uiLength);
  return sArg.GetView();
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezUntrackedString& sArg)
{
  EZ_IGNORE_UNUSED(szTmp);
  EZ_IGNORE_UNUSED(uiLength);
  return sArg.GetView();
}

const ezStringView& BuildString(char* szTmp, ezUInt32 uiLength, const ezStringView& sArg)
{
  EZ_IGNORE_UNUSED(szTmp);
  EZ_IGNORE_UNUSED(uiLength);
  return sArg;
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgC& arg)
{
  EZ_IGNORE_UNUSED(uiLength);

  szTmp[0] = arg.m_Value;
  szTmp[1] = '\0';

  return ezStringView(&szTmp[0], &szTmp[1]);
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgP& arg)
{
  ezStringUtils::snprintf(szTmp, uiLength, "%p", arg.m_Value);
  return ezStringView(szTmp);
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, ezResult arg)
{
  EZ_IGNORE_UNUSED(szTmp);
  EZ_IGNORE_UNUSED(uiLength);

  if (arg.Failed())
    return "<failed>";
  else
    return "<succeeded>";
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezVariant& arg)
{
  ezString sString = arg.ConvertTo<ezString>();
  ezStringUtils::snprintf(szTmp, uiLength, "%s", sString.GetData());
  return ezStringView(szTmp);
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezAngle& arg)
{
  ezUInt32 writepos = 0;
  ezStringUtils::OutputFormattedFloat(szTmp, uiLength - 2, writepos, arg.GetDegree(), 1, false, 1, false);

  // Utf-8 representation of the degree sign
  szTmp[writepos + 0] = /*(char)0xC2;*/ -62;
  szTmp[writepos + 1] = /*(char)0xB0;*/ -80;
  szTmp[writepos + 2] = '\0';

  return ezStringView(szTmp, szTmp + writepos + 2);
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezRational& arg)
{
  ezUInt32 writepos = 0;

  if (arg.IsIntegral())
  {
    ezStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, arg.GetIntegralResult(), 1, false, 10);

    return ezStringView(szTmp, szTmp + writepos);
  }
  else
  {
    ezStringUtils::snprintf(szTmp, uiLength, "%i/%i", arg.GetNumerator(), arg.GetDenominator());

    return ezStringView(szTmp);
  }
}

ezStringView BuildString(char* pTmp, ezUInt32 uiLength, const ezTime& arg)
{
  ezUInt32 writepos = 0;

  const double fAbsSec = ezMath::Abs(arg.GetSeconds());

  if (fAbsSec < 0.000001)
  {
    ezStringUtils::OutputFormattedFloat(pTmp, uiLength - 5, writepos, arg.GetNanoseconds(), 1, false, 1, false, true);
    // szTmp[writepos++] = ' ';
    pTmp[writepos++] = 'n';
    pTmp[writepos++] = 's';
  }
  else if (fAbsSec < 0.001)
  {
    ezStringUtils::OutputFormattedFloat(pTmp, uiLength - 5, writepos, arg.GetMicroseconds(), 1, false, 1, false, true);

    // szTmp[writepos++] = ' ';
    // Utf-8 representation of the microsecond (us) sign
    pTmp[writepos++] = /*(char)0xC2;*/ -62;
    pTmp[writepos++] = /*(char)0xB5;*/ -75;
    pTmp[writepos++] = 's';
  }
  else if (fAbsSec < 1.0)
  {
    ezStringUtils::OutputFormattedFloat(pTmp, uiLength - 5, writepos, arg.GetMilliseconds(), 1, false, 1, false, true);

    // tmp[writepos++] = ' ';
    pTmp[writepos++] = 'm';
    pTmp[writepos++] = 's';
  }
  else if (fAbsSec < 60.0)
  {
    ezStringUtils::OutputFormattedFloat(pTmp, uiLength - 5, writepos, arg.GetSeconds(), 1, false, 1, false, true);

    // szTmp[writepos++] = ' ';
    pTmp[writepos++] = 's';
    pTmp[writepos++] = 'e';
    pTmp[writepos++] = 'c';
  }
  else if (fAbsSec < 60.0 * 60.0)
  {
    double tRem = fAbsSec;

    ezInt32 iMin = static_cast<ezInt32>(ezMath::Trunc(tRem / 60.0));
    tRem -= iMin * 60;
    iMin *= ezMath::Sign(static_cast<ezInt32>(arg.GetSeconds()));

    const ezInt32 iSec = static_cast<ezInt32>(ezMath::Trunc(tRem));

    writepos = ezStringUtils::snprintf(pTmp, uiLength, "%imin %isec", iMin, iSec);
  }
  else
  {
    double tRem = fAbsSec;

    ezInt32 iHrs = static_cast<ezInt32>(ezMath::Trunc(tRem / (60.0 * 60.0)));
    tRem -= iHrs * 60 * 60;
    iHrs *= ezMath::Sign(static_cast<ezInt32>(arg.GetSeconds()));

    const ezInt32 iMin = static_cast<ezInt32>(ezMath::Trunc(tRem / 60.0));
    tRem -= iMin * 60;

    const ezInt32 iSec = static_cast<ezInt32>(ezMath::Trunc(tRem));

    writepos = ezStringUtils::snprintf(pTmp, uiLength, "%ih %imin %isec", iHrs, iMin, iSec);
  }

  pTmp[writepos] = '\0';
  return ezStringView(pTmp, pTmp + writepos);
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgHumanReadable& arg)
{
  ezUInt32 suffixIndex = 0;
  ezUInt64 divider = 1;
  double absValue = ezMath::Abs(arg.m_Value);
  while (absValue / divider >= arg.m_Base && suffixIndex < arg.m_SuffixCount - 1)
  {
    divider *= arg.m_Base;
    ++suffixIndex;
  }

  ezUInt32 writepos = 0;
  if (divider == 1 && ezMath::Fraction(arg.m_Value) == 0.0)
  {
    ezStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, static_cast<ezInt64>(arg.m_Value), 1, false, 10);
  }
  else
  {
    ezStringUtils::OutputFormattedFloat(szTmp, uiLength, writepos, arg.m_Value / divider, 1, false, 2, false);
  }
  ezStringUtils::Copy(szTmp + writepos, uiLength - writepos, arg.m_Suffixes[suffixIndex]);

  return ezStringView(szTmp);
}

ezArgSensitive::BuildStringCallback ezArgSensitive::s_BuildStringCB = nullptr;

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgSensitive& arg)
{
  if (ezArgSensitive::s_BuildStringCB)
  {
    return ezArgSensitive::s_BuildStringCB(szTmp, uiLength, arg);
  }

  return arg.m_sSensitiveInfo;
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezSizeU32& arg)
{
  ezUInt32 writepos = 0;
  ezStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, arg.width, 1, false, 10);
  szTmp[writepos++] = 'x';
  ezStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, arg.height, 1, false, 10);
  szTmp[writepos] = '\0';
  return ezStringView(szTmp, szTmp + writepos);
}


ezStringView ezArgSensitive::BuildString_SensitiveUserData_Hash(char* szTmp, ezUInt32 uiLength, const ezArgSensitive& arg)
{
  const ezUInt32 len = arg.m_sSensitiveInfo.GetElementCount();

  if (len == 0)
    return ezStringView();

  if (!ezStringUtils::IsNullOrEmpty(arg.m_szContext))
  {
    ezStringUtils::snprintf(
      szTmp, uiLength, "sud:%s#%08x($%u)", arg.m_szContext, ezHashingUtils::xxHash32(arg.m_sSensitiveInfo.GetStartPointer(), len), len);
  }
  else
  {
    ezStringUtils::snprintf(szTmp, uiLength, "sud:#%08x($%u)", ezHashingUtils::xxHash32(arg.m_sSensitiveInfo.GetStartPointer(), len), len);
  }

  return szTmp;
}
