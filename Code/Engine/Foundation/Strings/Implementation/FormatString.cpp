#include <Foundation/PCH.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/String.h>

#if (__cplusplus >= 201402L || _MSC_VER >= 1900)

void ezFormatString::AppendView(ezStringBuilder& sb, const ezStringView& sub)
{
  sb.Append(sub);
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

ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgUI& arg)
{
  ezUInt32 writepos = 0;
  ezStringUtils::OutputFormattedUInt(tmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_uiBase);
  tmp[writepos] = '\0';
  return ezStringView(tmp, tmp + writepos);
}

ezStringView BuildString(char* tmp, ezUInt32 uiLength, ezUInt64 arg)
{
  ezUInt32 writepos = 0;
  ezStringUtils::OutputFormattedUInt(tmp, uiLength, writepos, arg, 1, false, 10);
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
  ezStringUtils::OutputFormattedFloat(tmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_iPrecision, arg.m_bScientific);
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

ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezStringBuilder& arg)
{
  return ezStringView(arg.GetData(), arg.GetData() + arg.GetElementCount());
}

const ezStringView& BuildString(char* tmp, ezUInt32 uiLength, const ezStringView& arg)
{
  return arg;
}

#endif

