#pragma once

#include <GameUtils/Basics.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Strings/String.h>

/// \brief A 32x32 matrix of named filters that can be configured to enable or disable collisions
class EZ_GAMEUTILS_DLL ezCollisionFilterConfig
{
public:
  ezCollisionFilterConfig();

  void SetFilterGroupCount(ezUInt32 uiNumGroups);

  ezUInt32 GetFilterGroupCount() const;

  void SetGroupName(ezUInt32 uiGroup, const char* szName);

  const char* GetGroupName(ezUInt32 uiGroup) const;

  void EnableCollision(ezUInt32 uiGroup1, ezUInt32 uiGroup2, bool bEnable = true);

  bool IsCollisionEnabled(ezUInt32 uiGroup1, ezUInt32 uiGroup2) const;

  inline ezUInt32 GetFilterMask(ezUInt32 uiGroup) const { return m_GroupMasks[uiGroup]; }

  ezUInt32 GetFilterGroupByName(const char* szName) const;


private:
  ezStaticArray<ezUInt32, 32> m_GroupMasks;
  ezStaticArray<ezString, 32> m_GroupNames;
};




