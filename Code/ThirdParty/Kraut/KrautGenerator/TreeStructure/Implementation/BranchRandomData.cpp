#include <KrautGenerator/PCH.h>

#include <KrautGenerator/TreeStructure/BranchRandomData.h>

namespace Kraut
{
  BranchNodeRandomData BranchRandomData::GetBranchNodeRD()
  {
    BranchNodeRandomData rd;
    rd.m_SpawnNodesRD = m_BranchNodeRD.GetRandomNumber();
    rd.m_NodePlacementRD.m_uiSeedValue = m_BranchNodeRD.GetRandomNumber();
    rd.m_BranchCandidateRD.m_uiSeedValue = m_BranchNodeRD.GetRandomNumber();
    rd.m_BranchRD.m_uiSeedValue = m_BranchNodeRD.GetRandomNumber();
    return rd;
  }

  BranchRandomData BranchNodeRandomData::GetBranchRD()
  {
    BranchRandomData rd;
    rd.m_AngleDeviationRD = m_BranchRD.GetRandomNumber();
    rd.m_BranchNodeRD.m_uiSeedValue = m_BranchRD.GetRandomNumber();
    rd.m_ColorVariation.m_uiSeedValue = m_BranchRD.GetRandomNumber();
    rd.m_GrowDirChange.m_uiSeedValue = m_BranchRD.GetRandomNumber();
    rd.m_LengthRD = m_BranchRD.GetRandomNumber();
    rd.m_RotationalDeviationRD = m_BranchRD.GetRandomNumber();
    rd.m_ThicknessRD = m_BranchRD.GetRandomNumber();
    rd.m_CurGrowTargetDirRD.m_uiSeedValue = m_BranchRD.GetRandomNumber();
    rd.m_TargetDirRD = m_BranchRD.GetRandomNumber();
    rd.m_uiLeafDeviationRD = m_BranchRD.GetRandomNumber();
    rd.m_FrondColorVariationRD = m_BranchRD.GetRandomNumber();

    return rd;
  }

} // namespace Kraut
