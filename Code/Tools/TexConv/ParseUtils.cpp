#include <TexConv/TexConvPCH.h>

#include <TexConv/TexConv.h>

ezResult ezTexConv::ParseUIntOption(ezStringView sOption, ezInt32 iMinValue, ezInt32 iMaxValue, ezUInt32& ref_uiResult) const
{
  const auto pCmd = ezCommandLineUtils::GetGlobalInstance();
  const ezUInt32 uiDefault = ref_uiResult;

  const ezInt32 val = pCmd->GetIntOption(sOption, ref_uiResult);

  if (!ezMath::IsInRange(val, iMinValue, iMaxValue))
  {
    ezLog::Error("'{}' value {} is out of valid range [{}; {}]", sOption, val, iMinValue, iMaxValue);
    return EZ_FAILURE;
  }

  ref_uiResult = static_cast<ezUInt32>(val);

  if (ref_uiResult == uiDefault)
  {
    ezLog::Info("Using default '{}': '{}'.", sOption, ref_uiResult);
    return EZ_SUCCESS;
  }

  ezLog::Info("Selected '{}': '{}'.", sOption, ref_uiResult);

  return EZ_SUCCESS;
}

ezResult ezTexConv::ParseStringOption(ezStringView sOption, const ezDynamicArray<KeyEnumValuePair>& allowed, ezInt32& ref_iResult) const
{
  const auto pCmd = ezCommandLineUtils::GetGlobalInstance();
  const ezStringBuilder sValue = pCmd->GetStringOption(sOption, 0);

  if (sValue.IsEmpty())
  {
    ref_iResult = allowed[0].m_iEnumValue;

    ezLog::Info("Using default '{}': '{}'", sOption, allowed[0].m_sKey);
    return EZ_SUCCESS;
  }

  for (ezUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    if (sValue.IsEqual_NoCase(allowed[i].m_sKey))
    {
      ref_iResult = allowed[i].m_iEnumValue;

      ezLog::Info("Selected '{}': '{}'", sOption, allowed[i].m_sKey);
      return EZ_SUCCESS;
    }
  }

  ezLog::Error("Unknown value for option '{}': '{}'.", sOption, sValue);

  PrintOptionValues(sOption, allowed);

  return EZ_FAILURE;
}

void ezTexConv::PrintOptionValues(ezStringView sOption, const ezDynamicArray<KeyEnumValuePair>& allowed) const
{
  ezLog::Info("Valid values for option '{}' are:", sOption);

  for (ezUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    ezLog::Info("  {}", allowed[i].m_sKey);
  }
}

void ezTexConv::PrintOptionValuesHelp(ezStringView sOption, const ezDynamicArray<KeyEnumValuePair>& allowed) const
{
  ezStringBuilder out(sOption, " ");

  for (ezUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    if (i > 0)
      out.Append(" | ");

    out.Append(allowed[i].m_sKey);
  }

  ezLog::Info(out);
}

bool ezTexConv::ParseFile(ezStringView sOption, ezString& ref_sResult) const
{
  const auto pCmd = ezCommandLineUtils::GetGlobalInstance();
  ref_sResult = pCmd->GetAbsolutePathOption(sOption);

  if (!ref_sResult.IsEmpty())
  {
    ezLog::Info("'{}' file: '{}'", sOption, ref_sResult);
    return true;
  }
  else
  {
    ezLog::Info("No '{}' file specified.", sOption);
    return false;
  }
}
