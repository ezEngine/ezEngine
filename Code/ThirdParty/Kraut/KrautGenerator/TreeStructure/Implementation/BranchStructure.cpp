#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/TreeStructureDesc.h>
#include <KrautGenerator/Mesh/Mesh.h>
#include <KrautGenerator/TreeStructure/BranchStructure.h>
#include <KrautGenerator/TreeStructure/TreeStructure.h>

namespace Kraut
{
  void GenerateVertexRing(VertexRing& out_VertexRing, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::BranchStructure& branchStructure, aeInt32 iPrevNodeIdx, aeInt32 iCurNodeIdx, aeInt32 iNextNodeIdx, float fVertexRingDetail, const aeVec3& vNormalAnchor);

  BranchStructure::BranchStructure() = default;
  BranchStructure::~BranchStructure() = default;

  void BranchStructure::Clear()
  {
    m_iParentBranchID = -1;
    m_uiParentBranchNodeID = 0;
    m_iUmbrellaBuddyID = -1;
    m_fUmbrellaBranchRotation = 0;

    m_Nodes.clear();

    m_fBranchLength = 0.0f;
    m_fThickness = 0.0f;
    m_fLastDiameter = 0.0f;
    m_fFrondColorVariation = 0.0f;
    m_uiFrondTextureVariation = 0;
    m_vLeafUpDirection.SetZero();

    m_Type = Kraut::BranchType::None;
  }

  aeVec3 BranchStructure::GetDirectionAtNode(aeUInt32 uiNode) const
  {
    if ((uiNode >= m_Nodes.size()) || (m_Nodes.size() < 2))
      return aeVec3::ZeroVector();

    if (uiNode < m_Nodes.size() - 1)
      return (m_Nodes[uiNode + 1].m_vPosition - m_Nodes[uiNode].m_vPosition).GetNormalized();

    return (m_Nodes[uiNode].m_vPosition - m_Nodes[uiNode - 1].m_vPosition).GetNormalized();
  }

  const aeMatrix BranchStructure::GetNodeTransformation(aeUInt32 uiNode)
  {
    AE_CHECK_DEV(uiNode < m_Nodes.size(), "Invalid Node-Index");

    aeUInt32 uiNode1 = uiNode;
    aeUInt32 uiNode2 = uiNode + 1;

    if (uiNode == m_Nodes.size() - 1)
    {
      uiNode1 = uiNode - 1;
      uiNode2 = uiNode;
    }

    aeVec3 vDir = (m_Nodes[uiNode2].m_vPosition - m_Nodes[uiNode1].m_vPosition).GetNormalized();

    aeMatrix m;

    if (vDir.GetAngleBetween(aeVec3(0, 1, 0)) < 0.5f)
      m.SetObjectOrientationMatrixZ(m_Nodes[uiNode].m_vPosition, vDir, vDir.GetOrthogonalVector());
    else
      m.SetObjectOrientationMatrixZ(m_Nodes[uiNode].m_vPosition, vDir);

    return m;
  }

  bool BranchStructure::UpdateThickness(const TreeStructureDesc& structureDesc, const TreeStructure& treeStructure)
  {
    float fThickness = m_fThickness;
    const float fMinThickness = structureDesc.m_BranchTypes[m_Type].m_uiMinBranchThicknessInCM / 100.0f;

    if (m_iParentBranchID != -1)
    {
      const float fParentThickness = treeStructure.m_BranchStructures[m_iParentBranchID].m_Nodes[m_uiParentBranchNodeID].m_fThickness;

      if (fParentThickness < fMinThickness)
      {
        m_Nodes.clear();
        return false;
      }

      fThickness = aeMath::Clamp(fThickness, fMinThickness, fParentThickness);
    }

    m_fThickness = fThickness;

    float fLengthAtNode = 0.0f;
    m_Nodes[0].m_fThickness = fThickness;

    const Kraut::SpawnNodeDesc& bnd = structureDesc.m_BranchTypes[m_Type];

    float fThicknessFalloff = 1.0f;
    float fStepFalloff = (1.0f - bnd.m_fRoundnessFactor) / (m_Nodes.size());

    for (aeUInt32 i = 0; i < m_Nodes.size(); ++i)
    {
      fThicknessFalloff -= fStepFalloff;

      const float fContour = structureDesc.m_BranchTypes[m_Type].m_BranchContour.GetValueAt((float)i / (float)(m_Nodes.size() - 1));

      m_Nodes[i].m_fThickness = fThickness * fThicknessFalloff * fContour;
    }

    return true;
  }

  void BranchStructure::GenerateTexCoordV(const Kraut::TreeStructureDesc& treeStructureDesc)
  {
    AE_CHECK_DEV(m_Nodes.size() > 2, "Branch should have been culled.");

    const float fVertexRingDetail = 0.05f;

    Kraut::VertexRing vertexRing;
    Kraut::GenerateVertexRing(vertexRing, treeStructureDesc, *this, -1, 0, 1, fVertexRingDetail, m_Nodes[0].m_vPosition);

    float fPrevTexCoordV = 0.0f;
    float fLastLength = vertexRing.m_fDiameter;

    aeVec3 vLastPos = m_Nodes[0].m_vPosition;

    for (aeUInt32 n = 1; n < m_Nodes.size(); ++n)
    {
      Kraut::GenerateVertexRing(vertexRing, treeStructureDesc, *this, n - 1, n, n + 1, fVertexRingDetail, m_Nodes[n].m_vPosition);

      const float fDist = (vLastPos - m_Nodes[n].m_vPosition).GetLength();

      float fNextTexCoordV = fPrevTexCoordV + (fDist / fLastLength);

      if (vertexRing.m_fDiameter > 0.1f)
        fLastLength = vertexRing.m_fDiameter;

      m_Nodes[n - 1].m_fTexCoordV = fPrevTexCoordV;
      m_Nodes[n].m_fTexCoordV = fNextTexCoordV;

      fPrevTexCoordV = fNextTexCoordV;
      vLastPos = m_Nodes[n].m_vPosition;
    }

    m_fLastDiameter = fLastLength;
  }

} // namespace Kraut
