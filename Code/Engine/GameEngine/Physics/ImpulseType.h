#pragma once

#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/String.h>
#include <GameEngine/GameEngineDLL.h>

struct EZ_GAMEENGINE_DLL ezImpulseType
{
  ezHashedString m_sName;
  float m_fDefaultValue;
  ezString m_sDescription;
  ezArrayMap<ezUInt8, float> m_WeightOverrides; // maps from WeightCategory index to impulse value
};

class EZ_GAMEENGINE_DLL ezImpulseTypeConfig
{
public:
  ezImpulseTypeConfig();
  ~ezImpulseTypeConfig();

  /// value for when a key doesn't exist
  static constexpr const ezUInt8 InvalidKey = 255;

  /// any key smaller than this is not a valid impulse type key, but may represent another option
  static constexpr const ezUInt8 FirstValidKey = 10;

  /// the impulse type key with this value corresponds to "<Custom Value>"
  static constexpr const ezUInt8 CustomValueKey = 0;

  /// the impulse type key with this value corresponds to "<None>"
  static constexpr const ezUInt8 NoValueKey = 1;

  void Save(ezStreamWriter& inout_stream) const;
  void Load(ezStreamReader& inout_stream);

  static constexpr const ezStringView s_sConfigFile = ":project/RuntimeConfigs/ImpulseTypes.cfg"_ezsv;

  ezResult Save(ezStringView sFile = s_sConfigFile) const;
  ezResult Load(ezStringView sFile = s_sConfigFile);

  /// \brief Returns the key of the element with the searched name, or InvalidKey, if it doesn't exist.
  ezUInt8 FindByName(ezTempHashedString sName) const;

  /// \brief Returns the next free key, or InvalidKey, if the list is full.
  ezUInt8 GetFreeKey() const;

  /// \brief Looks up the impulse type and returns either the default impulse value or the override impulse for the given weight category.
  ///
  /// Returns 1 for uiImpulseType == CustomValueKey and for non-existing impulse types.
  /// Returns 0 for uiImpulseType == NoValueKey
  ///
  /// Multiply the returned value by the normalized impulse direction vector.
  float GetImpulseForWeight(ezUInt8 uiImpulseType, ezUInt8 uiWeightCategory) const;

  // maps from impulse type index to type
  ezArrayMap<ezUInt8, ezImpulseType> m_Types;
};
