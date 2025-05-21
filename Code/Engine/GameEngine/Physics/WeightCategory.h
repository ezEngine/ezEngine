#pragma once

#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/String.h>
#include <GameEngine/GameEngineDLL.h>

struct EZ_GAMEENGINE_DLL ezWeightCategory
{
  ezHashedString m_sName;
  float m_fMass = 10.0f;
  ezString m_sDescription;
};

class EZ_GAMEENGINE_DLL ezWeightCategoryConfig
{
public:
  ezWeightCategoryConfig();
  ~ezWeightCategoryConfig();

  void Save(ezStreamWriter& inout_stream) const;
  void Load(ezStreamReader& inout_stream);

  static constexpr const ezStringView s_sConfigFile = ":project/RuntimeConfigs/WeightCategories.cfg"_ezsv;

  /// value for when a key doesn't exist
  static constexpr const ezUInt8 InvalidKey = 255;

  /// any key smaller than this is not a valid weight category key, but may represent another option
  static constexpr const ezUInt8 FirstValidKey = 10;

  /// the impulse type key with this value corresponds to "<Default>", which stands for a good default mass (that may be different for different component types)
  static constexpr const ezUInt8 DefaultValueKey = 0;

  /// the impulse type key with this value corresponds to "<Custom Mass>"
  static constexpr const ezUInt8 CustomMassKey = 1;

  /// the impulse type key with this value corresponds to "<Custom Density>"
  static constexpr const ezUInt8 CustomDensityKey = 2;

  ezResult Save(ezStringView sFile = s_sConfigFile) const;
  ezResult Load(ezStringView sFile = s_sConfigFile);

  /// \brief Returns the index of the element with the searched name, or InvalidKey, if it doesn't exist.
  ezUInt8 FindByName(ezTempHashedString sName) const;

  /// \brief Returns the next free key, or InvalidKey, if the list is full.
  ezUInt8 GetFreeKey() const;

  /// \brief Returns the mass according to the weight category.
  ///
  /// * The default mass, if uiWeightCategory == DefaultValueKey
  /// * The custom mass, if uiWeightCategory == CustomMassKey
  /// * 0, if uiWeightCategory == CustomDensityKey
  /// * the mapped mass (scaled and clamped) for an existing key
  /// * the default value, if the key doesn't exist
  float GetMassForWeightCategory(ezUInt8 uiWeightCategory, float fDefaultMass, float fCustomMass, float fWeightScale, float fMinMass = 1.0f, float fMaxMass = 1000.0f) const;

  ezArrayMap<ezUInt8, ezWeightCategory> m_Categories;
};
