#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Strings/String.h>

static constexpr ezUInt32 ezAiNumGroundTypes = 32;

struct EZ_AIPLUGIN_DLL ezAiNavmeshConfig
{
  ezString m_sName;

  ezUInt16 m_uiNumSectorsX = 64;
  ezUInt16 m_uiNumSectorsY = 64;

  float m_fSectorSize = 32.0f;

  /// The physics collision layer to use for building this navmesh (retrieving the physics geometry).
  ezUInt8 m_uiCollisionLayer = 0;

  float m_fCellSize = 0.2f;
  float m_fCellHeight = 0.2f;

  float m_fAgentRadius = 0.2f;
  float m_fAgentHeight = 1.5f;
  float m_fAgentStepHeight = 0.6f;

  ezAngle m_WalkableSlope = ezAngle::MakeFromDegree(45);

  float m_fMaxEdgeLength = 4.0f;
  float m_fMaxSimplificationError = 1.3f;
  float m_fMinRegionSize = 0.5f;
  float m_fRegionMergeSize = 5.0f;
  float m_fDetailMeshSampleDistanceFactor = 1.0f;
  float m_fDetailMeshSampleErrorFactor = 1.0f;
};

struct EZ_AIPLUGIN_DLL ezAiPathSearchConfig
{
  ezAiPathSearchConfig();

  ezString m_sName;
  float m_fGroundTypeCost[ezAiNumGroundTypes];   // = 1.0f
  bool m_bGroundTypeAllowed[ezAiNumGroundTypes]; // = true
};

struct EZ_AIPLUGIN_DLL ezAiNavigationConfig
{
  ezAiNavigationConfig();

  struct GroundType
  {
    bool m_bUsed = false;
    ezString m_sName;
    ezColorGammaUB m_Color;
  };

  GroundType m_GroundTypes[ezAiNumGroundTypes];

  ezDynamicArray<ezAiPathSearchConfig> m_PathSearchConfigs;
  ezDynamicArray<ezAiNavmeshConfig> m_NavmeshConfigs;

  static constexpr const ezStringView s_sConfigFile = ":project/RuntimeConfigs/AiPluginConfig.cfg"_ezsv;

  ezResult Save(ezStringView sFile = s_sConfigFile) const;
  ezResult Load(ezStringView sFile = s_sConfigFile);

  void Save(ezStreamWriter& inout_stream) const;
  void Load(ezStreamReader& inout_stream);
};
