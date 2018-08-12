#include <PCH.h>

#include <GuiFoundation/UIServices/DynamicStringEnum.h>

ezMap<ezString, ezDynamicStringEnum> ezDynamicStringEnum::s_DynamicEnums;

void ezDynamicStringEnum::Clear()
{
  m_ValidValues.Clear();
}

void ezDynamicStringEnum::AddValidValue(const char* szNewName, bool bSortValues /*= false*/)
{
  ezString sName = szNewName;

  if (!m_ValidValues.Contains(sName))
    m_ValidValues.PushBack(sName);

  if (bSortValues)
    m_ValidValues.Sort();
}

void ezDynamicStringEnum::RemoveValue(const char* szValue)
{
  m_ValidValues.Remove(szValue);
}

bool ezDynamicStringEnum::IsValueValid(const char* szValue) const
{
  return m_ValidValues.Contains(szValue);
}

ezDynamicStringEnum& ezDynamicStringEnum::GetDynamicEnum(const char* szEnumName)
{
  return s_DynamicEnums[szEnumName];
}
