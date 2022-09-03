#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/LodDesc.h>
#include <KrautGenerator/Description/TreeStructureDesc.h>
#include <KrautGenerator/Lod/BranchStructureLod.h>
#include <KrautGenerator/Mesh/BranchMesh.h>
#include <KrautGenerator/Mesh/TreeMeshGenerator.h>
#include <KrautGenerator/TreeStructure/BranchNode.h>
#include <KrautGenerator/TreeStructure/BranchStructure.h>

namespace Kraut
{
  void TreeMeshGenerator::AddStaticLeafTriangles(Kraut::BranchMesh& mesh, const Kraut::BranchStructure& branchStructure, aeUInt32 uiNodeID, float fSize, aeUInt8 uiTilingX, aeUInt8 uiTilingY)
  {
    if (fSize < 0.05f)
      return;

    AE_CHECK_DEV(uiNodeID > 0, "Cannot spawn leaves at node zero.");

    const aeVec3 vPos = branchStructure.m_Nodes[uiNodeID].m_vPosition;
    const aeVec3 vPrevPos = branchStructure.m_Nodes[uiNodeID - 1].m_vPosition;
    const aeVec3 vDir = (vPos - vPrevPos).GetNormalized();

    aeVec3 vRight = vDir.GetOrthogonalVector().GetNormalized();
    aeVec3 vUp = vDir.Cross(vRight).GetNormalized();

    Kraut::Triangle t;
    t.m_uiPickingSubID = 0;

    Kraut::Vertex mv;
    mv.m_uiBranchNodeIdx = uiNodeID;
    mv.m_uiColorVariation = (aeUInt8)m_RNG.Rand(255);
    mv.m_vNormal = vDir;
    mv.m_vTangent = vRight;
    mv.m_vBiTangent = vUp;

    vRight *= fSize;
    vUp *= fSize;

    const aeInt32 iTexX = m_RNG.Rand(uiTilingX);
    const aeInt32 iTexY = m_RNG.Rand(uiTilingY);

    const float fTexWidth = 1.0f / uiTilingX;
    const float fTexHeight = 1.0f / uiTilingY;

    const float fTexCoordU0 = fTexWidth * iTexX;
    const float fTexCoordV0 = fTexHeight * iTexY;

    const float fTexCoordU1 = fTexCoordU0 + fTexWidth;
    const float fTexCoordV1 = fTexCoordV0 + fTexHeight;

    aeUInt32 uiVtx[4];

    // add the 4 vertices with different texcoords
    mv.m_vTexCoord.SetVector(fTexCoordU0, fTexCoordV0, 1);
    mv.m_vPosition = vPos - vRight - vUp;
    mv.m_vTangent.SetVector(0, 0); // encode the billboard span factor into the tangent
    uiVtx[0] = mesh.m_Mesh[Kraut::BranchGeometryType::Leaf].AddVertex(mv);

    mv.m_vTexCoord.SetVector(fTexCoordU1, fTexCoordV0, 1);
    mv.m_vPosition = vPos + vRight - vUp;
    mv.m_vTangent.SetVector(1, 0); // encode the billboard span factor into the tangent
    uiVtx[1] = mesh.m_Mesh[Kraut::BranchGeometryType::Leaf].AddVertex(mv);

    mv.m_vTexCoord.SetVector(fTexCoordU1, fTexCoordV1, 1);
    mv.m_vPosition = vPos + vRight + vUp;
    mv.m_vTangent.SetVector(1, 1); // encode the billboard span factor into the tangent
    uiVtx[2] = mesh.m_Mesh[Kraut::BranchGeometryType::Leaf].AddVertex(mv);

    mv.m_vTexCoord.SetVector(fTexCoordU0, fTexCoordV1, 1);
    mv.m_vPosition = vPos - vRight + vUp;
    mv.m_vTangent.SetVector(0, 1); // encode the billboard span factor into the tangent
    uiVtx[3] = mesh.m_Mesh[Kraut::BranchGeometryType::Leaf].AddVertex(mv);

    // add the two triangles
    t.m_uiVertexIDs[0] = uiVtx[0];
    t.m_uiVertexIDs[1] = uiVtx[1];
    t.m_uiVertexIDs[2] = uiVtx[2];
    mesh.m_Mesh[Kraut::BranchGeometryType::Leaf].m_Triangles.push_back(t);

    t.m_uiVertexIDs[0] = uiVtx[0];
    t.m_uiVertexIDs[1] = uiVtx[2];
    t.m_uiVertexIDs[2] = uiVtx[3];
    mesh.m_Mesh[Kraut::BranchGeometryType::Leaf].m_Triangles.push_back(t);
  }

  void TreeMeshGenerator::AddBillboardLeafTriangles(Kraut::BranchMesh& mesh, const Kraut::BranchStructure& branchStructure, aeUInt32 uiNodeID, float fSize, aeUInt8 uiTilingX, aeUInt8 uiTilingY)
  {
    if (fSize < 0.05f)
      return;

    const aeVec3 vPos = branchStructure.m_Nodes[uiNodeID].m_vPosition;

    Kraut::Triangle t;
    t.m_uiPickingSubID = 0;

    Kraut::Vertex mv;
    mv.m_uiBranchNodeIdx = uiNodeID;
    mv.m_vPosition = vPos;
    mv.m_uiColorVariation = (aeUInt8)m_RNG.Rand(255);

    const aeInt32 iTexX = m_RNG.Rand(uiTilingX);
    const aeInt32 iTexY = m_RNG.Rand(uiTilingY);

    const float fTexWidth = 1.0f / uiTilingX;
    const float fTexHeight = 1.0f / uiTilingY;

    const float fTexCoordU0 = fTexWidth * iTexX;
    const float fTexCoordV0 = fTexHeight * iTexY;

    const float fTexCoordU1 = fTexCoordU0 + fTexWidth;
    const float fTexCoordV1 = fTexCoordV0 + fTexHeight;

    aeUInt32 uiVtx[4];

    // add the 4 vertices with different texcoords
    mv.m_vTexCoord.SetVector(fTexCoordU0, fTexCoordV0, fSize);
    mv.m_vTangent.SetVector(0, 0); // encode the billboard span factor into the tangent
    uiVtx[0] = mesh.m_Mesh[Kraut::BranchGeometryType::Leaf].AddVertex(mv);

    mv.m_vTexCoord.SetVector(fTexCoordU1, fTexCoordV0, fSize);
    mv.m_vTangent.SetVector(1, 0); // encode the billboard span factor into the tangent
    uiVtx[1] = mesh.m_Mesh[Kraut::BranchGeometryType::Leaf].AddVertex(mv);

    mv.m_vTexCoord.SetVector(fTexCoordU1, fTexCoordV1, fSize);
    mv.m_vTangent.SetVector(1, 1); // encode the billboard span factor into the tangent
    uiVtx[2] = mesh.m_Mesh[Kraut::BranchGeometryType::Leaf].AddVertex(mv);

    mv.m_vTexCoord.SetVector(fTexCoordU0, fTexCoordV1, fSize);
    mv.m_vTangent.SetVector(0, 1); // encode the billboard span factor into the tangent
    uiVtx[3] = mesh.m_Mesh[Kraut::BranchGeometryType::Leaf].AddVertex(mv);

    // add the two triangles
    t.m_uiVertexIDs[0] = uiVtx[0];
    t.m_uiVertexIDs[1] = uiVtx[1];
    t.m_uiVertexIDs[2] = uiVtx[2];
    mesh.m_Mesh[Kraut::BranchGeometryType::Leaf].m_Triangles.push_back(t);

    t.m_uiVertexIDs[0] = uiVtx[0];
    t.m_uiVertexIDs[1] = uiVtx[2];
    t.m_uiVertexIDs[2] = uiVtx[3];
    mesh.m_Mesh[Kraut::BranchGeometryType::Leaf].m_Triangles.push_back(t);
  }

  void TreeMeshGenerator::GenerateLeafTriangles(Kraut::BranchMesh& mesh, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::BranchStructure& branchStructure, const Kraut::BranchStructureLod& branchStructureLod, const Kraut::LodDesc& lodDesc)
  {
    const Kraut::SpawnNodeDesc& spawnDesc = treeStructureDesc.m_BranchTypes[branchStructure.m_Type];

    if ((lodDesc.m_AllowTypes[Kraut::BranchGeometryType::Leaf] & (1 << branchStructure.m_Type)) == 0)
      return;

    if (branchStructureLod.m_NodeIDs.size() < 2)
      return;

    const aeUInt32 uiMaxNodes = branchStructure.m_Nodes.size() - 1;
    aeUInt32 uiNodeID = uiMaxNodes;

    aeDeque<aeUInt32> Positions;
    Positions.push_back(uiNodeID);

    float fDistanceGone = 0.0f;

    if (spawnDesc.m_fLeafInterval > 0)
    {
      while (uiNodeID > 1)
      {
        fDistanceGone = 0.0f;

        while ((uiNodeID > 1) && (fDistanceGone < spawnDesc.m_fLeafInterval))
        {
          aeVec3 vPos1 = branchStructure.m_Nodes[uiNodeID].m_vPosition;
          aeVec3 vPos2 = branchStructure.m_Nodes[uiNodeID - 1].m_vPosition;

          fDistanceGone += (vPos1 - vPos2).GetLength();

          --uiNodeID;
        }

        if (uiNodeID > 1)
          Positions.push_back(uiNodeID);
      }

      if ((fDistanceGone < spawnDesc.m_fLeafInterval / 0.5f) && (Positions.size() > 1))
        Positions.pop_back();
    }

    m_RNG.m_uiSeedValue = branchStructure.m_RandomData.m_FrondColorVariationRD;

    const aeUInt32 uiTilingX = spawnDesc.m_uiTextureTilingX[Kraut::BranchGeometryType::Leaf];
    const aeUInt32 uiTilingY = spawnDesc.m_uiTextureTilingY[Kraut::BranchGeometryType::Leaf];

    for (aeUInt32 i = 0; i < Positions.size(); ++i)
    {
      const float fLeafSize = spawnDesc.m_fLeafSize * spawnDesc.m_LeafScale.GetValueAt(Positions[i] / (float)uiMaxNodes);

      if (spawnDesc.m_bBillboardLeaves)
      {
        AddBillboardLeafTriangles(mesh, branchStructure, Positions[i], fLeafSize, uiTilingX, uiTilingY);
      }
      else
      {
        AddStaticLeafTriangles(mesh, branchStructure, Positions[i], fLeafSize, uiTilingX, uiTilingY);
      }
    }
  }

} // namespace Kraut
