#include <FoundationPCH.h>

#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Variant.h>

ezFormatString::ezFormatString(const ezStringBuilder& s)
{
  m_szString = s.GetData();
}

void ezFormatString::SBAppendView(ezStringBuilder& sb, const ezStringView& sub)
{
  sb.Append(sub);
}

void ezFormatString::SBClear(ezStringBuilder& sb)
{
  sb.Clear();
}

void ezFormatString::SBAppendChar(ezStringBuilder& sb, ezUInt32 uiChar)
{
  sb.Append(uiChar);
}

const char* ezFormatString::SBReturn(ezStringBuilder& sb)
{
  return sb.GetData();
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgI& arg)
{
  ezUInt32 writepos = 0;
  ezStringUtils::OutputFormattedInt(tmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_uiBase);
  tmp[writepos] = '\0';
  return ezStringView(tmp, tmp + writepos);
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, ezInt64 arg)
{
  ezUInt32 writepos = 0;
  ezStringUtils::OutputFormattedInt(tmp, uiLength, writepos, arg, 1, false, 10);
  tmp[writepos] = '\0';
  return ezStringView(tmp, tmp + writepos);
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, ezInt32 arg)
{
  return BuildString(tmp, uiLength, (ezInt64)arg);
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgU& arg)
{
  ezUInt32 writepos = 0;
  ezStringUtils::OutputFormattedUInt(
    tmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_uiBase, arg.m_bUpperCase);
  tmp[writepos] = '\0';
  return ezStringView(tmp, tmp + writepos);
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, ezUInt64 arg)
{
  ezUInt32 writepos = 0;
  ezStringUtils::OutputFormattedUInt(tmp, uiLength, writepos, arg, 1, false, 10, false);
  tmp[writepos] = '\0';
  return ezStringView(tmp, tmp + writepos);
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, ezUInt32 arg)
{
  return BuildString(tmp, uiLength, (ezUInt64)arg);
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgF& arg)
{
  ezUInt32 writepos = 0;
  ezStringUtils::OutputFormattedFloat(
    tmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_iPrecision, arg.m_bScientific);
  tmp[writepos] = '\0';
  return ezStringView(tmp, tmp + writepos);
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, double arg)
{
  ezUInt32 writepos = 0;
  ezStringUtils::OutputFormattedFloat(tmp, uiLength, writepos, arg, 1, false, -1, false);
  tmp[writepos] = '\0';
  return ezStringView(tmp, tmp + writepos);
}


ezStringView BuildString(char* tmp, ezUInt32 uiLength, const char* arg)
{
  return arg;
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezString& arg)
{
  return ezStringView(arg.GetData(), arg.GetData() + arg.GetElementCount());
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezHashedString& arg)
{
  return ezStringView(arg.GetData(), arg.GetData() + arg.GetString().GetElementCount());
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezStringBuilder& arg)
{
  return ezStringView(arg.GetData(), arg.GetData() + arg.GetElementCount());
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezUntrackedString& arg)
{
  return ezStringView(arg.GetData(), arg.GetData() + arg.GetElementCount());
}

const ezStringView& BuildString(char* tmp, ezUInt32 uiLength, const ezStringView& arg)
{
  return arg;
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgC& arg)
{
  tmp[0] = arg.m_Value;
  tmp[1] = '\0';

  return ezStringView(&tmp[0], &tmp[1]);
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgP& arg)
{
  ezStringUtils::snprintf(tmp, uiLength, "%p", arg.m_Value);
  return ezStringView(tmp);
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, ezResult arg)
{
  if (arg.Failed())
    return "<failed>";
  else
    return "<succeeded>";
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezVariant& arg)
{
  ezString sString = arg.ConvertTo<ezString>();
  ezStringUtils::snprintf(tmp, uiLength, "%s", sString.GetData());
  return ezStringView(tmp);
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezAngle& arg)
{
  ezUInt32 writepos = 0;
  ezStringUtils::OutputFormattedFloat(tmp, uiLength-2, writepos, arg.GetDegree(), 1, false, 1, false);

  // Utf-8 representation of the degree sign
  tmp[writepos+0] = (char)0xC2;
  tmp[writepos+1] = (char)0xB0;
  tmp[writepos+2] = '\0';

  return ezStringView(tmp, tmp + writepos + 2);
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgHumanReadable& arg)
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
    ezStringUtils::OutputFormattedInt(tmp, uiLength, writepos, static_cast<ezInt64>(arg.m_Value), 1, false, 10);
  }
  else
  {
    ezStringUtils::OutputFormattedFloat(tmp, uiLength, writepos, arg.m_Value / divider, 1, false, 2, false);
  }
  ezStringUtils::Copy(tmp+writepos, uiLength - writepos, arg.m_Suffixes[suffixIndex]);

  return ezStringView(tmp);
}

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgErrorCode& arg)
{
  LPVOID lpMsgBuf = nullptr;
  FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, arg.m_ErrorCode,
    MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&lpMsgBuf, 0, nullptr);

  LPWSTR pCRLF = wcschr((LPWSTR)lpMsgBuf, L'\r');
  if (pCRLF != nullptr)
  {
    // remove the \r\n that FormatMessageW always appends
    *pCRLF = L'\0';
  }

  // we need a bigger boat
  static thread_local char FullMessage[256];

  ezStringUtils::snprintf(FullMessage, EZ_ARRAY_SIZE(FullMessage), "%i (\"%s\")", arg.m_ErrorCode, ezStringUtf8((LPWSTR)lpMsgBuf).GetData());
  LocalFree(lpMsgBuf);
  return ezStringView(FullMessage);
}
#endif

EZ_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_FormatString);

