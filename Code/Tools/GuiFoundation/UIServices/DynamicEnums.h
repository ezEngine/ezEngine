#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

/// \brief Stores the valid values and names for 'dynamic' enums.
///
/// The names and valid values for dynamic enums may change due to user configuration changes.
/// The UI should show these user specified names without restarting the tool.
///
/// Call the static function GetDynamicEnum() to create or get the ezDynamicEnum for a specific type.
class EZ_GUIFOUNDATION_DLL ezDynamicEnum
{
public:

  /// \brief Returns a ezDynamicEnum under the given name. Creates a new one, if the name has not been used before.
  static ezDynamicEnum& GetDynamicEnum(const char* szEnumName);

  /// \brief Returns all enum values and current names.
  const ezMap<ezInt32, ezString>& GetAllValidValues() const { return m_ValidValues; }

  /// \brief Resets the internal data.
  void Clear();

  /// \brief Sets the name for the given enum value.
  void SetValueAndName(ezInt32 iValue, const char* szNewName);

  /// \brief Removes a certain enum value, if it exists.
  void RemoveValue(ezInt32 iValue);

  /// \brief Returns whether a certain value is known.
  bool IsValueValid(ezInt32 iValue) const;

  /// \brief Returns the name for the given value. Returns "<invalid value>" if the value is not in use.
  const char* GetValueName(ezInt32 iValue) const;

private:
  ezMap<ezInt32, ezString> m_ValidValues;

  static ezMap<ezString, ezDynamicEnum> s_DynamicEnums;
};

