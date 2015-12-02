#pragma once

#include <PhysXPlugin/Basics.h>

class EZ_PHYSXPLUGIN_DLL ezPxCollisionFilterConfig
{
public:

  void SetFilterGroupCount(ezUInt32 uiNumGroups);

  ezUInt32 GetFilterGroupCount() const;

  void SetGroupName(ezUInt32 uiGroup, const char* szName);

  const char* GetGroupName(ezUInt32 uiGroup) const;

  void EnableCollision(ezUInt32 uiGroup1, ezUInt32 uiGroup2, bool bEnable = true);

  inline ezUInt32 GetFilterMask(ezUInt32 uiGroup) const { return m_GroupMasks[uiGroup]; }

  ezUInt32 GetFilterGroupByName(const char* szName) const;


private:
  ezStaticArray<ezUInt32, 32> m_GroupMasks;
  ezStaticArray<ezString, 32> m_GroupNames;
};




