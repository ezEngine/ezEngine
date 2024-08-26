#pragma once

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Strings/String.h>
#include <GameEngine/GameEngineDLL.h>

/// \brief A 32x32 matrix of named filters that can be configured to enable or disable collisions
class EZ_GAMEENGINE_DLL ezCollisionFilterConfig
{
public:
  ezCollisionFilterConfig();
  ~ezCollisionFilterConfig();

  void SetGroupName(ezUInt32 uiGroup, ezStringView sName);

  ezStringView GetGroupName(ezUInt32 uiGroup) const;

  void EnableCollision(ezUInt32 uiGroup1, ezUInt32 uiGroup2, bool bEnable = true);

  bool IsCollisionEnabled(ezUInt32 uiGroup1, ezUInt32 uiGroup2) const;

  inline ezUInt32 GetFilterMask(ezUInt32 uiGroup) const { return m_GroupMasks[uiGroup]; }

  /// \brief Returns how many groups have non-empty names
  ezUInt32 GetNumNamedGroups() const;

  /// \brief Returns the index of the n-th group that has a non-empty name (ie. maps index '3' to index '5' if there are two unnamed groups in
  /// between)
  ezUInt32 GetNamedGroupIndex(ezUInt32 uiGroup) const;

  /// \brief Returns ezInvalidIndex if no group with the given name exists.
  ezUInt32 GetFilterGroupByName(ezStringView sName) const;

  /// \brief Searches for a group without a name and returns the index or ezInvalidIndex if none found.
  ezUInt32 FindUnnamedGroup() const;

  void Save(ezStreamWriter& inout_stream) const;
  void Load(ezStreamReader& inout_stream);

  static constexpr const ezStringView s_sConfigFile = ":project/RuntimeConfigs/CollisionLayers.cfg"_ezsv;

  ezResult Save(ezStringView sFile = s_sConfigFile) const;
  ezResult Load(ezStringView sFile = s_sConfigFile);


private:
  ezUInt32 m_GroupMasks[32];
  ezString m_GroupNames[32];
};
