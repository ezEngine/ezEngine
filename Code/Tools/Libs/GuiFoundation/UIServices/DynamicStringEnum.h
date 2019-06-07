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
class EZ_GUIFOUNDATION_DLL ezDynamicStringEnum
{
public:

  /// \brief Returns a ezDynamicEnum under the given name. Creates a new one, if the name has not been used before.
  static ezDynamicStringEnum& GetDynamicEnum(const char* szEnumName);

  /// \brief Returns all enum values and current names.
  const ezHybridArray<ezString, 16>& GetAllValidValues() const { return m_ValidValues; }

  /// \brief Resets the internal data.
  void Clear();

  /// \brief Sets the name for the given enum value.
  void AddValidValue(const char* szValue, bool bSortValues = false);

  /// \brief Removes a certain enum value, if it exists.
  void RemoveValue(const char* szValue);

  /// \brief Returns whether a certain value is known.
  bool IsValueValid(const char* szValue) const;

private:
  ezHybridArray<ezString, 16> m_ValidValues;

  static ezMap<ezString, ezDynamicStringEnum> s_DynamicEnums;
};

