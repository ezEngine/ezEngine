#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/LodDesc.h>
#include <KrautGenerator/Description/TreeStructureDesc.h>
#include <KrautGenerator/Lod/BranchStructureLod.h>
#include <KrautGenerator/Lod/TreeStructureLodGenerator.h>
#include <KrautGenerator/Mesh/BranchMesh.h>
#include <KrautGenerator/Mesh/Mesh.h>
#include <KrautGenerator/Mesh/TreeMeshGenerator.h>
#include <KrautGenerator/TreeStructure/BranchStructure.h>

namespace Kraut
{
  void AlignVertexRing(Kraut::VertexRing& vertexRing, const aeVec3& vNodePosition, aeVec3& vRotationalDir);

  void TreeMeshGenerator::GenerateCapTriangles(Kraut::BranchMesh& mesh, const Kraut::VertexRing& vertexRing)
  {
    const aePlane p(vertexRing.m_Vertices[0], vertexRing.m_Vertices[1], vertexRing.m_Vertices[2]);
    Kraut::Triangle tri;
    Kraut::Vertex vtx[3];

    AE_CHECK_DEV(false, "Not implemented.");
    //vtx[0].m_uiBranchNodeIdx = ...;
    //vtx[1].m_uiBranchNodeIdx = ...;
    //vtx[2].m_uiBranchNodeIdx = ...;

    tri.m_uiPickingSubID = 0;

    vtx[0].m_vNormal = p.m_vNormal;
    vtx[1].m_vNormal = p.m_vNormal;
    vtx[2].m_vNormal = p.m_vNormal;

    for (aeUInt32 t = 0; t < vertexRing.m_Vertices.size() - 2; ++t)
    {
      vtx[0].m_vPosition = vertexRing.m_Vertices[0];
      vtx[1].m_vPosition = vertexRing.m_Vertices[t + 1];
      vtx[2].m_vPosition = vertexRing.m_Vertices[t + 2];

      tri.m_uiVertexIDs[0] = mesh.m_Mesh[Kraut::BranchGeometryType::Branch].AddVertex(vtx[0]);
      tri.m_uiVertexIDs[1] = mesh.m_Mesh[Kraut::BranchGeometryType::Branch].AddVertex(vtx[1]);
      tri.m_uiVertexIDs[2] = mesh.m_Mesh[Kraut::BranchGeometryType::Branch].AddVertex(vtx[2]);

      mesh.m_Mesh[Kraut::BranchGeometryType::Branch].m_Triangles.push_back(tri);
    }
  }


  void TreeMeshGenerator::GenerateSegmentTriangles(Kraut::BranchMesh& mesh, const Kraut::BranchStructure& branchStructure, Kraut::VertexRing& vertexRing0, Kraut::VertexRing& vertexRing1, float fTexCoordV0, float fTexCoordV1, float fTexCoordUOffset1, float fTexCoordUOffset2, bool bIsLastSegment, aeUInt32 uiBranchNodeID0, aeUInt32 uiBranchNodeID1)
  {
    const aeUInt32 uiVertices0 = vertexRing0.m_Vertices.size();
    const aeUInt32 uiVertices1 = vertexRing1.m_Vertices.size();

    aeUInt32 uiCurVertex0 = 0;
    aeUInt32 uiCurVertex1 = 0;

    aeUInt32 uiNextVertex0 = 1;
    aeUInt32 uiNextVertex1 = 1;

    aeUInt32 uiProcVertices0 = 0;
    aeUInt32 uiProcVertices1 = 0;

    const float fTexCoordStepU0 = (1.0f / vertexRing0.m_Vertices.size());
    const float fTexCoordStepU1 = (1.0f / vertexRing1.m_Vertices.size());

    float fDistToNext0 = fTexCoordStepU0;
    float fDistToNext1 = fTexCoordStepU1;

    Kraut::Triangle tri;
    Kraut::Vertex vtx[3];
    tri.m_uiPickingSubID = uiBranchNodeID0;

    float fPrevTexCoordU0 = fTexCoordUOffset1;
    float fPrevTexCoordU1 = fTexCoordUOffset2;
    float fNextTexCoordU0 = fPrevTexCoordU0 + fTexCoordStepU0;
    float fNextTexCoordU1 = fPrevTexCoordU1 + fTexCoordStepU1;

    const float fRing0Length = vertexRing0.m_fDiameter;
    const float fRing1Length = vertexRing1.m_fDiameter;

    const float fRingQ = fRing1Length / fRing0Length;

    while ((uiProcVertices0 < uiVertices0) || (uiProcVertices1 < uiVertices1))
    {
      if ((uiProcVertices0 < uiVertices0) && (fDistToNext0 <= fDistToNext1))
      {
        aeInt32& iID0 = vertexRing0.m_VertexIDs[uiCurVertex0];
        aeInt32& iID1 = vertexRing1.m_VertexIDs[uiCurVertex1];
        aeInt32& iID2 = vertexRing0.m_VertexIDs[uiNextVertex0];

        vtx[0].m_uiBranchNodeIdx = uiBranchNodeID0;
        vtx[1].m_uiBranchNodeIdx = uiBranchNodeID1;
        vtx[2].m_uiBranchNodeIdx = uiBranchNodeID0;

        vtx[0].m_vPosition = vertexRing0.m_Vertices[uiCurVertex0];
        vtx[1].m_vPosition = vertexRing1.m_Vertices[uiCurVertex1];
        vtx[2].m_vPosition = vertexRing0.m_Vertices[uiNextVertex0];

        vtx[0].m_vNormal = vertexRing0.m_Normals[uiCurVertex0];
        vtx[1].m_vNormal = vertexRing1.m_Normals[uiCurVertex1];
        vtx[2].m_vNormal = vertexRing0.m_Normals[uiNextVertex0];

        vtx[0].m_vTangent = (vertexRing0.m_Vertices[uiNextVertex0] - vertexRing0.m_Vertices[uiCurVertex0]).GetNormalized();
        vtx[1].m_vTangent = (vertexRing1.m_Vertices[uiNextVertex1] - vertexRing1.m_Vertices[uiCurVertex1]).GetNormalized();
        vtx[2].m_vTangent = (vertexRing0.m_Vertices[uiNextVertex0] - vertexRing0.m_Vertices[uiCurVertex0]).GetNormalized();

        for (int i = 0; i < 3; ++i)
        {
          vtx[i].m_vBiTangent = vtx[i].m_vNormal.Cross(vtx[i].m_vTangent).GetNormalized();
          vtx[i].m_vTangent = vtx[i].m_vBiTangent.Cross(vtx[i].m_vNormal).GetNormalized();
        }

        vtx[0].m_vTexCoord.x = fPrevTexCoordU0;
        vtx[1].m_vTexCoord.x = fPrevTexCoordU1 * fRingQ;
        vtx[2].m_vTexCoord.x = fNextTexCoordU0;

        vtx[0].m_vTexCoord.y = fTexCoordV0;
        vtx[1].m_vTexCoord.y = fTexCoordV1 * fRingQ;
        vtx[2].m_vTexCoord.y = fTexCoordV0;

        vtx[0].m_vTexCoord.z = 1;
        vtx[1].m_vTexCoord.z = fRingQ;
        vtx[2].m_vTexCoord.z = 1;

        uiCurVertex0 = uiNextVertex0;
        ++uiNextVertex0;
        ++uiProcVertices0;

        if (uiNextVertex0 == uiVertices0)
        {
          uiNextVertex0 = 0;
          fDistToNext1 = -1;
        }
        else
          fDistToNext0 += fTexCoordStepU0;

        fPrevTexCoordU0 = fNextTexCoordU0;
        fNextTexCoordU0 += fTexCoordStepU0;

        vtx[0].m_iSharedVertex = iID0;
        vtx[1].m_iSharedVertex = iID1;
        vtx[2].m_iSharedVertex = iID2;

        tri.m_uiVertexIDs[0] = mesh.m_Mesh[Kraut::BranchGeometryType::Branch].AddVertex(vtx[0], iID0);

        if (bIsLastSegment)
          vtx[1].m_iSharedVertex = vertexRing1.m_VertexIDs[0];

        tri.m_uiVertexIDs[1] = mesh.m_Mesh[Kraut::BranchGeometryType::Branch].AddVertex(vtx[1], iID1);
        tri.m_uiVertexIDs[2] = mesh.m_Mesh[Kraut::BranchGeometryType::Branch].AddVertex(vtx[2], iID2);

        mesh.m_Mesh[Kraut::BranchGeometryType::Branch].m_Triangles.push_back(tri);
      }
      else
      {
        aeInt32& iID0 = vertexRing0.m_VertexIDs[uiCurVertex0];
        aeInt32& iID1 = vertexRing1.m_VertexIDs[uiCurVertex1];
        aeInt32& iID2 = vertexRing1.m_VertexIDs[uiNextVertex1];

        vtx[0].m_uiBranchNodeIdx = uiBranchNodeID0;
        vtx[1].m_uiBranchNodeIdx = uiBranchNodeID1;
        vtx[2].m_uiBranchNodeIdx = uiBranchNodeID1;

        vtx[0].m_vPosition = vertexRing0.m_Vertices[uiCurVertex0];
        vtx[1].m_vPosition = vertexRing1.m_Vertices[uiCurVertex1];
        vtx[2].m_vPosition = vertexRing1.m_Vertices[uiNextVertex1];

        vtx[0].m_vNormal = vertexRing0.m_Normals[uiCurVertex0];
        vtx[1].m_vNormal = vertexRing1.m_Normals[uiCurVertex1];
        vtx[2].m_vNormal = vertexRing1.m_Normals[uiNextVertex1];

        vtx[0].m_vTangent = (vertexRing0.m_Vertices[uiNextVertex0] - vertexRing0.m_Vertices[uiCurVertex0]).GetNormalized();
        vtx[1].m_vTangent = (vertexRing1.m_Vertices[uiNextVertex1] - vertexRing1.m_Vertices[uiCurVertex1]).GetNormalized();
        vtx[2].m_vTangent = (vertexRing1.m_Vertices[uiNextVertex1] - vertexRing1.m_Vertices[uiCurVertex1]).GetNormalized();

        for (int i = 0; i < 3; ++i)
        {
          vtx[i].m_vBiTangent = vtx[i].m_vNormal.Cross(vtx[i].m_vTangent).GetNormalized();
          vtx[i].m_vTangent = vtx[i].m_vBiTangent.Cross(vtx[i].m_vNormal).GetNormalized();
        }

        vtx[0].m_vTexCoord.x = fPrevTexCoordU0;
        vtx[1].m_vTexCoord.x = fPrevTexCoordU1 * fRingQ;
        vtx[2].m_vTexCoord.x = fNextTexCoordU1 * fRingQ;

        vtx[0].m_vTexCoord.y = fTexCoordV0;
        vtx[1].m_vTexCoord.y = fTexCoordV1 * fRingQ;
        vtx[2].m_vTexCoord.y = fTexCoordV1 * fRingQ;

        vtx[0].m_vTexCoord.z = 1;
        vtx[1].m_vTexCoord.z = fRingQ;
        vtx[2].m_vTexCoord.z = fRingQ;

        uiCurVertex1 = uiNextVertex1;
        ++uiNextVertex1;
        ++uiProcVertices1;

        if (uiNextVertex1 == uiVertices1)
        {
          uiNextVertex1 = 0;
          fDistToNext0 = -1;
        }
        else
          fDistToNext1 += fTexCoordStepU1;

        fPrevTexCoordU1 = fNextTexCoordU1;
        fNextTexCoordU1 += fTexCoordStepU1;

        vtx[0].m_iSharedVertex = iID0;
        vtx[1].m_iSharedVertex = iID1;
        vtx[2].m_iSharedVertex = iID2;

        tri.m_uiVertexIDs[0] = mesh.m_Mesh[Kraut::BranchGeometryType::Branch].AddVertex(vtx[0], iID0);

        if (bIsLastSegment)
        {
          vtx[1].m_iSharedVertex = vertexRing1.m_VertexIDs[0];
          vtx[2].m_iSharedVertex = vertexRing1.m_VertexIDs[0];
        }

        tri.m_uiVertexIDs[1] = mesh.m_Mesh[Kraut::BranchGeometryType::Branch].AddVertex(vtx[1], iID1);
        tri.m_uiVertexIDs[2] = mesh.m_Mesh[Kraut::BranchGeometryType::Branch].AddVertex(vtx[2], iID2);

        mesh.m_Mesh[Kraut::BranchGeometryType::Branch].m_Triangles.push_back(tri);
      }
    }
  }

  void TreeMeshGenerator::GenerateBranchTriangles(Kraut::BranchMesh& mesh, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::BranchStructure& branchStructure, const Kraut::BranchStructureLod& branchStructureLod, const Kraut::LodDesc& lodDesc)
  {
    const aeUInt32 uiNumLodNodes = branchStructureLod.m_NodeIDs.size();

    if ((lodDesc.m_AllowTypes[Kraut::BranchGeometryType::Branch] & (1 << branchStructure.m_Type)) == 0)
      return;

    if (uiNumLodNodes < 2)
      return;

    const Kraut::SpawnNodeDesc& spawnDesc = treeStructureDesc.m_BranchTypes[branchStructure.m_Type];

    aeInt32 iCurRing = 1;
    aeInt32 iPrevRing = 0;

    Kraut::VertexRing vertexRing[2];

    TreeStructureLodGenerator::GenerateLodBranchVertexRing(branchStructureLod, vertexRing[iPrevRing], treeStructureDesc, branchStructure, 0, branchStructure.m_Nodes[branchStructureLod.m_NodeIDs[0]].m_vPosition, lodDesc.m_fVertexRingDetail);

    aeVec3 vRotationalDir = (vertexRing[iPrevRing].m_Vertices[0] - branchStructure.m_Nodes[branchStructureLod.m_NodeIDs[0]].m_vPosition).GetNormalized();

    // if the branches should start without a hole, this could be used to fill it
    //GenerateCapTriangles(vertexRing[iPrevRing], uiPickingID);

    for (aeUInt32 n = 1; n < uiNumLodNodes; ++n)
    {
      const aeUInt32 uiPrevNodeID = branchStructureLod.m_NodeIDs[n - 1];
      const aeUInt32 uiCurNodeID = branchStructureLod.m_NodeIDs[n];
      const auto& prevBranchNode = branchStructure.m_Nodes[uiPrevNodeID];
      const auto& curBranchNode = branchStructure.m_Nodes[uiCurNodeID];
      const float fPosAlongBranch0 = (float)(n - 1) / (float)(uiNumLodNodes - 1);
      const float fPosAlongBranch1 = (float)n / (float)(uiNumLodNodes - 1);
      const float fFlareRotation0 = spawnDesc.m_bRotateTexCoords ? (spawnDesc.m_fFlareRotation * fPosAlongBranch0) / 360.0f : 0.0f;
      const float fFlareRotation1 = spawnDesc.m_bRotateTexCoords ? (spawnDesc.m_fFlareRotation * fPosAlongBranch1) / 360.0f : 0.0f;

      TreeStructureLodGenerator::GenerateLodBranchVertexRing(branchStructureLod, vertexRing[iCurRing], treeStructureDesc, branchStructure, n, curBranchNode.m_vPosition, lodDesc.m_fVertexRingDetail);

      AlignVertexRing(vertexRing[iCurRing], curBranchNode.m_vPosition, vRotationalDir);

      GenerateSegmentTriangles(mesh, branchStructure, vertexRing[iPrevRing], vertexRing[iCurRing], prevBranchNode.m_fTexCoordV, curBranchNode.m_fTexCoordV, fFlareRotation0, fFlareRotation1, false, uiPrevNodeID, uiCurNodeID);

      aeMath::Swap(iPrevRing, iCurRing);
    }

    // Do the same for the tip

    const Kraut::BranchNode* pPrevNode = &branchStructure.m_Nodes[branchStructureLod.m_NodeIDs.back()];

    const bool bSimpleSpikeTip = (branchStructureLod.m_TipNodes.size() == 1) && (vertexRing[iPrevRing].m_Vertices.size() == 3);

    if (bSimpleSpikeTip && lodDesc.m_BranchSpikeTipMode != BranchSpikeTipMode::FullDetail)
    {
      if (lodDesc.m_BranchSpikeTipMode == BranchSpikeTipMode::SingleTriangle)
      {
        const aeUInt32 uiNodeID = branchStructureLod.m_NodeIDs[uiNumLodNodes - 1];

        const aeVec3 vPos = branchStructure.m_Nodes[uiNodeID].m_vPosition;

        Kraut::Triangle t;
        Kraut::Vertex vtx[3];

        t.m_uiPickingSubID = branchStructure.m_Nodes.size() - 1;

        vtx[0].m_uiBranchNodeIdx = uiNodeID;
        vtx[1].m_uiBranchNodeIdx = uiNodeID;
        vtx[2].m_uiBranchNodeIdx = uiNodeID;
        vtx[0].m_vPosition = vertexRing[iPrevRing].m_Vertices[0];
        vtx[1].m_vPosition = vertexRing[iPrevRing].m_Vertices[2];
        vtx[2].m_vPosition = vertexRing[iPrevRing].m_Vertices[1];
        vtx[1].m_vTexCoord.x = 1;
        vtx[1].m_vTexCoord.y = 0;
        vtx[2].m_vTexCoord.x = 0.5f;
        vtx[2].m_vTexCoord.y = 1;
        vtx[0].m_vNormal = (vertexRing[iPrevRing].m_Vertices[0] - vPos).GetNormalized();
        vtx[1].m_vNormal = (vertexRing[iPrevRing].m_Vertices[2] - vPos).GetNormalized();
        vtx[2].m_vNormal = (vertexRing[iPrevRing].m_Vertices[1] - vPos).GetNormalized();

        vtx[0].m_iSharedVertex = vertexRing[iPrevRing].m_VertexIDs[0];
        vtx[1].m_iSharedVertex = vertexRing[iPrevRing].m_VertexIDs[2];
        vtx[2].m_iSharedVertex = vertexRing[iPrevRing].m_VertexIDs[1];

        t.m_uiVertexIDs[0] = mesh.m_Mesh[Kraut::BranchGeometryType::Branch].AddVertex(vtx[0], vertexRing[iPrevRing].m_VertexIDs[0]);
        t.m_uiVertexIDs[1] = mesh.m_Mesh[Kraut::BranchGeometryType::Branch].AddVertex(vtx[1], vertexRing[iPrevRing].m_VertexIDs[2]);
        t.m_uiVertexIDs[2] = mesh.m_Mesh[Kraut::BranchGeometryType::Branch].AddVertex(vtx[2], vertexRing[iPrevRing].m_VertexIDs[1]);

        mesh.m_Mesh[Kraut::BranchGeometryType::Branch].m_Triangles.push_back(t);
      }
      else
      {
        // leave a hole
      }
    }
    else
    {
      const aeVec3 vNormalAnchor = branchStructure.m_Nodes[branchStructureLod.m_NodeIDs.back()].m_vPosition;

      const float fFlareRotation = spawnDesc.m_bRotateTexCoords ? (spawnDesc.m_fFlareRotation) / 360.0f : 0.0f;

      const aeUInt32 uiTipRingVertices = vertexRing[iPrevRing].m_Vertices.size();
      const aeUInt32 uiBranchNodeID = branchStructure.m_Nodes.size() - 1;

      for (aeUInt32 n = 0; n < branchStructureLod.m_TipNodes.size(); ++n)
      {
        TreeStructureLodGenerator::GenerateLodTipVertexRing(branchStructureLod, vertexRing[iCurRing], treeStructureDesc, branchStructure, n, vNormalAnchor, uiTipRingVertices);

        AlignVertexRing(vertexRing[iCurRing], branchStructureLod.m_TipNodes[n].m_vPosition, vRotationalDir);

        const float fNextTexCoordV = branchStructureLod.m_TipNodes[n].m_fTexCoordV;

        GenerateSegmentTriangles(mesh, branchStructure, vertexRing[iPrevRing], vertexRing[iCurRing], pPrevNode->m_fTexCoordV, branchStructureLod.m_TipNodes[n].m_fTexCoordV, fFlareRotation, fFlareRotation, n == branchStructureLod.m_TipNodes.size() - 1, uiBranchNodeID + n, uiBranchNodeID + n + 1);

        aeMath::Swap(iPrevRing, iCurRing);

        pPrevNode = &branchStructureLod.m_TipNodes[n];
      }
    }

    mesh.m_Mesh[Kraut::BranchGeometryType::Branch].GenerateVertexNormals();
  }

} // namespace Kraut
