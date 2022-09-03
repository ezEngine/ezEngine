#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/Physics.h>
#include <KrautGenerator/Description/TreeStructureDesc.h>
#include <KrautGenerator/Mesh/Mesh.h>
#include <KrautGenerator/TreeStructure/BranchStructure.h>
#include <KrautGenerator/TreeStructure/TreeStructure.h>
#include <KrautGenerator/TreeStructure/TreeStructureGenerator.h>

namespace Kraut
{
  aeInt32 TreeStructureGenerator::GrowBranch(const Kraut::BranchStats& branchStats, Kraut::Physics& physics)
  {
    const aeUInt32 uiBranchID = m_pTreeStructure->m_BranchStructures.size();
    m_pTreeStructure->m_BranchStructures.push_back();

    Kraut::BranchStructure& branchStructure = m_pTreeStructure->m_BranchStructures[uiBranchID];

    branchStructure.m_RandomData = branchStats.m_RandomData;

    const Kraut::SpawnNodeDesc& BranchTypeDesc = m_pTreeStructureDesc->m_BranchTypes[branchStats.m_Type];

    branchStructure.m_Type = branchStats.m_Type;
    branchStructure.m_fThickness = branchStats.m_fBranchThickness;
    branchStructure.m_fFrondColorVariation = branchStats.m_fFrondColorVariation;
    branchStructure.m_uiFrondTextureVariation = branchStats.m_uiFrondTextureVariation;
    branchStructure.m_iParentBranchID = branchStats.m_iParentBranchID;
    branchStructure.m_uiParentBranchNodeID = branchStats.m_iParentBranchNodeID;

    aeVec3 vCurGrowDirection = branchStats.m_vStartDirection;
    aeVec3 vCurGrowTargetDir = branchStats.m_vGrowDirection;
    aeVec3 vCurNodePos = branchStats.m_vStartPosition;
    branchStructure.m_fBranchLength = 0.0f;

    if (m_pTreeStructureDesc->m_bLeafCardMode)
    {
      // in leaf card mode, always orient all leaves towards the camera
      branchStructure.m_vLeafUpDirection.SetVector(0, 0, 1);
    }
    else
    {
      // if this branch is a leaf, only allow it to grow in a fixed plane
      // it needs an 'up' vector for this
      branchStructure.m_vLeafUpDirection = ComputeLeafUpDirection(branchStructure, BranchTypeDesc, vCurGrowDirection);
    }

    aeVec3 vLeafPlaneNormal;

    if (aeMath::Abs(branchStats.m_vStartDirection.Dot(branchStats.m_vGrowDirection)) > 0.999f)
    {
      vLeafPlaneNormal = branchStats.m_vStartDirection.GetOrthogonalVector();
    }
    else
      vLeafPlaneNormal = branchStats.m_vStartDirection.Cross(branchStats.m_vGrowDirection);

    // if this branch is a leaf, it may only grow in this plane and not deviate from it
    const aePlane LeafPlane(vLeafPlaneNormal.GetNormalized(), aeVec3::ZeroVector());

    AE_CHECK_DEV(LeafPlane.m_vNormal.IsValid(), "Leaf Plane is degenerate.");

    bool bChangeDir = true;

    const float fMaxDirectionChange = (BranchTypeDesc.m_fGrowMaxDirChangePerSegment / 10.0f) * BranchTypeDesc.m_iSegmentLengthCM;

    aeVec3 vTargetDir1 = branchStats.m_vGrowDirection;
    aeVec3 vTargetDir2 = branchStats.m_vGrowDirection2;

    // keep on growing until the target length is reached
    while (branchStructure.m_fBranchLength < branchStats.m_fBranchLength)
    {
      Kraut::BranchNode n;
      n.m_vPosition = vCurNodePos;

      AE_CHECK_DEV(n.m_vPosition.IsValid(), "GrowBranch: Skeleton Position is degenerate.");

      branchStructure.m_Nodes.push_back(n);

      if ((bChangeDir) || (aeMath::ACosDeg(vCurGrowDirection.Dot(vCurGrowTargetDir)) < fMaxDirectionChange))
      {
        m_RNG.m_uiSeedValue = branchStructure.m_RandomData.m_CurGrowTargetDirRD.GetRandomNumber();

        if (branchStructure.m_fBranchLength >= branchStats.m_fGrowDir2UUsageDistance)
          vCurGrowTargetDir = Kraut::CreateRandomVector(m_RNG, vTargetDir2, (aeUInt8)BranchTypeDesc.m_fGrowMaxTargetDirDeviation);
        else
          vCurGrowTargetDir = Kraut::CreateRandomVector(m_RNG, vTargetDir1, (aeUInt8)BranchTypeDesc.m_fGrowMaxTargetDirDeviation);

        bChangeDir = false;

        if (BranchTypeDesc.m_bDoPhysicalSimulation)
        {
          const float fRemainingBranchLength = branchStats.m_fBranchLength - branchStructure.m_fBranchLength;
          const float fRayCastLength = aeMath::Min(BranchTypeDesc.m_fPhysicsLookAhead, fRemainingBranchLength);
          const aeVec3 vRayCastTarget = vCurNodePos + vCurGrowTargetDir * fRayCastLength;

          float fDist;
          if (physics.IsLineObstructed(vCurNodePos, vRayCastTarget, fDist))
          {
            vCurGrowTargetDir = physics.FindLeastObstructedDirection(vCurNodePos, vRayCastTarget, (aeUInt32)BranchTypeDesc.m_fPhysicsEvasionAngle, fDist);
          }
        }

        // if this branch is a leaf, restrict its growth to the leaf plane
        if (BranchTypeDesc.m_bRestrictGrowthToFrondPlane)
          vCurGrowTargetDir = LeafPlane.ProjectOntoPlane(vCurGrowTargetDir).GetNormalized();
      }

      aeQuaternion qSegmentRotation;
      qSegmentRotation.CreateQuaternion(vCurGrowDirection, vCurGrowTargetDir);
      aeVec3 vAxis = qSegmentRotation.GetRotationAxis();

      AE_CHECK_DEV(vAxis.IsValid(), "Rotation Axis is degenerate.");

      // if this branch is a leaf, restrict its growth to the leaf plane
      if (BranchTypeDesc.m_bRestrictGrowthToFrondPlane)
      {
        if (vAxis.Dot(LeafPlane.m_vNormal) > 0.0f)
          vAxis = LeafPlane.m_vNormal;
        else
          vAxis = -LeafPlane.m_vNormal;
        //vCurGrowDirection = LeafPlane.ProjectOntoPlane (vCurGrowDirection);
      }

      float fAngle = qSegmentRotation.GetRotationAngle();

      if (fAngle < 0.5f)
      {
        bChangeDir = true;
      }

      m_RNG.m_uiSeedValue = branchStructure.m_RandomData.m_GrowDirChange.GetRandomNumber();
      float fRealAngle = /*m_RNG.Randf (*/ aeMath::Min(fAngle, fMaxDirectionChange); //);

      aeMatrix mSegmentRotation;
      mSegmentRotation.SetRotationMatrix(vAxis, fRealAngle);


      vCurGrowDirection = mSegmentRotation * vCurGrowDirection;
      vCurGrowDirection.NormalizeSafe();

      AE_CHECK_DEV(!vCurGrowDirection.IsZeroVector(), "Grow direction is degenerate.");

      const aeVec3 vInfluence = ComputeAverageInfluence(vCurNodePos, branchStructure.m_Type);

      vCurGrowTargetDir += vInfluence;
      vCurGrowTargetDir.NormalizeSafe();

      AE_CHECK_DEV(!vCurGrowTargetDir.IsZeroVector(), "Grow target direction is degenerate.");

      vTargetDir1 += vInfluence;
      vTargetDir1.NormalizeSafe();
      vTargetDir2 += vInfluence;
      vTargetDir2.NormalizeSafe();

      const float fSegmentLength = BranchTypeDesc.m_iSegmentLengthCM / 100.0f; // Segment Length in Meters

      // if this branch uses obstacle avoidance, try to grow around obstacles
      if (BranchTypeDesc.m_bDoPhysicalSimulation)
      {
        const float fRemainingBranchLength = branchStats.m_fBranchLength - branchStructure.m_fBranchLength;
        const float fRayCastLength = aeMath::Min(BranchTypeDesc.m_fPhysicsLookAhead, fRemainingBranchLength);
        const aeVec3 vRayCastTarget = vCurNodePos + vCurGrowDirection * fRayCastLength;

        float fLineFraction;

        if (physics.IsLineObstructed(vCurNodePos, vRayCastTarget, fLineFraction))
        {
          float fDist = fRayCastLength * fLineFraction;

          if (fDist < 0.31f)
            break;

          if (fDist < 0.51f)
          {
            vCurGrowDirection = physics.FindLeastObstructedDirection(vCurNodePos, vRayCastTarget, (aeUInt32)BranchTypeDesc.m_fPhysicsEvasionAngle, fDist);

            AE_CHECK_DEV(vCurGrowDirection.IsValid(), "Grow Direction is degenerate.");
          }
          else
          {
            vCurGrowTargetDir = physics.FindLeastObstructedDirection(vCurNodePos, vRayCastTarget, (aeUInt32)BranchTypeDesc.m_fPhysicsEvasionAngle, fDist);

            // if this branch is a leaf, restrict its growth to the leaf plane
            if (BranchTypeDesc.m_bRestrictGrowthToFrondPlane)
              vCurGrowTargetDir = LeafPlane.ProjectOntoPlane(vCurGrowTargetDir).GetNormalized();
          }
        }

        vCurGrowDirection.NormalizeSafe();
      }

      AE_CHECK_DEV(vCurGrowDirection.IsValid(), "Grow Direction is degenerate.");

      vCurNodePos += vCurGrowDirection * fSegmentLength;

      branchStructure.m_fBranchLength += fSegmentLength;
    }

    if (branchStructure.m_Nodes.size() < 4)
    {
      m_pTreeStructure->m_BranchStructures.pop_back();
      return -1;
    }

    if (!branchStructure.UpdateThickness(*m_pTreeStructureDesc, *m_pTreeStructure))
    {
      m_pTreeStructure->m_BranchStructures.pop_back();
      return -1;
    }

    branchStructure.GenerateTexCoordV(*m_pTreeStructureDesc);

    physics.CreateBranchCollisionCapsules(*m_pTreeStructureDesc, *m_pTreeStructure, uiBranchID);
    return uiBranchID;
  }

  float TreeStructureGenerator::GetGrowDir2Distance(Kraut::BranchTargetDir2Usage::Enum Usage, float fBranchLength, float fUseDist)
  {
    switch (Usage)
    {
      case Kraut::BranchTargetDir2Usage::Off:
        return 10000.0f;
      case Kraut::BranchTargetDir2Usage::Absolute:
        return fUseDist;
      case Kraut::BranchTargetDir2Usage::Relative:
        return fBranchLength * fUseDist / 5.0f; // the targetdirusage is in [0;5] range
    }

    AE_CHECK_DEV(false, "Wrong Code Path.");
    return 0;
  }

  aeVec3 TreeStructureGenerator::GetTargetDir(Kraut::BranchTargetDir::Enum dir, const aeVec3& vParentDir, float fMaxDeviation, const aeVec3& vRotationAxis, const aeVec3& vOwnStartDir, bool bRelativeToParent)
  {
    aeVec3 r;
    float fAngle = 0.0f;

    switch (dir)
    {
      case Kraut::BranchTargetDir::Straight:
        r = Kraut::CreateRandomVector(m_RNG, vOwnStartDir, (aeUInt8)fMaxDeviation);
        return r;
      case Kraut::BranchTargetDir::Upwards:
        fAngle = 0.0f;
        break;
      case Kraut::BranchTargetDir::Degree22:
        fAngle = 22.5f;
        break;
      case Kraut::BranchTargetDir::Degree45:
        fAngle = 45.0f;
        break;
      case Kraut::BranchTargetDir::Degree67:
        fAngle = 67.5f;
        break;
      case Kraut::BranchTargetDir::Degree90:
        fAngle = 90.0f;
        break;
      case Kraut::BranchTargetDir::Degree112:
        fAngle = 112.5f;
        break;
      case Kraut::BranchTargetDir::Degree135:
        fAngle = 135.0f;
        break;
      case Kraut::BranchTargetDir::Degree157:
        fAngle = 157.5f;
        break;
      case Kraut::BranchTargetDir::Downwards:
        fAngle = 180.0f;
        break;
    }

    aeVec3 vNormal;

    if (bRelativeToParent)
    {
      aeQuaternion q;
      q.CreateQuaternion(vRotationAxis, fAngle);

      vNormal = q * vParentDir;
    }
    else
    {
      aeVec3 vRotateDownAxis;
      if (aeMath::Abs(vOwnStartDir.Dot(aeVec3(0, 1, 0))) > 0.99999f)
      {
        vRotateDownAxis = vRotationAxis;
      }
      else
        vRotateDownAxis = aeVec3(0, 1, 0).Cross(vOwnStartDir).GetNormalized();

      aeQuaternion q;
      q.CreateQuaternion(vRotateDownAxis, fAngle);

      vNormal = q * aeVec3(0, 1, 0);
    }

    r = Kraut::CreateRandomVector(m_RNG, vNormal, (aeUInt8)fMaxDeviation);

    return r;
  }

  bool TreeStructureGenerator::SkipNodes_Reverse(aeInt32& uiNode, float& fDistance, const Kraut::BranchStructure& branchStructure, float fSkipDistance, float fMaxDistance)
  {
    float fSkipped = 0.0f;

    if (fDistance > fMaxDistance)
      return false;

    while (uiNode > 0)
    {
      if (fSkipped >= fSkipDistance)
        return true;

      --uiNode;

      const float fNodeDist = (branchStructure.m_Nodes[uiNode].m_vPosition - branchStructure.m_Nodes[uiNode + 1].m_vPosition).GetLength();

      fDistance += fNodeDist;
      fSkipped += fNodeDist;

      if (fDistance > fMaxDistance)
        return false;
    }

    return false;
  }

} // namespace Kraut
