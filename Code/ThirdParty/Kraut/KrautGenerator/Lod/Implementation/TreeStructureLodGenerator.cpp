#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/LodDesc.h>
#include <KrautGenerator/Description/TreeStructureDesc.h>
#include <KrautGenerator/Lod/TreeStructureLod.h>
#include <KrautGenerator/Lod/TreeStructureLodGenerator.h>
#include <KrautGenerator/Mesh/Mesh.h>
#include <KrautGenerator/TreeStructure/TreeStructure.h>

namespace Kraut
{
  void GenerateVertexRing(VertexRing& out_VertexRing, const Kraut::SpawnNodeDesc& spawnDesc, const Kraut::BranchNode* pPrevNode, const Kraut::BranchNode* pCurNode, const Kraut::BranchNode* pNextNode, float fVertexRingDetail, float fPosAlongBranch, const aeVec3& vNormalAnchor, aeUInt32 uiVertices);

  void GenerateVertexRing(VertexRing& out_VertexRing, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::BranchStructure& branchStructure, aeInt32 iPrevNodeIdx, aeInt32 iCurNodeIdx, aeInt32 iNextNodeIdx, float fVertexRingDetail, const aeVec3& vNormalAnchor);

  void TreeStructureLodGenerator::GenerateTreeStructureLod()
  {
    if (m_pLodDesc == nullptr)
    {
      Kraut::LodDesc lod;
      lod.m_fTipDetail = 0.03f;
      lod.m_fVertexRingDetail = 0.04f;

      GenerateFullDetailLod(*m_pTreeStructureDesc, lod, *m_pTreeStructure);
    }
    else
    {
      GenerateTreeStructureLod(*m_pTreeStructureDesc, *m_pLodDesc, *m_pTreeStructure);
    }
  }

  void TreeStructureLodGenerator::GenerateFullDetailLod(const Kraut::TreeStructureDesc& structureDesc, const Kraut::LodDesc& lodDesc, const Kraut::TreeStructure& structure)
  {
    m_pTreeStructureLod->m_BranchLODs.clear();
    m_pTreeStructureLod->m_BranchLODs.resize(structure.m_BranchStructures.size());

    for (aeUInt32 b = 0; b < structure.m_BranchStructures.size(); ++b)
    {
      GenerateFullDetailLod(m_pTreeStructureLod->m_BranchLODs[b], structureDesc, lodDesc, structure.m_BranchStructures[b]);
    }
  }

  void TreeStructureLodGenerator::GenerateTreeStructureLod(const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::LodDesc& lodDesc, const Kraut::TreeStructure& structure)
  {
    m_pTreeStructureLod->m_BranchLODs.clear();
    m_pTreeStructureLod->m_BranchLODs.resize(structure.m_BranchStructures.size());

    for (aeUInt32 b = 0; b < structure.m_BranchStructures.size(); ++b)
    {
      GenerateBranchLod(treeStructureDesc, lodDesc, structure.m_BranchStructures[b], m_pTreeStructureLod->m_BranchLODs[b]);
    }
  }

  static float GetDistanceToLineSQR(const aeVec3& A, const aeVec3& vLineDir, const aeVec3& P)
  {
    // bring P into the space of the line AB
    const aeVec3 P2 = P - A;

    const float fProjectedLength = vLineDir.Dot(P2);

    const aeVec3 vProjectedPos = fProjectedLength * vLineDir;

    return (vProjectedPos - P2).GetLengthSquared();
  }

  static float GetNodeFlareThickness(const Kraut::SpawnNodeDesc& spawnDesc, const Kraut::BranchStructure& branchStructure, aeUInt32 uiNode)
  {
    if ((spawnDesc.m_uiFlares == 0) || (spawnDesc.m_fFlareWidth <= 1.0f))
      return branchStructure.m_Nodes[uiNode].m_fThickness;

    float fPosAlongBranch = uiNode / (float)(branchStructure.m_Nodes.size() - 1);
    float fFlareCurve = spawnDesc.m_FlareWidthCurve.GetValueAt(fPosAlongBranch);
    float fFlareThickness = (1.0f + fFlareCurve * (spawnDesc.m_fFlareWidth - 1.0f));
    float fRet = branchStructure.m_Nodes[uiNode].m_fThickness * fFlareThickness;
    return fRet;
  }

  void TreeStructureLodGenerator::GenerateBranchLod(const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::LodDesc& lodDesc, const Kraut::BranchStructure& branchStructure, Kraut::BranchStructureLod& branchStructureLod)
  {
    branchStructureLod.m_NodeIDs.clear();
    branchStructureLod.m_NodeIDs.reserve(branchStructure.m_Nodes.size());

    if (branchStructure.m_Type == Kraut::BranchType::None)
      return;

    const Kraut::SpawnNodeDesc& bnd = treeStructureDesc.m_BranchTypes[branchStructure.m_Type];

    if (branchStructure.m_Nodes.size() < 4)
      return;

    const float fMaxCurvatureDistanceSQR = aeMath::Square(lodDesc.m_fCurvatureThreshold * 0.01f);

    aeInt32 iAnchor0 = 0;
    aeInt32 iAnchor1 = 1;

    aeVec3 vDir = (branchStructure.m_Nodes[iAnchor1].m_vPosition - branchStructure.m_Nodes[iAnchor0].m_vPosition).GetNormalized();
    float fMinRadius = aeMath::Min(GetNodeFlareThickness(bnd, branchStructure, iAnchor0), GetNodeFlareThickness(bnd, branchStructure, iAnchor1));
    float fMaxRadius = aeMath::Max(GetNodeFlareThickness(bnd, branchStructure, iAnchor0), GetNodeFlareThickness(bnd, branchStructure, iAnchor1));

    if (branchStructure.m_iUmbrellaBuddyID != -1)
    {
      const auto& buddyLod = m_pTreeStructureLod->m_BranchLODs[branchStructure.m_iUmbrellaBuddyID];

      if (!buddyLod.m_NodeIDs.empty())
      {
        branchStructureLod.m_NodeIDs = buddyLod.m_NodeIDs;
        return;
      }
    }

    branchStructureLod.m_NodeIDs.push_back(iAnchor0);

    const float fSegmentLength = treeStructureDesc.m_BranchTypes[branchStructure.m_Type].m_iSegmentLengthCM / 100.0f;
    float fAccuLength = 0.0f;

    const aeUInt32 uiMaxNodes = branchStructure.m_Nodes.size();
    for (aeUInt32 n = 2; n < uiMaxNodes; ++n)
    {
      const float fAnchorRotation = ((float)iAnchor0 / (float)(uiMaxNodes - 1)) * bnd.m_fFlareRotation;

      //aeVec3 vPos = Branch.m_Nodes[n].m_vPosition;
      //aeVec3 vDirToPos = (vPos - Branch.m_Nodes[iAnchor0].m_vPosition).GetNormalized ();

      fMinRadius = aeMath::Min(fMinRadius, GetNodeFlareThickness(bnd, branchStructure, n));
      fMaxRadius = aeMath::Max(fMaxRadius, GetNodeFlareThickness(bnd, branchStructure, n));

      // do not insert nodes unless the segment length is reached
      // all manually painted branches have a segment length of ~10cm, independent from the specified segment length
      // so this allows to skip nodes to reduce complexity
      if (fAccuLength < fSegmentLength)
      {
        fAccuLength += (branchStructure.m_Nodes[n].m_vPosition - branchStructure.m_Nodes[n - 1].m_vPosition).GetLength();
        continue;
      }

      // if the branch curvature is below the threshold angle
      //if (/*(!Branch.m_Nodes[n-1].m_bHasChildBranches) && */(vDirToPos.Dot (vDir) >= fMaxDeviation))
      {
        // check whether the thickness threshold is reached

        const float fAnchor0Thickness = GetNodeFlareThickness(bnd, branchStructure, iAnchor0);
        const float fAnchor1Thickness = GetNodeFlareThickness(bnd, branchStructure, n);

        // compute the distance between the last anchor and the current node
        float fMaxLength = 0;
        for (aeUInt32 a = iAnchor0 + 1; a <= n; ++a)
          fMaxLength += (branchStructure.m_Nodes[a].m_vPosition - branchStructure.m_Nodes[a - 1].m_vPosition).GetLength();

        bool bFullFillsThicknessReq = true;
        bool bFullFillsCurvatureReq = true;

        const aeVec3 vLineDir = (branchStructure.m_Nodes[n].m_vPosition - branchStructure.m_Nodes[iAnchor0].m_vPosition).GetNormalized();

        // go through all nodes between the last anchor and this node and compute the interpolated thickness
        // because the polygon will "interpolate" the thickness from anchor0 to anchor1, thus we need to know that
        // to check how much the "desired" thickness deviates from the low-poly mesh thickness
        float fPieceLength = 0;
        for (aeUInt32 a = iAnchor0 + 1; a <= n; ++a)
        {
          fPieceLength += (branchStructure.m_Nodes[a].m_vPosition - branchStructure.m_Nodes[a - 1].m_vPosition).GetLength();
          const float fDesiredThickness = aeMath::Lerp(fAnchor0Thickness, fAnchor1Thickness, (fPieceLength / fMaxLength));

          // the interpolated thickness and the desired thickness should match within some threshold
          if (aeMath::Abs(1.0f - (GetNodeFlareThickness(bnd, branchStructure, a) / fDesiredThickness)) > lodDesc.m_fThicknessThreshold)
          {
            bFullFillsThicknessReq = false;
            break;
          }

          const float fFlareRotation = ((float)a / (float)(uiMaxNodes - 1)) * bnd.m_fFlareRotation;
          const float fRotationDiff = (aeMath::Abs(fFlareRotation - fAnchorRotation) / 360.0f) / 50;

          if (GetDistanceToLineSQR(branchStructure.m_Nodes[iAnchor0].m_vPosition, vLineDir, branchStructure.m_Nodes[a].m_vPosition) + fRotationDiff > fMaxCurvatureDistanceSQR)
          {
            bFullFillsCurvatureReq = false;
            break;
          }
        }

        if (bFullFillsThicknessReq && bFullFillsCurvatureReq)
        {
          iAnchor1 = n;
          continue;
        }
      }

      // if this code is reached, some threshold is not reached and a new node must be inserted

      fAccuLength = 0.0f;
      branchStructureLod.m_NodeIDs.push_back(iAnchor1);

      // reset the next direction, and thickness thresholds
      vDir = (branchStructure.m_Nodes[n].m_vPosition - branchStructure.m_Nodes[iAnchor1].m_vPosition).GetNormalized();
      fMinRadius = aeMath::Min(GetNodeFlareThickness(bnd, branchStructure, n), GetNodeFlareThickness(bnd, branchStructure, iAnchor1));
      fMaxRadius = aeMath::Max(GetNodeFlareThickness(bnd, branchStructure, n), GetNodeFlareThickness(bnd, branchStructure, iAnchor1));


      // set the new anchors
      iAnchor0 = iAnchor1;
      iAnchor1 = n;
    }

    branchStructureLod.m_NodeIDs.push_back(iAnchor1);

    if (iAnchor1 != uiMaxNodes - 1)
      branchStructureLod.m_NodeIDs.push_back(uiMaxNodes - 1);

    GrowBranchTip(branchStructureLod, treeStructureDesc, lodDesc, branchStructure);
  }


  void TreeStructureLodGenerator::GenerateFullDetailLod(BranchStructureLod& branchStructureLod, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::LodDesc& lodDesc, const Kraut::BranchStructure& branchStructure)
  {
    branchStructureLod.m_NodeIDs.clear();
    branchStructureLod.m_NodeIDs.reserve(branchStructure.m_Nodes.size());

    if (branchStructure.m_Type == Kraut::BranchType::None)
      return;

    for (aeUInt32 n = 0; n < branchStructure.m_Nodes.size(); ++n)
    {
      branchStructureLod.m_NodeIDs.push_back(n);
    }

    GrowBranchTip(branchStructureLod, treeStructureDesc, lodDesc, branchStructure);
  }

  void TreeStructureLodGenerator::GrowBranchTip(BranchStructureLod& branchStructureLod, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::LodDesc& lodDesc, const Kraut::BranchStructure& branchStructure)
  {
    if (!treeStructureDesc.m_BranchTypes[branchStructure.m_Type].m_bEnable[Kraut::BranchGeometryType::Branch])
      return;

    if (branchStructure.m_Nodes.size() <= 2)
      return;

    const float fRoundness = treeStructureDesc.m_BranchTypes[branchStructure.m_Type].m_fRoundnessFactor;

    if (fRoundness < 0.01f)
      return;

    const aeInt32 iSegments = aeMath::Max(1, (aeInt32)(branchStructure.m_Nodes.back().m_fThickness / lodDesc.m_fTipDetail));
    const float fSegmentLength = branchStructure.m_Nodes.back().m_fThickness / iSegments;

    if (fSegmentLength < 0.001f)
      return;

    const aeVec3 vPos = branchStructure.m_Nodes.back().m_vPosition;
    const float fThickness = branchStructure.m_Nodes.back().m_fThickness;

    const aeUInt32 uiSecondLastNodeIdx = branchStructureLod.m_NodeIDs[branchStructureLod.m_NodeIDs.size() - 2];

    const aeVec3 vDir = (vPos - branchStructure.m_Nodes[uiSecondLastNodeIdx].m_vPosition).GetNormalized() * fSegmentLength;

    if ((!vDir.IsValid()) || (vDir.IsZeroVector(0.00001f)))
      return;

    Kraut::BranchNode n;
    n.m_vPosition = vPos;

    branchStructureLod.m_TipNodes.clear();
    branchStructureLod.m_TipNodes.reserve(iSegments);
    for (aeInt32 i = 0; i < iSegments; ++i)
    {
      n.m_vPosition += vDir;

      const float fFactor = ((i + 1.0f) / iSegments);
      const float fCos = aeMath::Sqrt(1.0f - fFactor * fFactor);

      n.m_fThickness = fCos;
      n.m_fThickness *= fThickness;

      branchStructureLod.m_TipNodes.push_back(n);
    }

    branchStructureLod.m_TipNodes.back().m_fThickness = 0.001f;

    GenerateLodTipTexCoordV(branchStructureLod, treeStructureDesc, branchStructure, lodDesc.m_fVertexRingDetail);
  }

  void TreeStructureLodGenerator::GenerateLodBranchVertexRing(const BranchStructureLod& branchStructureLod, Kraut::VertexRing& out_VertexRing, const Kraut::TreeStructureDesc& structureDesc, const Kraut::BranchStructure& branchStructure, aeUInt32 uiNodeIdx, const aeVec3& vNormalAnchor, float fVertexRingDetail)
  {
    aeInt32 iPrevNode = ((aeInt32)uiNodeIdx) - 1;
    aeInt32 iNextNode = ((aeInt32)uiNodeIdx) + 1;

    iPrevNode = aeMath::Max(iPrevNode, 0);
    iNextNode = aeMath::Min(iNextNode, (aeInt32)(branchStructureLod.m_NodeIDs.size()) - 1);

    Kraut::GenerateVertexRing(out_VertexRing, structureDesc, branchStructure, branchStructureLod.m_NodeIDs[iPrevNode], branchStructureLod.m_NodeIDs[uiNodeIdx], branchStructureLod.m_NodeIDs[iNextNode], fVertexRingDetail, vNormalAnchor);
  }

  void TreeStructureLodGenerator::GenerateLodTipVertexRing(const BranchStructureLod& branchStructureLod, Kraut::VertexRing& out_VertexRing, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::BranchStructure& branchStructure, aeUInt32 uiNodeIdx, const aeVec3& vNormalAnchor, aeUInt32 uiRingVertices)
  {
    aeInt32 iPrevNode = ((aeInt32)uiNodeIdx) - 1;
    aeInt32 iNextNode = ((aeInt32)uiNodeIdx) + 1;

    iNextNode = aeMath::Min(iNextNode, (aeInt32)(branchStructureLod.m_TipNodes.size()) - 1);

    const Kraut::BranchNode* pPrevNode = nullptr;

    if (iPrevNode < 0)
      pPrevNode = &branchStructure.m_Nodes[branchStructureLod.m_NodeIDs.back()];
    else
      pPrevNode = &branchStructureLod.m_TipNodes[iPrevNode];

    const Kraut::BranchNode* pCurNode = &branchStructureLod.m_TipNodes[uiNodeIdx];
    const Kraut::BranchNode* pNextNode = &branchStructureLod.m_TipNodes[iNextNode];

    const Kraut::SpawnNodeDesc& spawnDesc = treeStructureDesc.m_BranchTypes[branchStructure.m_Type];

    Kraut::GenerateVertexRing(out_VertexRing, spawnDesc, pPrevNode, pCurNode, pNextNode, 0.0f, 1.0f, vNormalAnchor, uiRingVertices);
  }

  void TreeStructureLodGenerator::GenerateLodTipTexCoordV(BranchStructureLod& branchStructureLod, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::BranchStructure& branchStructure, float fVertexRingDetail)
  {
    Kraut::VertexRing VertexRing;

    const aeVec3 vNormalAnchor = branchStructure.m_Nodes.back().m_vPosition;
    float fPrevTexCoordV = branchStructure.m_Nodes.back().m_fTexCoordV;
    aeVec3 vLastPos = vNormalAnchor;

    float fLastBranchDiameter = branchStructure.m_fLastDiameter;
    AE_CHECK_DEV(fLastBranchDiameter > 0.0f, "Can't compute LOD V texcoord before doing this on the full branch.");

    for (aeUInt32 n = 0; n < branchStructureLod.m_TipNodes.size(); ++n)
    {
      GenerateLodTipVertexRing(branchStructureLod, VertexRing, treeStructureDesc, branchStructure, n, vNormalAnchor, 24);

      const float fDist = (vLastPos - branchStructureLod.m_TipNodes[n].m_vPosition).GetLength();

      float fNextTexCoordV = fPrevTexCoordV + (fDist / fLastBranchDiameter);

      if (VertexRing.m_fDiameter > 0.1f)
      {
        fLastBranchDiameter = VertexRing.m_fDiameter;
      }

      branchStructureLod.m_TipNodes[n].m_fTexCoordV = fNextTexCoordV;

      fPrevTexCoordV = fNextTexCoordV;
      vLastPos = branchStructureLod.m_TipNodes[n].m_vPosition;
    }
  }

} // namespace Kraut
