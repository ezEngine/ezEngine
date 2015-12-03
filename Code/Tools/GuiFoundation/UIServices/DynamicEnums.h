#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>


/// \todo Document this class

/// \brief Stores the valid values and names for 'dynamic' enums.
///
/// The names and valid values for dynamic enums may change due to user configuration changes. The UI should show these user specified names without restarting the tool.
class EZ_GUIFOUNDATION_DLL ezDynamicEnum
{
public:


  const ezMap<ezInt32, ezString>& GetAllValidValues() const { return m_ValidValues; }

  void Clear();

  void SetValueAndName(ezInt32 iValue, const char* szNewName);

  void RemoveValue(ezInt32 iValue);

  bool IsValueValid(ezInt32 iValue) const;

  const char* GetValueName(ezInt32 iValue) const;

  static ezDynamicEnum& GetDynamicEnum(const char* szEnumName);

private:
  ezMap<ezInt32, ezString> m_ValidValues;

  static ezMap<ezString, ezDynamicEnum> s_DynamicEnums;
};

