#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/GuiFoundationDLL.h>

/// Stores the valid values and names for 'dynamic' bitflags.
///
/// The names and valid values for dynamic bitflags may change due to user configuration changes.
/// The UI should show these user specified names without restarting the tool.
///
/// Call the static function GetDynamicBitflags() to create or get the ezDynamicBitflags for a specific type.
class EZ_GUIFOUNDATION_DLL ezDynamicBitflags
{
public:
  /// Returns a ezDynamicBitflags under the given name. Creates a new one, if the name has not been used before.
  static ezDynamicBitflags& GetDynamicBitflags(ezStringView sName);

  /// Returns all bitflag values and current names.
  const ezMap<ezUInt64, ezString>& GetAllValidValues() const { return m_ValidValues; }

  /// Resets stored values.
  void Clear();

  /// Sets the name for the given bit position.
  void SetValueAndName(ezUInt32 uiBitPos, ezStringView sName);

  /// Removes a value, if it exists.
  void RemoveValue(ezUInt32 uiBitPos);

  /// Returns whether a certain value is known.
  bool IsValueValid(ezUInt32 uiBitPos) const;

  /// Returns the name for the given value
  bool TryGetValueName(ezUInt32 uiBitPos, ezStringView& out_sName) const;

private:
  ezMap<ezUInt64, ezString> m_ValidValues;

  static ezMap<ezString, ezDynamicBitflags> s_DynamicBitflags;
};
