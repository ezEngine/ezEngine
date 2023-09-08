#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/GuiFoundationDLL.h>

/// \brief Stores the valid values and names for 'dynamic' bitflags.
///
/// The names and valid values for dynamic bitflags may change due to user configuration changes.
/// The UI should show these user specified names without restarting the tool.
///
/// Call the static function GetDynamicBitflags() to create or get the ezDynamicBitflags for a specific type.
class EZ_GUIFOUNDATION_DLL ezDynamicBitflags
{
public:
  /// \brief Returns a ezDynamicBitflags under the given name. Creates a new one, if the name has not been used before.
  static ezDynamicBitflags& GetDynamicBitflags(const char* szName);

  /// \brief Returns all bitflag values and current names.
  const ezMap<ezInt64, ezString>& GetAllValidValues() const { return m_ValidValues; }

  /// \brief Resets the internal data.
  void Clear();

  /// \brief Sets the name for the given bit position.
  void SetBitPosAndName(ezInt32 iBitPos, const char* szNewName);

  /// \brief Removes a certain bit value, if it exists.
  void RemoveValue(ezInt32 iBitPos);

  /// \brief Returns whether a certain value is known.
  bool IsValueValid(ezInt32 iBitPos) const;

  /// \brief Returns the name for the given value. Returns "<invalid value>" if the value is not in use.
  const char* GetBitName(ezInt32 iBitPos) const;

private:
  ezMap<ezInt64, ezString> m_ValidValues;

  static ezMap<ezString, ezDynamicBitflags> s_DynamicBitflags;
};
