#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/CollisionFilter/CollisionFilter.h>



void ezPxCollisionFilterConfig::SetFilterGroupCount(ezUInt32 uiNumGroups)
{
  m_GroupMasks.SetCount(uiNumGroups);
  m_GroupNames.SetCount(uiNumGroups);
}

ezUInt32 ezPxCollisionFilterConfig::GetFilterGroupCount() const
{
  return m_GroupNames.GetCount();
}

void ezPxCollisionFilterConfig::SetGroupName(ezUInt32 uiGroup, const char* szName)
{
  m_GroupNames[uiGroup] = szName;
}

const char* ezPxCollisionFilterConfig::GetGroupName(ezUInt32 uiGroup) const
{
  return m_GroupNames[uiGroup];
}

void ezPxCollisionFilterConfig::EnableCollision(ezUInt32 uiGroup1, ezUInt32 uiGroup2, bool bEnable)
{
  if (bEnable)
  {
    m_GroupMasks[uiGroup1] |= EZ_BIT(uiGroup2);
    m_GroupMasks[uiGroup2] |= EZ_BIT(uiGroup1);
  }
  else
  {
    m_GroupMasks[uiGroup1] &= ~EZ_BIT(uiGroup2);
    m_GroupMasks[uiGroup2] &= ~EZ_BIT(uiGroup1);
  }
}

ezUInt32 ezPxCollisionFilterConfig::GetFilterGroupByName(const char* szName) const
{
  return m_GroupNames.IndexOf(szName);
}

