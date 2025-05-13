#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/DynamicEnums.h>

ezMap<ezString, ezDynamicEnum> ezDynamicEnum::s_DynamicEnums;

void ezDynamicEnum::Clear()
{
  m_ValidValues.Clear();
}

void ezDynamicEnum::SetValueAndName(ezInt32 iValue, ezStringView sNewName)
{
  m_ValidValues[iValue] = sNewName;
}

void ezDynamicEnum::RemoveValue(ezInt32 iValue)
{
  m_ValidValues.Remove(iValue);
}

bool ezDynamicEnum::IsValueValid(ezInt32 iValue) const
{
  return m_ValidValues.Find(iValue).IsValid();
}

ezStringView ezDynamicEnum::GetValueName(ezInt32 iValue) const
{
  auto it = m_ValidValues.Find(iValue);

  if (!it.IsValid())
    return "<invalid value>";

  return it.Value();
}

void ezDynamicEnum::SetEditCommand(ezStringView sCmd, const ezVariant& value)
{
  m_sEditCommand = sCmd;
  m_EditCommandValue = value;
}

ezDynamicEnum& ezDynamicEnum::GetDynamicEnum(const char* szEnumName)
{
  return s_DynamicEnums[szEnumName];
}
