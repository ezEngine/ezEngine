#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/GuiFoundationDLL.h>

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
  ///
  /// Calls s_RequestUnknownCallback, if the requested enum is not known yet, which will try to load the data.
  static ezDynamicStringEnum& GetDynamicEnum(ezStringView sEnumName);

  /// \brief Always (re-) creates the ezDynamicEnum under the requested name.
  ///
  /// Use this when you intend to reset the values and don't want them to be loaded from file.
  static ezDynamicStringEnum& CreateDynamicEnum(ezStringView sEnumName);

  /// \brief Removes the entire enum with the given name.
  static void RemoveEnum(ezStringView sEnumName);

  /// \brief Returns all enum values and current names.
  const ezHybridArray<ezString, 16>& GetAllValidValues() const { return m_ValidValues; }

  /// \brief Resets the internal data.
  void Clear();

  /// \brief Sets the name for the given enum value.
  void AddValidValue(ezStringView sValue, bool bSortValues = false);

  /// \brief Removes a certain enum value, if it exists.
  void RemoveValue(ezStringView sValue);

  /// \brief Returns whether a certain value is known.
  bool IsValueValid(ezStringView sValue) const;

  /// \brief Sorts existing values alphabetically
  void SortValues();

  /// \brief If set to non-empty, the user can easily edit this enum through a simple dialog and the values will be saved in this file.
  ///
  /// Empty by default, as most dynamic enums need to be set up according to other criteria.
  void SetStorageFile(ezStringView sFile) { m_sStorageFile = sFile; }

  /// \brief The file where values will be stored.
  ezStringView GetStorageFile() const { return m_sStorageFile; }

  /// \brief If specified, the widget shows an "edit" option, which will run ezActionManager::ExecuteAction(sCmd, value)
  ///
  /// This is meant to be used to open existing config dialogs.
  /// There is currently no way to report back a selection, so after making changes, the user has to make another selection.
  void SetEditCommand(ezStringView sCmd, const ezVariant& value);
  ezStringView GetEditCommand() const { return m_sEditCommand; }
  const ezVariant& GetEditCommandValue() const { return m_EditCommandValue; }

  void ReadFromStorage();

  void SaveToStorage();

  /// \brief Invoked by GetDynamicEnum() for enums that are unkonwn at that time.
  ///
  /// Can be used to on-demand load those values, before GetDynamicEnum() returns.
  static ezDelegate<void(ezStringView sEnumName, ezDynamicStringEnum& e)> s_RequestUnknownCallback;

private:
  ezHybridArray<ezString, 16> m_ValidValues;
  ezString m_sStorageFile;
  ezString m_sEditCommand;
  ezVariant m_EditCommandValue;

  static ezMap<ezString, ezDynamicStringEnum> s_DynamicEnums;
};
