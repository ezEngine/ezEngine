#include <TexConvPCH.h>

#include <TexConv/TexConv.h>

ezResult ezTexConv::ParseUIntOption(const char* szOption, ezInt32 iMinValue, ezInt32 iMaxValue, ezUInt32& uiResult) const
{
  const auto pCmd = ezCommandLineUtils::GetGlobalInstance();
  const ezUInt32 uiDefault = uiResult;

  const ezInt32 val = pCmd->GetIntOption(szOption, uiResult);

  if (!ezMath::IsInRange(val, iMinValue, iMaxValue))
  {
    ezLog::Error("'{}' value {} is out of valid range [{}; {}]", szOption, val, iMinValue, iMaxValue);
    return EZ_FAILURE;
  }

  uiResult = static_cast<ezUInt32>(val);

  if (uiResult == uiDefault)
  {
    ezLog::Info("Using default '{}': '{}'.", szOption, uiResult);
    return EZ_SUCCESS;
  }

  ezLog::Info("Selected '{}': '{}'.", szOption, uiResult);

  return EZ_SUCCESS;
}

ezResult ezTexConv::ParseStringOption(const char* szOption, const ezDynamicArray<KeyEnumValuePair>& allowed, ezInt32& iResult) const
{
  const auto pCmd = ezCommandLineUtils::GetGlobalInstance();
  const ezStringBuilder sValue = pCmd->GetStringOption(szOption, 0);

  if (sValue.IsEmpty())
  {
    iResult = allowed[0].m_iEnumValue;

    ezLog::Info("Using default '{}': '{}'", szOption, allowed[0].m_szKey);
    return EZ_SUCCESS;
  }

  for (ezUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    if (sValue.IsEqual_NoCase(allowed[i].m_szKey))
    {
      iResult = allowed[i].m_iEnumValue;

      ezLog::Info("Selected '{}': '{}'", szOption, allowed[i].m_szKey);
      return EZ_SUCCESS;
    }
  }

  ezLog::Error("Unknown value for option '{}': '{}'.", szOption, sValue);

  PrintOptionValues(szOption, allowed);

  return EZ_FAILURE;
}

void ezTexConv::PrintOptionValues(const char* szOption, const ezDynamicArray<KeyEnumValuePair>& allowed) const
{
  ezLog::Info("Valid values for option '{}' are:", szOption);

  for (ezUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    ezLog::Info("  {}", allowed[i].m_szKey);
  }
}

void ezTexConv::PrintOptionValuesHelp(const char* szOption, const ezDynamicArray<KeyEnumValuePair>& allowed) const
{
  ezStringBuilder out(szOption, " ");

  for (ezUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    if (i > 0)
      out.Append(" | ");

    out.Append(allowed[i].m_szKey);
  }

  ezLog::Info(out);
}

bool ezTexConv::ParseFile(const char* szOption, ezString& result) const
{
  const auto pCmd = ezCommandLineUtils::GetGlobalInstance();
  result = pCmd->GetAbsolutePathOption(szOption);

  if (!result.IsEmpty())
  {
    ezLog::Info("'{}' file: '{}'", szOption, result);
    return true;
  }
  else
  {
    ezLog::Info("No '{}' file specified.", szOption);
    return false;
  }
}
