#pragma once

#include <KrautGenerator/Infrastructure/RandomNumberGenerator.h>

namespace Kraut
{
  struct BranchRandomData;

  struct KRAUT_DLL BranchNodeRandomData
  {
    aeUInt32 m_SpawnNodesRD = 0;
    Kraut::RandomNumberGenerator m_NodePlacementRD;
    Kraut::RandomNumberGenerator m_BranchCandidateRD;
    Kraut::RandomNumberGenerator m_BranchRD;

    BranchRandomData GetBranchRD();
  };

  struct KRAUT_DLL BranchRandomData
  {
    aeUInt32 m_LengthRD = 0;
    aeUInt32 m_ThicknessRD = 0;
    aeUInt32 m_AngleDeviationRD = 0;
    aeUInt32 m_RotationalDeviationRD = 0;
    aeUInt32 m_FrondColorVariationRD = 0;
    Kraut::RandomNumberGenerator m_ColorVariation;
    Kraut::RandomNumberGenerator m_GrowDirChange;
    Kraut::RandomNumberGenerator m_BranchNodeRD;
    aeUInt32 m_TargetDirRD = 0;
    Kraut::RandomNumberGenerator m_CurGrowTargetDirRD;
    aeUInt32 m_uiLeafDeviationRD = 0;

    BranchNodeRandomData GetBranchNodeRD();
  };

} // namespace Kraut
