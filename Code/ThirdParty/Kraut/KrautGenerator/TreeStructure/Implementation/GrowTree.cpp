#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/Physics.h>
#include <KrautGenerator/Description/SpawnNodeDesc.h>
#include <KrautGenerator/Description/TreeStructureDesc.h>
#include <KrautGenerator/TreeStructure/BranchStats.h>
#include <KrautGenerator/TreeStructure/BranchStructure.h>
#include <KrautGenerator/TreeStructure/TreeStructure.h>
#include <KrautGenerator/TreeStructure/TreeStructureGenerator.h>

namespace Kraut
{
  aeVec3 CreateRandomVector(RandomNumberGenerator& rng, const aeVec3& vNormal, aeUInt8 iMaxAlpha)
  {
    if (iMaxAlpha == 0)
    {
      return vNormal;
    }

    aeVec3 vUp = aeVec3::GetAxisY();

    if (aeMath::Abs(vNormal.Dot(aeVec3::GetAxisY())) > 0.7f) //45°
      vUp = aeVec3::GetAxisX();


    const aeVec3 vOrtho = vNormal.Cross(vUp); //.GetNormalized ();

    const float fr1 = (float)rng.Rand(iMaxAlpha);
    const float fr2 = (float)rng.Rand(360);

    aeQuaternion qR1, qR2;
    qR1.CreateQuaternion(vOrtho, fr1);
    qR2.CreateQuaternion(vNormal, fr2);

    return qR2 * (qR1 * vNormal);
  }

  //////////////////////////////////////////////////////////////////////////

  TreeStructureGenerator::TreeStructureGenerator() = default;
  TreeStructureGenerator::~TreeStructureGenerator() = default;

  void TreeStructureGenerator::GenerateTreeStructure()
  {
    GenerateTreeStructure(m_pTreeStructureDesc->m_uiRandomSeed);
  }

  void TreeStructureGenerator::GenerateTreeStructure(aeUInt32 uiRandomSeed)
  {
    m_uiRandomSeed = uiRandomSeed;
    m_pInternalPhysics = m_pPhysics;

    Physics_EmptyImpl noPhysics;
    if (m_pInternalPhysics == nullptr)
    {
      m_pInternalPhysics = &noPhysics;
    }

    m_RNG.m_uiSeedValue = m_uiRandomSeed;

    if (m_pTreeStructureDesc->m_bGrowProceduralTrunks)
    {
      // for each trunk type
      for (aeUInt32 trunks = 0; trunks < 3; ++trunks)
      {
        const Kraut::SpawnNodeDesc& spawnDesc = m_pTreeStructureDesc->m_BranchTypes[Kraut::BranchType::Trunk1 + trunks];

        if (!spawnDesc.m_bUsed)
          continue;

        Kraut::BranchNodeRandomData NodeRD;
        NodeRD.m_BranchRD.m_uiSeedValue = m_uiRandomSeed + trunks;
        NodeRD.m_SpawnNodesRD = m_uiRandomSeed + trunks;
        m_RNG.m_uiSeedValue = NodeRD.m_SpawnNodesRD;

        // determine how many trunks to spawn (of the current type)
        const aeUInt32 uiTrunks = m_RNG.Rand(spawnDesc.m_uiMinBranches, spawnDesc.m_uiMaxBranches);

        aeInt32 iFirstBranch = -1;

        // for each trunk to spawn
        for (aeUInt32 t = 0; t < uiTrunks; ++t)
        {
          const float fRotAngle = t * (360.0f / uiTrunks);
          aeQuaternion qMainDir;
          qMainDir.CreateQuaternion(aeVec3(0, 1, 0), fRotAngle);

          const float fMinDistanceToStartPos = (uiTrunks > 1) ? aeMath::Max(0.01f, spawnDesc.m_fNodeSpacingBefore) : 0.01f;

          m_RNG.m_uiSeedValue = NodeRD.m_NodePlacementRD.GetRandomNumber();
          const float fDistance = fMinDistanceToStartPos + m_RNG.Rand((aeInt32)(spawnDesc.m_fNodeHeight / 0.05f) + 1) * 0.05f;

          Kraut::BranchStats dTrunk = CreateBranchDesc(spawnDesc, NodeRD.GetBranchRD());

          dTrunk.m_vStartPosition = qMainDir * aeVec3(fDistance, 0, 0);

          m_RNG.m_uiSeedValue = dTrunk.m_RandomData.m_TargetDirRD;

          aeVec3 vBranchAngleAxis;

          {
            const float fRotationalDev = (spawnDesc.m_fMaxRotationalDeviation > 0) ? (m_RNG.Rand(static_cast<aeUInt32>(spawnDesc.m_fMaxRotationalDeviation * 2)) - spawnDesc.m_fMaxRotationalDeviation) : m_RNG.Rand(1);
            aeQuaternion qRotationalDev;
            qRotationalDev.CreateQuaternion(aeVec3(0, 1, 0), fRotationalDev);

            const aeVec3 vDirToPos = qRotationalDev * dTrunk.m_vStartPosition.GetNormalized();
            vBranchAngleAxis = aeVec3(0, 1, 0).Cross(vDirToPos).GetNormalized();
            const float fBranchAngle = (spawnDesc.m_fBranchAngle - 90) + m_RNG.Randf(spawnDesc.m_fMaxBranchAngleDeviation * 2.0f) - spawnDesc.m_fMaxBranchAngleDeviation;
            aeQuaternion qBranchAngle;
            qBranchAngle.CreateQuaternion(vBranchAngleAxis, fBranchAngle);

            dTrunk.m_vStartDirection = qBranchAngle * aeVec3(0, 1, 0);
          }

          dTrunk.m_vGrowDirection = GetTargetDir(spawnDesc.m_TargetDirection, aeVec3(0, 1, 0), spawnDesc.m_fMaxTargetDirDeviation, vBranchAngleAxis, dTrunk.m_vStartDirection, spawnDesc.m_bTargetDirRelative);
          dTrunk.m_vGrowDirection2 = GetTargetDir(spawnDesc.m_TargetDirection2, aeVec3(0, 1, 0), spawnDesc.m_fMaxTargetDirDeviation, vBranchAngleAxis, dTrunk.m_vStartDirection, spawnDesc.m_bTargetDirRelative);
          dTrunk.m_fGrowDir2UUsageDistance = GetGrowDir2Distance(spawnDesc.m_TargetDir2Uage, dTrunk.m_fBranchLength, spawnDesc.m_fTargetDir2Usage);

          iFirstBranch = GrowBranch(dTrunk, *m_pInternalPhysics);
          if ((spawnDesc.m_BranchTypeMode == Kraut::BranchTypeMode::Umbrella) || (iFirstBranch < 0))
            break;
        }

        if ((iFirstBranch >= 0) && (spawnDesc.m_BranchTypeMode == Kraut::BranchTypeMode::Umbrella))
        {
          float fSpawnAngle = 0.0f;
          const float fSpawnAngleStep = 360.0f / uiTrunks;

          aeInt32 iLastBranchID = iFirstBranch;
          for (aeUInt32 t = 1; t < uiTrunks; ++t)
          {
            Kraut::BranchRandomData BranchRD = NodeRD.GetBranchRD();

            // the next branch will be spawned in another direction
            fSpawnAngle += fSpawnAngleStep;

            iLastBranchID = m_pTreeStructure->DuplicateBranch(iFirstBranch, fSpawnAngle, iLastBranchID);
          }

          m_pTreeStructure->m_BranchStructures[iFirstBranch].m_fUmbrellaBranchRotation = 0.0f;
          m_pTreeStructure->m_BranchStructures[iFirstBranch].m_iUmbrellaBuddyID = iLastBranchID;

          break;
        }
      }
    }

    Kraut::BranchType::Enum UpdateTypes[4][3] = {
      {Kraut::BranchType::Trunk1, Kraut::BranchType::Trunk2, Kraut::BranchType::Trunk3},
      {Kraut::BranchType::MainBranches1, Kraut::BranchType::MainBranches2, Kraut::BranchType::MainBranches3},
      {Kraut::BranchType::SubBranches1, Kraut::BranchType::SubBranches2, Kraut::BranchType::SubBranches3},
      {Kraut::BranchType::Twigs1, Kraut::BranchType::Twigs2, Kraut::BranchType::Twigs3}};
    //    { Kraut::BranchType::SubTwigs1,     Kraut::BranchType::SubTwigs2,     Kraut::BranchType::SubTwigs3     } };

    for (int r = 1; r <= 3; ++r)
    {
      const aeUInt32 uiMaxBranches = m_pTreeStructure->m_BranchStructures.size();
      for (aeUInt32 b = 0; b < uiMaxBranches; ++b)
      {
        const Kraut::BranchType::Enum type = m_pTreeStructure->m_BranchStructures[b].m_Type;

        if ((type != UpdateTypes[r - 1][0]) && (type != UpdateTypes[r - 1][1]) && (type != UpdateTypes[r - 1][2]))
          continue;

        //if (m_pTreeStructure->m_Branches[b]->m_bManuallyCreated)
        //{
        //  Kraut::BranchNodeRandomData NodeRD;
        //  NodeRD.m_BranchRD.m_uiSeedValue = m_uiRandomSeed;

        //  m_pTreeStructure->m_Branches[b]->m_RandomData = NodeRD.GetBranchRD();
        //}

        SpawnSubBranches_Reverse(b, m_pTreeStructureDesc->m_BranchTypes[UpdateTypes[r][0]], m_pTreeStructureDesc->m_BranchTypes[UpdateTypes[r][1]], m_pTreeStructureDesc->m_BranchTypes[UpdateTypes[r][2]]);
      }
    }
  }

  aeInt32 TreeStructureGenerator::InsertOneBranch(aeUInt32 uiParentBranch, aeUInt32 uiStartNode0, const Kraut::BranchStats& desc, float fBranchDistance, float fRotation, const Kraut::SpawnNodeDesc& NodeDesc, Kraut::BranchRandomData& BranchRD, bool bReverse)
  {
    aeInt32 uiStartNode = uiStartNode0;

    Kraut::BranchStats bd = desc;

    Kraut::BranchStructure& branchStructure = m_pTreeStructure->m_BranchStructures[uiParentBranch];

    const Kraut::SpawnNodeDesc& ParentDesc = m_pTreeStructureDesc->m_BranchTypes[branchStructure.m_Type];

    float fCurDistance = 0.01f;

    aeInt32 iPrevNode = uiStartNode;

    // find a node that is far enough away to spawn the branch
    while (fCurDistance < fBranchDistance)
    {
      iPrevNode = uiStartNode;

      if (bReverse)
        --uiStartNode;
      else
        ++uiStartNode;

      if ((uiStartNode < 0) || (uiStartNode >= (aeInt32)branchStructure.m_Nodes.size()))
        return -1;

      fCurDistance += (branchStructure.m_Nodes[uiStartNode].m_vPosition - branchStructure.m_Nodes[iPrevNode].m_vPosition).GetLength();
    }

    // now we know at which node to spawn this branch (uiStartNode)

    const float fRealSpawnAngle = fRotation + desc.m_fRotationalDeviation;

    aeVec3 vNodeDirection;
    aeVec3 vStartNodeDirection;
    {
      if (uiStartNode > 0)
        vNodeDirection = (branchStructure.m_Nodes[uiStartNode].m_vPosition - branchStructure.m_Nodes[uiStartNode - 1].m_vPosition).GetNormalized();
      else
        vNodeDirection = (branchStructure.m_Nodes[1].m_vPosition - branchStructure.m_Nodes[0].m_vPosition).GetNormalized();

      if (uiStartNode0 > 0)
        vStartNodeDirection = (branchStructure.m_Nodes[uiStartNode0].m_vPosition - branchStructure.m_Nodes[uiStartNode0 - 1].m_vPosition).GetNormalized();
      else
        vStartNodeDirection = (branchStructure.m_Nodes[1].m_vPosition - branchStructure.m_Nodes[0].m_vPosition).GetNormalized();
    }

    // determine the exact spawn position for this branch (can be between nodes)
    aeVec3 vBranchSpawnPosition = branchStructure.m_Nodes[uiStartNode].m_vPosition;
    {
      vBranchSpawnPosition -= vNodeDirection * (fCurDistance - fBranchDistance);
    }

    aeVec3 vOrthoDir;

    if (!m_pTreeStructureDesc->m_bLeafCardMode)
    {
      if (aeMath::Abs(vStartNodeDirection.Dot(aeVec3(0, 1, 0))) > 0.999f)
        vOrthoDir = vStartNodeDirection.GetOrthogonalVector();
      else
      {
        vOrthoDir = aeVec3(0, 1, 0).Cross(vStartNodeDirection);
        vOrthoDir = vStartNodeDirection.Cross(vOrthoDir);
      }

      vOrthoDir.Normalize();
    }
    else
      vOrthoDir.SetVector(0, 0, 1);

    const float fBranchAngle = desc.m_fBranchAngle;

    aeQuaternion qBranchAngle, qBranchDir;
    qBranchAngle.CreateQuaternion(vOrthoDir, fBranchAngle);
    qBranchDir.CreateQuaternion(vNodeDirection, fRealSpawnAngle);

    const aeVec3 vBranchDir = qBranchDir * qBranchAngle * vNodeDirection;

    const aeVec3 vRotationAxis = -vBranchDir.Cross(vNodeDirection).GetNormalized();

    // check how long the branch shall be that is spawned at this position
    bd.m_fBranchLength *= NodeDesc.m_MaxBranchLengthParentScale.GetValueAt((float)uiStartNode / (branchStructure.m_Nodes.size() - 1));

    bd.m_vGrowDirection = GetTargetDir(NodeDesc.m_TargetDirection, vNodeDirection, NodeDesc.m_fMaxTargetDirDeviation, vRotationAxis, vBranchDir, NodeDesc.m_bTargetDirRelative);
    bd.m_vGrowDirection2 = GetTargetDir(NodeDesc.m_TargetDirection2, vNodeDirection, NodeDesc.m_fMaxTargetDirDeviation, vRotationAxis, vBranchDir, NodeDesc.m_bTargetDirRelative);
    bd.m_fGrowDir2UUsageDistance = GetGrowDir2Distance(NodeDesc.m_TargetDir2Uage, bd.m_fBranchLength, NodeDesc.m_fTargetDir2Usage);

    bd.m_vStartDirection = vBranchDir;
    bd.m_vStartPosition = vBranchSpawnPosition;
    bd.m_iParentBranchID = uiParentBranch;
    bd.m_iParentBranchNodeID = uiStartNode;

    const aeInt32 iBranchID = GrowBranch(bd, *m_pInternalPhysics);

    if (iBranchID >= 0)
      branchStructure.m_Nodes[uiStartNode].m_bHasChildBranches = true;

    return iBranchID;
  }

  void TreeStructureGenerator::InsertBranchNode(aeUInt32 uiParentBranch, aeUInt32 uiStartNode, const Kraut::SpawnNodeDesc& spawnDesc, Kraut::BranchNodeRandomData& NodeRD, float fDistAtStartNode, float fBranchlessPartEnd, bool bReverse)
  {
    const Kraut::BranchStructure* pParentBranch = &m_pTreeStructure->m_BranchStructures[uiParentBranch];
    const Kraut::SpawnNodeDesc& ParentBND = m_pTreeStructureDesc->m_BranchTypes[pParentBranch->m_Type];
    const float fParentSegmentLength = ParentBND.m_iSegmentLengthCM / 100.0f;

    m_RNG.m_uiSeedValue = NodeRD.m_SpawnNodesRD;
    aeUInt32 uiBranches = m_RNG.Rand(spawnDesc.m_uiMinBranches, spawnDesc.m_uiMaxBranches + 1);

    AE_CHECK_DEV(uiBranches >= spawnDesc.m_uiMinBranches, "Number of Branches to spawn (%i) is smaller than minimum number of branches (%i). This should not happen.", uiBranches, spawnDesc.m_uiMinBranches);

    aeArray<float> BranchPos;
    BranchPos.reserve(uiBranches);
    aeArray<bool> Used;
    Used.reserve(uiBranches);
    for (aeUInt32 b = 0; b < uiBranches; ++b)
      Used.push_back(false);

    m_RNG.m_uiSeedValue = NodeRD.m_NodePlacementRD.GetRandomNumber();

    for (aeUInt32 b = 0; b < uiBranches; ++b)
    {
      aeInt32 iIndex = m_RNG.Rand(uiBranches);
      while (Used[iIndex])
        iIndex = m_RNG.Rand(uiBranches);

      Used[iIndex] = true;

      float fOffset = (spawnDesc.m_fNodeHeight / uiBranches) * 0.5f;
      float fDist = fOffset + (spawnDesc.m_fNodeHeight / uiBranches) * iIndex;

      if (fDistAtStartNode + fDist <= fBranchlessPartEnd)
        BranchPos.push_back(fDist);
    }

    // in which direction to spawn the next branch
    float fSpawnAngle = 0.0f;
    float fSpawnAngleStep = 360.0f / uiBranches;
    const float fMaxRotationalDeviation = 180.0f / BranchPos.size();

    if (m_pTreeStructureDesc->m_bLeafCardMode)
      fSpawnAngleStep = 180.0f;

    if (spawnDesc.m_BranchTypeMode == Kraut::BranchTypeMode::Umbrella)
    {
      Kraut::BranchRandomData BranchRD = NodeRD.GetBranchRD();
      const aeInt32 iFirstBranch = InsertOneBranch(uiParentBranch, uiStartNode, CreateBranchDesc(spawnDesc, BranchRD, fMaxRotationalDeviation), BranchPos[0], fSpawnAngle, spawnDesc, BranchRD, bReverse);

      if (iFirstBranch >= 0)
      {
        aeInt32 iLastBranchID = iFirstBranch;

        for (aeUInt32 b = 1; b < uiBranches; ++b)
        {
          Kraut::BranchRandomData BranchRD = NodeRD.GetBranchRD();

          // the next branch will be spawned in another direction
          fSpawnAngle += fSpawnAngleStep;

          iLastBranchID = m_pTreeStructure->DuplicateBranch(iFirstBranch, fSpawnAngle, iLastBranchID);
        }

        m_pTreeStructure->m_BranchStructures[iFirstBranch].m_fUmbrellaBranchRotation = 0.0f;
        m_pTreeStructure->m_BranchStructures[iFirstBranch].m_iUmbrellaBuddyID = iLastBranchID;
      }
    }
    else
    {
      // go through all branches that should be spawned
      for (aeUInt32 b = 0; b < BranchPos.size(); ++b)
      {
        Kraut::BranchRandomData BranchRD = NodeRD.GetBranchRD();
        InsertOneBranch(uiParentBranch, uiStartNode, CreateBranchDesc(spawnDesc, BranchRD, fMaxRotationalDeviation), BranchPos[b], fSpawnAngle, spawnDesc, BranchRD, bReverse);

        // the next branch will be spawned in another direction
        fSpawnAngle += fSpawnAngleStep;
      }
    }
  }

  void TreeStructureGenerator::SpawnSubBranches_Reverse(aeUInt32 uiParentBranch, const Kraut::SpawnNodeDesc& desc1, const Kraut::SpawnNodeDesc& desc2, const Kraut::SpawnNodeDesc& desc3)
  {
    Kraut::BranchStructure& branchStructure = m_pTreeStructure->m_BranchStructures[uiParentBranch];
    const Kraut::SpawnNodeDesc& ParentDesc = m_pTreeStructureDesc->m_BranchTypes[branchStructure.m_Type];

    const float fBranchlessPart = branchStructure.m_fBranchLength - ParentDesc.m_fBranchlessPartABS;

    aeInt32 uiNode = branchStructure.m_Nodes.size() - 1;
    float fAccumDist = 0.01f;
    aeInt32 uiLastNode = uiNode + 1;

    // skip all nodes that are in the branchless part
    {
      if (!SkipNodes_Reverse(uiNode, fAccumDist, branchStructure, ParentDesc.m_fBranchlessPartEndABS - fAccumDist, fBranchlessPart))
        goto end;
    }

    while (uiNode > 0)
    {
      // don't want to end up in an endless loop
      if (uiLastNode == uiNode)
      {
        --uiNode;

        if ((uiNode >= 0) && (uiNode < (aeInt32)branchStructure.m_Nodes.size() - 1))
          fAccumDist += (branchStructure.m_Nodes[uiNode].m_vPosition - branchStructure.m_Nodes[uiNode + 1].m_vPosition).GetLength();
      }

      uiLastNode = uiNode;

      const Kraut::SpawnNodeDesc* pCurDesc = nullptr;

      Kraut::BranchNodeRandomData NodeRD = branchStructure.m_RandomData.GetBranchNodeRD();

      {
        const aeUInt8 uiCurRelPos = 100 - (aeUInt8)((fAccumDist / branchStructure.m_fBranchLength) * 100.0f);

        const Kraut::SpawnNodeDesc* pCandidates[3];
        aeUInt32 uiCandidates = 0;

        // if a branch is manually created and it wants to grow certain sub-types, then do so
        // if a branch is procedurally created and the description allows a certain sub-type, then grow it
        // however only grow the sub-type, if it is in the specified range of the parent branch
        if ((desc1.m_bUsed && ParentDesc.m_bAllowSubType[0]) && (desc1.m_uiLowerBound < uiCurRelPos) && (uiCurRelPos < desc1.m_uiUpperBound))
          pCandidates[uiCandidates++] = &desc1;
        if ((desc2.m_bUsed && ParentDesc.m_bAllowSubType[1]) && (desc2.m_uiLowerBound < uiCurRelPos) && (uiCurRelPos < desc2.m_uiUpperBound))
          pCandidates[uiCandidates++] = &desc2;
        if ((desc3.m_bUsed && ParentDesc.m_bAllowSubType[2]) && (desc3.m_uiLowerBound < uiCurRelPos) && (uiCurRelPos < desc3.m_uiUpperBound))
          pCandidates[uiCandidates++] = &desc3;

        // if none of the three possible candidates is active in the current branch area,
        // increase the accumulated distance and node counter and skip this node
        if (uiCandidates == 0)
          continue;

        m_RNG.m_uiSeedValue = NodeRD.m_BranchCandidateRD.GetRandomNumber();
        pCurDesc = pCandidates[m_RNG.Rand(uiCandidates)];
      }

      // skip all nodes that are in the 'free space before node' range
      if (!SkipNodes_Reverse(uiNode, fAccumDist, branchStructure, pCurDesc->m_fNodeSpacingAfter, fBranchlessPart))
        goto end;

      AE_CHECK_DEV(uiNode > 0, "1");

      InsertBranchNode(uiParentBranch, uiNode, *pCurDesc, NodeRD, fAccumDist, fBranchlessPart, true);

      // skip all nodes in the 'free space after node' range
      if (!SkipNodes_Reverse(uiNode, fAccumDist, branchStructure, pCurDesc->m_fNodeHeight, fBranchlessPart))
        goto end;
      if (!SkipNodes_Reverse(uiNode, fAccumDist, branchStructure, pCurDesc->m_fNodeSpacingBefore, fBranchlessPart))
        goto end;
    }

    return;

  end:
    SkipNodes_Reverse(uiNode, fAccumDist, branchStructure, 1000, 1000);
  }

  const aeVec3 TreeStructureGenerator::ComputeAverageInfluence(const aeVec3& vPosition, aeUInt32 uiBranchType) const
  {
    aeVec3 res(0.0f);

    for (aeUInt32 i = 0; i < m_pTreeStructureDesc->m_Influences.size(); ++i)
    {
      if ((m_pTreeStructureDesc->m_Influences[i]->m_AffectedBranchTypes & (1 << uiBranchType)) == 0)
        continue;

      res += m_pTreeStructureDesc->m_Influences[i]->ComputeInfluence(vPosition);
    }

    return res;
  }

  Kraut::BranchStats TreeStructureGenerator::CreateBranchDesc(const Kraut::SpawnNodeDesc& spawnDesc, const Kraut::BranchRandomData& rd, float fMaxAbsRotationalDeviation /*= 180.0f*/)
  {
    Kraut::BranchStats d;
    d.m_RandomData = rd;
    d.m_Type = spawnDesc.m_Type;
    m_RNG.m_uiSeedValue = rd.m_LengthRD;

    d.m_fBranchLength = m_RNG.Rand(spawnDesc.m_uiMinBranchLengthInCM, spawnDesc.m_uiMaxBranchLengthInCM) / 100.0f;

    m_RNG.m_uiSeedValue = rd.m_ThicknessRD;
    //d.m_fBranchThickness = aeMath::Max (0.01f, m_fMinBranchThickness);
    d.m_fBranchThickness = m_RNG.Rand(spawnDesc.m_uiMinBranchThicknessInCM, spawnDesc.m_uiMaxBranchThicknessInCM + 1) / 100.0f;

    m_RNG.m_uiSeedValue = rd.m_AngleDeviationRD;
    do
    {
      d.m_fBranchAngle = spawnDesc.m_fBranchAngle + m_RNG.Randf(spawnDesc.m_fMaxBranchAngleDeviation * 2.0f) - spawnDesc.m_fMaxBranchAngleDeviation;
    } while ((d.m_fBranchAngle < 1.0f) || (d.m_fBranchAngle > 179.0f));

    fMaxAbsRotationalDeviation = aeMath::Min(fMaxAbsRotationalDeviation, spawnDesc.m_fMaxRotationalDeviation);

    m_RNG.m_uiSeedValue = rd.m_RotationalDeviationRD;
    d.m_fRotationalDeviation = m_RNG.Randf(fMaxAbsRotationalDeviation * 2.0f) - fMaxAbsRotationalDeviation;

    m_RNG.m_uiSeedValue = rd.m_FrondColorVariationRD;
    d.m_fFrondColorVariation = m_RNG.Randf(1.0f);
    d.m_uiFrondTextureVariation = m_RNG.Rand(255); // modulo will be done later

    return d;
  }

  aeVec3 TreeStructureGenerator::ComputeLeafUpDirection(const Kraut::BranchStructure& branchStructure, const Kraut::SpawnNodeDesc& spawnDesc, const aeVec3& vGrowDirection)
  {
    // if this branch has a parent branch, check its direction where the leaf is spawned
    aeVec3 vParentDirection(0, 1, 0);
    if (branchStructure.m_iParentBranchID >= 0)
    {
      aeInt32 iNodeID = branchStructure.m_uiParentBranchNodeID;

      if (iNodeID == 0)
        iNodeID = 1;

      const aeVec3 vPos1 = m_pTreeStructure->m_BranchStructures[branchStructure.m_iParentBranchID].m_Nodes[iNodeID].m_vPosition;
      const aeVec3 vPos0 = m_pTreeStructure->m_BranchStructures[branchStructure.m_iParentBranchID].m_Nodes[iNodeID - 1].m_vPosition;

      vParentDirection = (vPos1 - vPos0).GetNormalized();
    }

    const aeVec3 vGrowDir = vGrowDirection.GetNormalized();

    // find out which direction is supposed to be 'up' for this leaf
    aeVec3 vRotationalDir(0, 1, 0);

    switch (spawnDesc.m_FrondUpOrientation)
    {
      case Kraut::LeafOrientation::Upwards:
        vRotationalDir.SetVector(0, 1, 0);
        break;
      case Kraut::LeafOrientation::AlongBranch:
        vRotationalDir = vParentDirection;
        break;
      case Kraut::LeafOrientation::OrthogonalToBranch:
        if (aeMath::Abs(vGrowDir.Dot(vParentDirection)) > aeMath::CosDeg(1.0f))
          vRotationalDir = vGrowDir.GetOrthogonalVector().GetNormalized();
        else
          vRotationalDir = vGrowDir.Cross(vParentDirection).GetNormalized();
        break;
    }

    AE_CHECK_DEV(vRotationalDir.IsValid(), "Leaf Up Direction is degenerate.");

    // if the leaf-up dir and the grow dir are too close (1°), just get some random (orthogonal) vector as up-vector
    if (aeMath::Abs(vGrowDir.Dot(vRotationalDir)) > aeMath::CosDeg(1.0f))
      vRotationalDir = vGrowDir.GetOrthogonalVector().GetNormalized();

    AE_CHECK_DEV(vRotationalDir.IsValid(), "Leaf Up Direction is degenerate.");

    if (spawnDesc.m_uiMaxFrondOrientationDeviation > 0)
    {
      m_RNG.m_uiSeedValue = branchStructure.m_RandomData.m_uiLeafDeviationRD;

      aeInt32 uiRotation = (aeInt32)m_RNG.Rand(spawnDesc.m_uiMaxFrondOrientationDeviation * 2 + 1) - (aeInt32)spawnDesc.m_uiMaxFrondOrientationDeviation;

      aeQuaternion q;
      q.CreateQuaternion(vGrowDir, (float)uiRotation);

      vRotationalDir = q * vRotationalDir;
    }

    AE_CHECK_DEV(vRotationalDir.IsValid(), "Leaf Up Direction is degenerate.");

    return vRotationalDir;
  }

} // namespace Kraut
