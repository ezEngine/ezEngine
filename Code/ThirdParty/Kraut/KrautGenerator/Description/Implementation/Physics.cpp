#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/Physics.h>
#include <KrautGenerator/Description/TreeStructureDesc.h>
#include <KrautGenerator/TreeStructure/BranchStructure.h>
#include <KrautGenerator/TreeStructure/TreeStructure.h>

namespace Kraut
{
  Physics::Physics() = default;
  Physics::~Physics() = default;

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

  void Physics_EmptyImpl::Reset()
  {
  }

  bool Physics_EmptyImpl::IsLineObstructed(const aeVec3& vPos, const aeVec3& vPosTo, float& out_fDistance) const
  {
    return false;
  }

  aeVec3 Physics_EmptyImpl::FindLeastObstructedDirection(const aeVec3& vPos, const aeVec3& vPosTo, aeUInt32 uiMaxDeviation, float& out_fDistance) const
  {
    out_fDistance = 1000.0f;
    return (vPosTo - vPos).GetNormalized();
  }

  void Physics_EmptyImpl::CreateBranchCollisionCapsules(const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::TreeStructure& treeStructure, aeUInt32 uiBranch)
  {
    return;
  }

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

  aeVec3 Physics_BaseImpl::FindLeastObstructedDirection(const aeVec3& vPos, const aeVec3& vPosTo, aeUInt32 uiMaxDeviation, float& out_fDistance) const
  {
    const float fLineLength = (vPosTo - vPos).GetLength();
    const aeVec3 vRayDir = (vPosTo - vPos).GetNormalized();

    out_fDistance = fLineLength;

    float fBestFraction = 0.0f;
    aeVec3 vBestDir = vRayDir;

    const aeVec3 vAngleOrtho = vRayDir.GetOrthogonalVector().GetNormalized();

    for (aeUInt32 angle = 1; angle < uiMaxDeviation; angle += 5)
    {
      aeMatrix mAngle;
      mAngle.SetRotationMatrix(vAngleOrtho, (float)angle);

      for (aeUInt32 round = 0; round < 360; round += 20)
      {
        aeMatrix mRound;
        mRound.SetRotationMatrix(vRayDir, (float)round);

        const aeVec3 vNewDir = mRound * (mAngle * vRayDir);

        const aeVec3 vTarget = vPos + vNewDir * fLineLength;

        float fFraction = 0;
        if (!IsLineObstructed(vPos, vTarget, fFraction))
          return vNewDir;

        if (fFraction > fBestFraction)
        {
          fBestFraction = fFraction;
          vBestDir = vNewDir;

          out_fDistance = fFraction * fLineLength;
        }
      }
    }

    return vBestDir;
  }

  void Physics_BaseImpl::CreateBranchCollisionCapsules(const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::TreeStructure& treeStructure, aeUInt32 uiBranch)
  {
    const Kraut::BranchStructure& branchStructure = treeStructure.m_BranchStructures[uiBranch];

    // if this type of branch should not act as an obstacle, do not insert collision shapes
    if (!treeStructureDesc.m_BranchTypes[branchStructure.m_Type].m_bActAsObstacle)
      return;

    if (branchStructure.m_Nodes.size() < 4)
      return;

    const float fMaxDeviation = aeMath::CosDeg(7.0f);

    aeInt32 iAnchor0 = 0;
    aeInt32 iAnchor1 = 1;

    aeVec3 vDir = (branchStructure.m_Nodes[iAnchor1].m_vPosition - branchStructure.m_Nodes[iAnchor0].m_vPosition).GetNormalized();
    float fMinRadius = aeMath::Min(branchStructure.m_Nodes[iAnchor0].m_fThickness, branchStructure.m_Nodes[iAnchor1].m_fThickness) / 2.0f;
    float fMaxRadius = aeMath::Max(branchStructure.m_Nodes[iAnchor0].m_fThickness, branchStructure.m_Nodes[iAnchor1].m_fThickness) / 2.0f;

    for (aeUInt32 n = 2; n < branchStructure.m_Nodes.size(); ++n)
    {
      aeVec3 vPos = branchStructure.m_Nodes[n].m_vPosition;
      aeVec3 vDirToPos = (vPos - branchStructure.m_Nodes[iAnchor0].m_vPosition).GetNormalized();

      fMinRadius = aeMath::Min(fMinRadius, branchStructure.m_Nodes[n].m_fThickness / 2.0f);
      fMaxRadius = aeMath::Max(fMaxRadius, branchStructure.m_Nodes[n].m_fThickness / 2.0f);

      if ((vDirToPos.Dot(vDir) >= fMaxDeviation) && (fMaxRadius / fMinRadius < 2.0f))
      {
        iAnchor1 = n;
        continue;
      }

      AddColliderCapsule(branchStructure.m_Nodes[iAnchor1].m_vPosition, branchStructure.m_Nodes[iAnchor0].m_vPosition, 1.5f * (fMinRadius + (fMaxRadius - fMinRadius) * 0.5f));

      iAnchor0 = n - 1;
      iAnchor1 = n;

      vDir = (branchStructure.m_Nodes[iAnchor1].m_vPosition - branchStructure.m_Nodes[iAnchor0].m_vPosition).GetNormalized();
      fMinRadius = aeMath::Min(branchStructure.m_Nodes[iAnchor0].m_fThickness, branchStructure.m_Nodes[iAnchor1].m_fThickness) / 2.0f;
      fMaxRadius = aeMath::Max(branchStructure.m_Nodes[iAnchor0].m_fThickness, branchStructure.m_Nodes[iAnchor1].m_fThickness) / 2.0f;
    }

    AddColliderCapsule(branchStructure.m_Nodes[iAnchor1].m_vPosition, branchStructure.m_Nodes[iAnchor0].m_vPosition, 1.5f * (fMinRadius + (fMaxRadius - fMinRadius) * 0.5f));
  }

} // namespace Kraut
