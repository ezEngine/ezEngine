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

  ezResult Save(ezStringView sFile = s_sConfigFile) const;
  ezResult Load(ezStringView sFile = s_sConfigFile);

  /// \brief Returns the index of the element with the searched name, or 255, if it doesn't exist.
  ezUInt8 FindByName(ezStringView sName) const;

  /// \brief Returns the next free key, or 255, if the list is full.
  ezUInt8 GetFreeKey() const;

  ezArrayMap<ezUInt8, ezWeightCategory> m_Categories;
};
