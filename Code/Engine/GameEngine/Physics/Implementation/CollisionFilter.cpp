#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GameEngine/Physics/CollisionFilter.h>

ezCollisionFilterConfig::ezCollisionFilterConfig()
{
  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_GroupMasks); ++i)
  {
    m_GroupMasks[i] = 0;
  }
}

ezCollisionFilterConfig::~ezCollisionFilterConfig() = default;

void ezCollisionFilterConfig::SetGroupName(ezUInt32 uiGroup, ezStringView sName)
{
  m_GroupNames[uiGroup] = sName;
}

ezStringView ezCollisionFilterConfig::GetGroupName(ezUInt32 uiGroup) const
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
    if (!m_GroupNames[i].IsEmpty())
      ++count;
  }

  return count;
}

ezUInt32 ezCollisionFilterConfig::GetNamedGroupIndex(ezUInt32 uiGroup) const
{
  for (ezUInt32 i = 0; i < 32; ++i)
  {
    if (!m_GroupNames[i].IsEmpty())
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
    if (m_GroupNames[i].IsEmpty())
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
  ezFileReader file;
  if (file.Open(sFile).Failed())
    return EZ_FAILURE;

  Load(file);
  return EZ_SUCCESS;
}

void ezCollisionFilterConfig::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 2;

  inout_stream << uiVersion;

  inout_stream.WriteBytes(m_GroupMasks, sizeof(ezUInt32) * 32).AssertSuccess();

  for (ezUInt32 i = 0; i < 32; ++i)
  {
    inout_stream << m_GroupNames[i];
  }
}


void ezCollisionFilterConfig::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion == 1 || uiVersion == 2, "Invalid version {0} for ezCollisionFilterConfig file", uiVersion);

  inout_stream.ReadBytes(m_GroupMasks, sizeof(ezUInt32) * 32);

  for (ezUInt32 i1 = 0; i1 < 32; ++i1)
  {
    for (ezUInt32 i2 = 0; i2 < 32; ++i2)
    {
      const bool b1 = (m_GroupMasks[i1] & EZ_BIT(i2)) != 0;
      const bool b2 = (m_GroupMasks[i2] & EZ_BIT(i1)) != 0;

      if (b1 != b2)
      {
        // reset group masks that differ due to previously uninitialized memory
        m_GroupMasks[i1] &= ~EZ_BIT(i2);
        m_GroupMasks[i2] &= ~EZ_BIT(i1);
      }
    }
  }

  if (uiVersion == 1)
  {
    char groupNames[32][32];
    inout_stream.ReadBytes(groupNames, sizeof(char) * 32 * 32);

    for (ezUInt32 i = 0; i < 32; ++i)
    {
      m_GroupNames[i] = groupNames[i];
    }
  }
  else
  {
    for (ezUInt32 i = 0; i < 32; ++i)
    {
      inout_stream >> m_GroupNames[i];
    }
  }
}
