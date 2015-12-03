#include <GameUtils/PCH.h>
#include <GameUtils/CollisionFilter/CollisionFilter.h>

ezCollisionFilterConfig::ezCollisionFilterConfig()
{
  int i = 0;
}

void ezCollisionFilterConfig::SetFilterGroupCount(ezUInt32 uiNumGroups)
{
  m_GroupMasks.SetCount(uiNumGroups);
  m_GroupNames.SetCount(uiNumGroups);
}

ezUInt32 ezCollisionFilterConfig::GetFilterGroupCount() const
{
  return m_GroupNames.GetCount();
}

void ezCollisionFilterConfig::SetGroupName(ezUInt32 uiGroup, const char* szName)
{
  m_GroupNames[uiGroup] = szName;
}

const char* ezCollisionFilterConfig::GetGroupName(ezUInt32 uiGroup) const
{
  return m_GroupNames[uiGroup];
}

void ezCollisionFilterConfig::EnableCollision(ezUInt32 uiGroup1, ezUInt32 uiGroup2, bool bEnable)
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

bool ezCollisionFilterConfig::IsCollisionEnabled(ezUInt32 uiGroup1, ezUInt32 uiGroup2) const
{
  return (m_GroupMasks[uiGroup1] & EZ_BIT(uiGroup2)) != 0;
}

ezUInt32 ezCollisionFilterConfig::GetFilterGroupByName(const char* szName) const
{
  return m_GroupNames.IndexOf(szName);
}

