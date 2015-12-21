#include <GameUtils/PCH.h>
#include <GameUtils/CollisionFilter/CollisionFilter.h>


ezCollisionFilterConfig::ezCollisionFilterConfig()
{
  for (int i = 0; i < 32; ++i)
  {
    ezMemoryUtils::ZeroFill<char>(m_GroupNames[i], 32);

    m_GroupMasks[i] = 0xFFFFFFFF; // collide with everything
  }
}

void ezCollisionFilterConfig::SetGroupName(ezUInt32 uiGroup, const char* szName)
{
  ezStringUtils::Copy(m_GroupNames[uiGroup], 32, szName);
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

ezUInt32 ezCollisionFilterConfig::GetNumNamedGroups() const
{
  ezUInt32 count = 0;

  for (ezUInt32 i = 0; i < 32; ++i)
  {
    if (!ezStringUtils::IsNullOrEmpty(m_GroupNames[i]))
      ++count;
  }

  return count;
}

ezUInt32 ezCollisionFilterConfig::GetNamedGroupIndex(ezUInt32 uiGroup) const
{
  for (ezUInt32 i = 0; i < 32; ++i)
  {
    if (!ezStringUtils::IsNullOrEmpty(m_GroupNames[i]))
    {
      if (uiGroup == 0)
        return i;

      --uiGroup;
    }
  }

  EZ_REPORT_FAILURE("Invalid index, there are not so many named collision filter groups");
  return -1;
}

ezInt32 ezCollisionFilterConfig::GetFilterGroupByName(const char* szName) const
{
  for (ezUInt32 i = 0; i < 32; ++i)
  {
    if (ezStringUtils::IsEqual_NoCase(m_GroupNames[i], szName))
      return i;
  }

  return -1;
}

ezInt32 ezCollisionFilterConfig::FindUnnamedGroup() const
{
  for (ezUInt32 i = 0; i < 32; ++i)
  {
    if (ezStringUtils::IsNullOrEmpty(m_GroupNames[i]))
      return i;
  }

  return -1;
}

ezResult ezCollisionFilterConfig::Save(const char* szFile) const
{
  ezFileWriter file;
  if (file.Open(szFile).Failed())
    return EZ_FAILURE;

  Save(file);

  return EZ_SUCCESS;
}

ezResult ezCollisionFilterConfig::Load(const char* szFile)
{
  ezFileReader file;
  if (file.Open(szFile).Failed())
    return EZ_FAILURE;

  Load(file);
  return EZ_SUCCESS;
}

void ezCollisionFilterConfig::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;

  stream << uiVersion;

  stream.WriteBytes(m_GroupMasks, sizeof(ezUInt32) * 32);
  stream.WriteBytes(m_GroupNames, sizeof(char) * 32 * 32);
}


void ezCollisionFilterConfig::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;

  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion == 1, "Invalid version %u for ezCollisionFilterConfig file", uiVersion);

  stream.ReadBytes(m_GroupMasks, sizeof(ezUInt32) * 32);
  stream.ReadBytes(m_GroupNames, sizeof(char) * 32 * 32);
}

