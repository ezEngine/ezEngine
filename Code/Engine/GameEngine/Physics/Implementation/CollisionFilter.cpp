#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GameEngine/Physics/CollisionFilter.h>


ezCollisionFilterConfig::ezCollisionFilterConfig()
{
  for (int i = 0; i < 32; ++i)
  {
    ezMemoryUtils::ZeroFill<char>(m_GroupNames[i], 32);

    m_GroupMasks[i] = 0xFFFFFFFF; // collide with everything
  }
}

void ezCollisionFilterConfig::SetGroupName(ezUInt32 uiGroup, ezStringView sName)
{
  ezStringBuilder tmp;
  ezStringUtils::Copy(m_GroupNames[uiGroup], 32, sName.GetData(tmp));
}

ezStringView ezCollisionFilterConfig::GetGroupName(ezUInt32 uiGroup) const
{
  return ezStringView((const char*)m_GroupNames[uiGroup]);
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
  return ezInvalidIndex;
}

ezUInt32 ezCollisionFilterConfig::GetFilterGroupByName(ezStringView sName) const
{
  for (ezUInt32 i = 0; i < 32; ++i)
  {
    if (sName.IsEqual_NoCase(m_GroupNames[i]))
      return i;
  }

  return ezInvalidIndex;
}

ezUInt32 ezCollisionFilterConfig::FindUnnamedGroup() const
{
  for (ezUInt32 i = 0; i < 32; ++i)
  {
    if (ezStringUtils::IsNullOrEmpty(m_GroupNames[i]))
      return i;
  }

  return ezInvalidIndex;
}

ezResult ezCollisionFilterConfig::Save(ezStringView sFile) const
{
  ezFileWriter file;
  if (file.Open(sFile).Failed())
    return EZ_FAILURE;

  Save(file);

  return EZ_SUCCESS;
}

ezResult ezCollisionFilterConfig::Load(ezStringView sFile)
{
#if EZ_ENABLED(EZ_MIGRATE_RUNTIMECONFIGS)
  if (sFile == s_sConfigFile)
  {
    sFile = ezFileSystem::MigrateFileLocation(":project/CollisionLayers.cfg", s_sConfigFile);
  }
#endif

  ezFileReader file;
  if (file.Open(sFile).Failed())
    return EZ_FAILURE;

  Load(file);
  return EZ_SUCCESS;
}

void ezCollisionFilterConfig::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 1;

  inout_stream << uiVersion;

  inout_stream.WriteBytes(m_GroupMasks, sizeof(ezUInt32) * 32).IgnoreResult();
  inout_stream.WriteBytes(m_GroupNames, sizeof(char) * 32 * 32).IgnoreResult();
}


void ezCollisionFilterConfig::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion == 1, "Invalid version {0} for ezCollisionFilterConfig file", uiVersion);

  inout_stream.ReadBytes(m_GroupMasks, sizeof(ezUInt32) * 32);
  inout_stream.ReadBytes(m_GroupNames, sizeof(char) * 32 * 32);
}
