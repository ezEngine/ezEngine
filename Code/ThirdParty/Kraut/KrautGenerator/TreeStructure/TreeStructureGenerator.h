#pragma once

#include <KrautGenerator/KrautGeneratorDLL.h>

#include <KrautFoundation/Math/Vec3.h>
#include <KrautGenerator/Infrastructure/RandomNumberGenerator.h>
#include <KrautGenerator/TreeStructure/BranchRandomData.h>
#include <KrautGenerator/Description/DescriptionEnums.h>

namespace Kraut
{
  using namespace AE_NS_FOUNDATION;

  struct BranchStats;
  struct Physics;
  struct TreeStructure;
  struct TreeStructureDesc;
  struct SpawnNodeDesc;
  struct BranchNodeRandomData;
  struct BranchStructure;

  aeVec3 CreateRandomVector(Kraut::RandomNumberGenerator& rng, const aeVec3& vNormal, aeUInt8 iMaxAlpha);

  class KRAUT_DLL TreeStructureGenerator
  {
  public:
    TreeStructureGenerator();
    ~TreeStructureGenerator();

    const Kraut::TreeStructureDesc* m_pTreeStructureDesc = nullptr;
    Kraut::TreeStructure* m_pTreeStructure = nullptr;
    Kraut::Physics* m_pPhysics = nullptr;

    void GenerateTreeStructure();
    void GenerateTreeStructure(aeUInt32 uiRandomSeed);

  private:
    aeUInt32 m_uiRandomSeed = 0;
    Kraut::Physics* m_pInternalPhysics = nullptr;
    Kraut::RandomNumberGenerator m_RNG;

    aeInt32 GrowBranch(const Kraut::BranchStats& branchStats, Kraut::Physics& physics);

    aeInt32 InsertOneBranch(aeUInt32 uiParentBranch, aeUInt32 uiStartNode0, const Kraut::BranchStats& desc, float fBranchDistance, float fRotation, const Kraut::SpawnNodeDesc& NodeDesc, Kraut::BranchRandomData& BranchRD, bool bReverse);

    void InsertBranchNode(aeUInt32 uiParentBranch, aeUInt32 uiStartNode, const Kraut::SpawnNodeDesc& desc, Kraut::BranchNodeRandomData& NodeRD, float fDistAtStartNode, float fBranchlessPartEnd, bool bReverse);

    void SpawnSubBranches_Reverse(aeUInt32 uiParentBranch, const Kraut::SpawnNodeDesc& desc1, const Kraut::SpawnNodeDesc& desc2, const Kraut::SpawnNodeDesc& desc3);

    const aeVec3 ComputeAverageInfluence(const aeVec3& vPosition, aeUInt32 uiBranchType) const;

    Kraut::BranchStats CreateBranchDesc(const Kraut::SpawnNodeDesc& spawnDesc, const Kraut::BranchRandomData& rd, float fMaxAbsRotationalDeviation = 180.0f);

    aeVec3 ComputeLeafUpDirection(const Kraut::BranchStructure& branchStructure, const Kraut::SpawnNodeDesc& spawnDesc, const aeVec3& vGrowDirection);

    static float GetGrowDir2Distance(Kraut::BranchTargetDir2Usage::Enum Usage, float fBranchLength, float fUseDist);

    aeVec3 GetTargetDir(Kraut::BranchTargetDir::Enum dir, const aeVec3& vParentDir, float fMaxDeviation, const aeVec3& vRotationAxis, const aeVec3& vOwnStartDir, bool bRelativeToParent);

    static bool SkipNodes_Reverse(aeInt32& uiNode, float& fDistance, const Kraut::BranchStructure& branchStructure, float fSkipDistance, float fMaxDistance);
  };

} // namespace Kraut
