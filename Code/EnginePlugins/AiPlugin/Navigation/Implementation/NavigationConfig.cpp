#include <AiPlugin/Navigation/NavigationConfig.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

ezAiNavigationConfig::ezAiNavigationConfig()
{
  m_GroundTypes[0].m_bUsed = true;
  m_GroundTypes[0].m_sName = "<None>";
  m_GroundTypes[1].m_bUsed = true;
  m_GroundTypes[1].m_sName = "<Default>";

  ezStringBuilder tmp;
  for (ezUInt32 i = 2; i < ezAiNumGroundTypes; ++i)
  {
    tmp.SetFormat("Custom Ground Type {}", i + 1);
    m_GroundTypes[i].m_sName = tmp;
  }

  for (ezUInt32 i = 0; i < ezAiNumGroundTypes / 8; ++i)
  {
    const float f = 1.0f + i * 0.5f;
    m_GroundTypes[(i * 8) + 0].m_Color = ezColor::RosyBrown.GetDarker(f);
    m_GroundTypes[(i * 8) + 1].m_Color = ezColor::CornflowerBlue.GetDarker(f);
    m_GroundTypes[(i * 8) + 2].m_Color = ezColor::Crimson.GetDarker(f);
    m_GroundTypes[(i * 8) + 3].m_Color = ezColor::Cyan.GetDarker(f);
    m_GroundTypes[(i * 8) + 4].m_Color = ezColor::DarkSeaGreen.GetDarker(f);
    m_GroundTypes[(i * 8) + 5].m_Color = ezColor::DarkViolet.GetDarker(f);
    m_GroundTypes[(i * 8) + 6].m_Color = ezColor::HoneyDew.GetDarker(f);
    m_GroundTypes[(i * 8) + 7].m_Color = ezColor::LightSalmon.GetDarker(f);
  }
}

ezResult ezAiNavigationConfig::Save(ezStringView sFile) const
{
  ezFileWriter file;
  if (file.Open(sFile).Failed())
    return EZ_FAILURE;

  Save(file);

  return EZ_SUCCESS;
}

ezResult ezAiNavigationConfig::Load(ezStringView sFile)
{
  ezFileReader file;
  if (file.Open(sFile).Failed())
    return EZ_FAILURE;

  Load(file);
  return EZ_SUCCESS;
}

void ezAiNavigationConfig::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 2;

  inout_stream << uiVersion;

  const ezUInt8 numGroundTypes = ezAiNumGroundTypes;
  inout_stream << numGroundTypes;

  for (ezUInt32 i = 0; i < numGroundTypes; ++i)
  {
    const auto& gt = m_GroundTypes[i];
    inout_stream << gt.m_bUsed;
    inout_stream << gt.m_sName;
    inout_stream << gt.m_Color;
  }

  const ezUInt8 numSearchTypes = m_PathSearchConfigs.GetCount();
  inout_stream << numSearchTypes;

  for (ezUInt32 i = 0; i < numSearchTypes; ++i)
  {
    const auto& cfg = m_PathSearchConfigs[i];

    inout_stream << cfg.m_sName;
    inout_stream.WriteArray(cfg.m_fGroundTypeCost).AssertSuccess();
    inout_stream.WriteArray(cfg.m_bGroundTypeAllowed).AssertSuccess();
  }

  const ezUInt8 numNavmeshTypes = m_NavmeshConfigs.GetCount();
  inout_stream << numNavmeshTypes;

  for (ezUInt32 i = 0; i < numNavmeshTypes; ++i)
  {
    const auto& nc = m_NavmeshConfigs[i];

    inout_stream << nc.m_sName;

    inout_stream << nc.m_uiNumSectorsX;
    inout_stream << nc.m_uiNumSectorsY;
    inout_stream << nc.m_fSectorSize;

    inout_stream << nc.m_uiCollisionLayer;
    inout_stream << nc.m_fCellSize;
    inout_stream << nc.m_fCellHeight;
    inout_stream << nc.m_fAgentRadius;
    inout_stream << nc.m_fAgentHeight;
    inout_stream << nc.m_fAgentStepHeight;
    inout_stream << nc.m_WalkableSlope;

    inout_stream << nc.m_fMaxEdgeLength;
    inout_stream << nc.m_fMaxSimplificationError;
    inout_stream << nc.m_fMinRegionSize;
    inout_stream << nc.m_fRegionMergeSize;
    inout_stream << nc.m_fDetailMeshSampleDistanceFactor;
    inout_stream << nc.m_fDetailMeshSampleErrorFactor;
  }
}

void ezAiNavigationConfig::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= 2, "Invalid version {0} for ezAiNavigationConfig file", uiVersion);

  ezUInt8 numGroundTypes = 0;
  inout_stream >> numGroundTypes;

  numGroundTypes = ezMath::Min<ezUInt8>(numGroundTypes, ezAiNumGroundTypes);

  for (ezUInt32 i = 0; i < numGroundTypes; ++i)
  {
    auto& gt = m_GroundTypes[i];
    inout_stream >> gt.m_bUsed;
    inout_stream >> gt.m_sName;
    inout_stream >> gt.m_Color;
  }

  ezUInt8 numSearchTypes = 0;
  inout_stream >> numSearchTypes;

  m_PathSearchConfigs.Clear();
  m_PathSearchConfigs.Reserve(numSearchTypes);

  for (ezUInt32 i = 0; i < numSearchTypes; ++i)
  {
    auto& cfg = m_PathSearchConfigs.ExpandAndGetRef();

    inout_stream >> cfg.m_sName;
    inout_stream.ReadArray(cfg.m_fGroundTypeCost).AssertSuccess();
    inout_stream.ReadArray(cfg.m_bGroundTypeAllowed).AssertSuccess();
  }

  ezUInt8 numNavmeshTypes = 0;
  inout_stream >> numNavmeshTypes;

  m_NavmeshConfigs.Clear();
  m_NavmeshConfigs.Reserve(numNavmeshTypes);

  for (ezUInt32 i = 0; i < numNavmeshTypes; ++i)
  {
    auto& nc = m_NavmeshConfigs.ExpandAndGetRef();

    inout_stream >> nc.m_sName;

    if (uiVersion >= 2)
    {
      inout_stream >> nc.m_uiNumSectorsX;
      inout_stream >> nc.m_uiNumSectorsY;
      inout_stream >> nc.m_fSectorSize;
    }

    inout_stream >> nc.m_uiCollisionLayer;
    inout_stream >> nc.m_fCellSize;
    inout_stream >> nc.m_fCellHeight;
    inout_stream >> nc.m_fAgentRadius;
    inout_stream >> nc.m_fAgentHeight;
    inout_stream >> nc.m_fAgentStepHeight;
    inout_stream >> nc.m_WalkableSlope;

    inout_stream >> nc.m_fMaxEdgeLength;
    inout_stream >> nc.m_fMaxSimplificationError;
    inout_stream >> nc.m_fMinRegionSize;
    inout_stream >> nc.m_fRegionMergeSize;
    inout_stream >> nc.m_fDetailMeshSampleDistanceFactor;
    inout_stream >> nc.m_fDetailMeshSampleErrorFactor;
  }
}

ezAiPathSearchConfig::ezAiPathSearchConfig()
{
  for (ezUInt32 i = 0; i < ezAiNumGroundTypes; ++i)
  {
    m_fGroundTypeCost[i] = 1.0f;
    m_bGroundTypeAllowed[i] = true;
  }
}
