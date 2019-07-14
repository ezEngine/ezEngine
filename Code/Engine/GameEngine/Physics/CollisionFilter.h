#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Strings/String.h>

/// \brief A 32x32 matrix of named filters that can be configured to enable or disable collisions
class EZ_GAMEENGINE_DLL ezCollisionFilterConfig
{
public:
  ezCollisionFilterConfig();

  void SetGroupName(ezUInt32 uiGroup, const char* szName);

  const char* GetGroupName(ezUInt32 uiGroup) const;

  void EnableCollision(ezUInt32 uiGroup1, ezUInt32 uiGroup2, bool bEnable = true);

  bool IsCollisionEnabled(ezUInt32 uiGroup1, ezUInt32 uiGroup2) const;

  inline ezUInt32 GetFilterMask(ezUInt32 uiGroup) const { return m_GroupMasks[uiGroup]; }

  /// \brief Returns how many groups have non-empty names
  ezUInt32 GetNumNamedGroups() const;

  /// \brief Returns the index of the n-th group that has a non-empty name (ie. maps index '3' to index '5' if there are two unnamed groups in between)
  ezUInt32 GetNamedGroupIndex(ezUInt32 uiGroup) const;

  /// \brief Returns -1 if no group with the given name exists.
  ezInt32 GetFilterGroupByName(const char* szName) const;

  /// \brief Searches for a group without a name and returns the index or -1 if none found.
  ezInt32 FindUnnamedGroup() const;

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);

  ezResult Save(const char* szFile) const;
  ezResult Load(const char* szFile);


private:
  ezUInt32 m_GroupMasks[32];
  char m_GroupNames[32][32];


};


