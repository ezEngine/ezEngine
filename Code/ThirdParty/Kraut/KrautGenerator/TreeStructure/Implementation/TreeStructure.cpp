#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/SpawnNodeDesc.h>
#include <KrautGenerator/TreeStructure/TreeStructure.h>

namespace Kraut
{
  TreeStructure::TreeStructure() = default;
  TreeStructure::~TreeStructure() = default;

  void TreeStructure::Clear()
  {
    m_BranchStructures.clear();
  }

  Kraut::BoundingBox TreeStructure::ComputeBoundingBox() const
  {
    Kraut::BoundingBox result;
    result.SetInvalid();

    float fMaxThickness = 0.0f;

    // increase the bbox size by all node that are in the high-detail skeleton
    for (aeUInt32 b = 0; b < m_BranchStructures.size(); ++b)
    {
      const auto& nodes = m_BranchStructures[b].m_Nodes;

      // all regular nodes
      for (aeUInt32 n = 0; n < nodes.size(); ++n)
      {
        result.ExpandToInclude(nodes[n].m_vPosition);

        fMaxThickness = aeMath::Max(fMaxThickness, nodes[n].m_fThickness);
      }
    }

    result.AddBoundary(aeVec3(fMaxThickness));

    return result;
  }

  aeUInt32 TreeStructure::DuplicateBranch(aeUInt32 uiOriginalBranchID, float fRotation, aeInt32 iBranchBuddy)
  {
    const Kraut::BranchStructure& originalBranch = m_BranchStructures[uiOriginalBranchID];

    const aeVec3 vOrigin = originalBranch.m_Nodes[0].m_vPosition;
    const aeInt32 iParentBranch = originalBranch.m_iParentBranchID;
    aeVec3 vParentDirection(0, 1, 0);

    if (iParentBranch >= 0)
    {
      const Kraut::BranchStructure& parentBranch = m_BranchStructures[iParentBranch];

      const aeVec3 vPrevOrigin = parentBranch.m_Nodes[originalBranch.m_uiParentBranchNodeID - 1].m_vPosition;
      vParentDirection = (vOrigin - vPrevOrigin).GetNormalized();
    }

    const aeUInt32 uiNewBranchID = m_BranchStructures.size();
    m_BranchStructures.push_back();
    Kraut::BranchStructure& newBranch = m_BranchStructures[uiNewBranchID];

    aeMatrix mRotation, mTransI, mTrans, mFinal;
    mTransI.SetTranslationMatrix(-vOrigin);
    mTrans.SetTranslationMatrix(vOrigin);
    mRotation.SetRotationMatrix(vParentDirection, fRotation);

    mFinal = mTrans * mRotation * mTransI;

    newBranch = originalBranch;
    newBranch.m_iUmbrellaBuddyID = iBranchBuddy;
    newBranch.m_fUmbrellaBranchRotation = fRotation;

    for (aeUInt32 n = 0; n < newBranch.m_Nodes.size(); ++n)
    {
      newBranch.m_Nodes[n].m_vPosition = mFinal * newBranch.m_Nodes[n].m_vPosition;
    }

    return uiNewBranchID;
  }

} // namespace Kraut
