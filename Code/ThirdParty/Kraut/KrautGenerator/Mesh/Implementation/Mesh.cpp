#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/TreeStructureDesc.h>
#include <KrautGenerator/Mesh/Mesh.h>
#include <KrautGenerator/TreeStructure/BranchNode.h>
#include <KrautGenerator/TreeStructure/BranchStructure.h>

namespace Kraut
{
  Vertex::Vertex() = default;
  Triangle::Triangle() = default;

  void Triangle::Flip()
  {
    aeMath::Swap(m_uiVertexIDs[1], m_uiVertexIDs[2]);
  }

  aeUInt32 Mesh::AddVertex(const Kraut::Vertex& vtx)
  {
    AE_CHECK_DEV(vtx.m_uiBranchNodeIdx != 0xFFFFFFFF, "Kraut::Mesh::AddVertex: Invalid branch node ID");
    AE_CHECK_DEV(vtx.m_vPosition.IsValid(), "Kraut::Mesh::AddVertex: Position is degenerate.");
    AE_CHECK_DEV(vtx.m_vNormal.IsValid(), "Kraut::Mesh::AddVertex: Normal is degenerate.");
    AE_CHECK_DEV(vtx.m_vTangent.IsValid(), "Kraut::Mesh::AddVertex: Tangent is degenerate: %.8f | %.8f | %.8f", vtx.m_vTangent.x, vtx.m_vTangent.y, vtx.m_vTangent.z);
    AE_CHECK_DEV(vtx.m_vBiTangent.IsValid(), "Kraut::Mesh::AddVertex: BiTangent is degenerate.");
    AE_CHECK_DEV(vtx.m_vTexCoord.IsValid(), "Kraut::Mesh::AddVertex: TexCoord is degenerate.");


    m_Vertices.push_back(vtx);

    if (vtx.m_iSharedVertex == -1)
    {
      m_Vertices.back().m_iSharedVertex = (aeInt32)m_Vertices.size() - 1;
    }

    return ((aeUInt32)m_Vertices.size() - 1);
  }

  aeUInt32 Mesh::AddVertex(const Kraut::Vertex& vtx, aeInt32& iWriteBack)
  {
    aeUInt32 ui = AddVertex(vtx);

    if (iWriteBack == -1)
      iWriteBack = (aeInt32)ui;

    return ui;
  }

  aeUInt32 Mesh::AddVertex(const Kraut::Vertex& vtx, aeUInt32 x, aeUInt32 uiWidth, aeUInt32 y, aeUInt32 uiFirstVertex)
  {
    AE_CHECK_DEV(vtx.m_uiBranchNodeIdx != 0xFFFFFFFF, "Kraut::Mesh::AddVertex: Invalid branch node ID");
    AE_CHECK_DEV(vtx.m_vPosition.IsValid(), "Kraut::Mesh::AddVertex: Position is degenerate.");
    AE_CHECK_DEV(vtx.m_vNormal.IsValid(), "Kraut::Mesh::AddVertex: Normal is degenerate.");
    AE_CHECK_DEV(vtx.m_vTangent.IsValid(), "Kraut::Mesh::AddVertex: Tangent is degenerate: %.8f | %.8f | %.8f", vtx.m_vTangent.x, vtx.m_vTangent.y, vtx.m_vTangent.z);
    AE_CHECK_DEV(vtx.m_vBiTangent.IsValid(), "Kraut::Mesh::AddVertex: BiTangent is degenerate.");
    AE_CHECK_DEV(vtx.m_vTexCoord.IsValid(), "Kraut::Mesh::AddVertex: TexCoord is degenerate.");

    const aeUInt32 uiSupposedIndex = uiFirstVertex + (y * uiWidth) + x;

    if (m_Vertices.size() <= uiSupposedIndex)
    {
      m_Vertices.resize(uiSupposedIndex + 1);
      m_Vertices[uiSupposedIndex] = vtx;
    }
    else
    {
      if (m_Vertices[uiSupposedIndex].m_vPosition.IsZeroVector())
      {
        m_Vertices[uiSupposedIndex] = vtx;
      }
      else
      {
        m_Vertices[uiSupposedIndex].m_vNormal += vtx.m_vNormal;
        m_Vertices[uiSupposedIndex].m_vBiTangent += vtx.m_vBiTangent;
      }
    }

    return uiSupposedIndex;
  }

  void Mesh::Clear()
  {
    m_Vertices.clear();
    m_Triangles.clear();
  }

  aeUInt32 Mesh::GetNumTriangles() const
  {
    return (aeUInt32)m_Triangles.size();
  }

  void Mesh::GenerateVertexNormals()
  {
    // reset all normals
    const aeUInt32 uiVertices = m_Vertices.size();
    for (aeUInt32 v = 0; v < uiVertices; ++v)
    {
      m_Vertices[v].m_vNormal.SetZero();
    }

    // now go through all triangles, compute their normals, add them to the vertex normals
    const aeUInt32 uiTriangles = m_Triangles.size();
    for (aeUInt32 t = 0; t < uiTriangles; ++t)
    {
      const Kraut::Triangle& tri = m_Triangles[t];

      aeVec3 pos[3];

      for (aeUInt32 v = 0; v < 3; ++v)
      {
        pos[v] = m_Vertices[tri.m_uiVertexIDs[v]].m_vPosition;
      }

      aePlane p(pos);

      for (aeUInt32 v = 0; v < 3; ++v)
      {
        const aeUInt32 vtxIdx = tri.m_uiVertexIDs[v];
        const aeUInt32 sharedIdx = m_Vertices[vtxIdx].m_iSharedVertex;

        m_Vertices[sharedIdx].m_vNormal += p.m_vNormal;
      }
    }

    // re-normalize all normals
    // compute bi-tangents
    for (aeUInt32 v = 0; v < uiVertices; ++v)
    {
      auto& vtx = m_Vertices[v];

      if (!vtx.m_vNormal.IsZeroVector())
      {
        vtx.m_vNormal.Normalize();
        vtx.m_vBiTangent = vtx.m_vNormal.Cross(vtx.m_vTangent).GetNormalized();
        vtx.m_vTangent = vtx.m_vBiTangent.Cross(vtx.m_vNormal).GetNormalized();
      }
    }

    // now gather all the smooth normals for the triangles
    for (aeUInt32 t = 0; t < uiTriangles; ++t)
    {
      const Kraut::Triangle& tri = m_Triangles[t];

      for (aeUInt32 v = 0; v < 3; ++v)
      {
        const aeUInt32 idx = tri.m_uiVertexIDs[v];
        const auto& sharedvtx = m_Vertices[m_Vertices[idx].m_iSharedVertex];
        auto& vtx = m_Vertices[idx];

        vtx.m_vNormal = sharedvtx.m_vNormal;
        vtx.m_vTangent = sharedvtx.m_vTangent;
        vtx.m_vBiTangent = sharedvtx.m_vBiTangent;
      }
    }
  }

  static float ComputeVertexRingCircumference(const Kraut::SpawnNodeDesc& bnd, aeUInt32 uiFlares, float fMinWidth, float fMaxWidth, float fPosAlongBranch)
  {
    const float fCF = fMinWidth * aeMath::PI() * 2.0f;

    if (uiFlares == 0)
      return fCF;

    const aeUInt32 uiMaxVertices = aeMath::Max<aeUInt32>(4, (aeUInt32)(fCF / 0.1f));

    aeVec3 vDir(aeMath::CosDeg(0), 0, aeMath::SinDeg(0));
    aeVec3 vLastPos = vDir * bnd.GetFlareDistance(fPosAlongBranch, fMinWidth, fMaxWidth, 0.0f);

    const float fAngleStep = 360.0f / uiMaxVertices;

    float fCircumference = 0.0f;

    for (aeUInt32 vert = 1; vert < uiMaxVertices; ++vert)
    {
      const float fDist = bnd.GetFlareDistance(fPosAlongBranch, fMinWidth, fMaxWidth, (float)vert / (float)uiMaxVertices);
      const aeVec3 vPos(aeMath::CosDeg(fAngleStep * vert) * fDist, 0, aeMath::SinDeg(fAngleStep * vert) * fDist);

      fCircumference += (vLastPos - vPos).GetLength();
      vLastPos = vPos;
    }

    return fCircumference;
  }


  void GenerateVertexRing(VertexRing& out_VertexRing, const Kraut::SpawnNodeDesc& spawnDesc, const Kraut::BranchNode* pPrevNode, const Kraut::BranchNode* pCurNode, const Kraut::BranchNode* pNextNode, float fVertexRingDetail, float fPosAlongBranch, const aeVec3& vNormalAnchor, aeUInt32 uiVertices)
  {
    aeVec3 vDirPrev = pCurNode->m_vPosition - pPrevNode->m_vPosition;
    aeVec3 vDirNext = pNextNode->m_vPosition - pCurNode->m_vPosition;

    vDirPrev.NormalizeSafe();
    vDirNext.NormalizeSafe();

    const aeVec3 vBranchDir = (vDirPrev + vDirNext).GetNormalized();

    // used to rotate vectors from the XZ plane to the actual branch node plane
    aeQuaternion q;
    q.CreateQuaternion(aeVec3(0, 1, 0), vBranchDir);

    const float fBranchRadius = aeMath::Max(0.001f, pCurNode->m_fThickness / 2.0f);

    const float fFlareWidth = spawnDesc.GetFlareWidthAt(fPosAlongBranch, fBranchRadius);

    if (uiVertices == 0)
    {
      AE_CHECK_DEV(fVertexRingDetail != 0.0f, "");

      const float fCircumference = ComputeVertexRingCircumference(spawnDesc, spawnDesc.m_uiFlares, fBranchRadius, fFlareWidth, fPosAlongBranch);
      uiVertices = aeMath::Clamp((int)(fCircumference * 2.0f / fVertexRingDetail), 3, 128);
    }

    out_VertexRing.m_Vertices.resize(uiVertices);
    out_VertexRing.m_VertexIDs.resize(uiVertices);
    out_VertexRing.m_Normals.resize(uiVertices);

    // compute the vertex positions (including flares)
    for (aeUInt32 i = 0; i < uiVertices; ++i)
    {
      float fAngle = (360.0f / uiVertices) * i;

      aeVec3 v(aeMath::CosDeg(fAngle), 0, aeMath::SinDeg(fAngle));

      float fNewThickness = spawnDesc.GetFlareDistance(fPosAlongBranch, fBranchRadius, fFlareWidth, (float)i / (float)uiVertices);

      out_VertexRing.m_Vertices[i] = q * (v * fNewThickness) + pCurNode->m_vPosition;
      out_VertexRing.m_VertexIDs[i] = -1;
    }

    aeUInt32 iPrevNode = uiVertices - 1;

    // compute the smooth normals
    for (aeUInt32 i = 0; i < uiVertices; ++i)
    {
      out_VertexRing.m_Normals[i] = out_VertexRing.m_Vertices[i] - vNormalAnchor;
      out_VertexRing.m_Normals[i].NormalizeSafe();

      iPrevNode = i;
    }

    out_VertexRing.m_fDiameter = 0.0f;

    aeVec3 vLast = out_VertexRing.m_Vertices.back();
    for (aeUInt32 i = 0; i < out_VertexRing.m_Vertices.size(); ++i)
    {
      aeVec3 vCur = out_VertexRing.m_Vertices[i];

      out_VertexRing.m_fDiameter += (vCur - vLast).GetLength();

      vLast = vCur;
    }
  }

  void GenerateVertexRing(VertexRing& out_VertexRing, const TreeStructureDesc& treeStructureDesc, const Kraut::BranchStructure& branchStructure, aeInt32 iPrevNodeIdx, aeInt32 iCurNodeIdx, aeInt32 iNextNodeIdx, float fVertexRingDetail, const aeVec3& vNormalAnchor)
  {
    iPrevNodeIdx = aeMath::Max(iPrevNodeIdx, 0);
    iNextNodeIdx = aeMath::Min(iNextNodeIdx, (aeInt32)(branchStructure.m_Nodes.size()) - 1);

    const Kraut::BranchNode* pPrevNode = &branchStructure.m_Nodes[iPrevNodeIdx];
    const Kraut::BranchNode* pCurNode = &branchStructure.m_Nodes[iCurNodeIdx];
    const Kraut::BranchNode* pNextNode = &branchStructure.m_Nodes[iNextNodeIdx];

    const Kraut::SpawnNodeDesc& spawnDesc = treeStructureDesc.m_BranchTypes[branchStructure.m_Type];
    const float fPosAlongBranch = iCurNodeIdx / (float)(branchStructure.m_Nodes.size() - 1);

    Kraut::GenerateVertexRing(out_VertexRing, spawnDesc, pPrevNode, pCurNode, pNextNode, fVertexRingDetail, fPosAlongBranch, vNormalAnchor, 0);
  }

  void AlignVertexRing(VertexRing& vertexRing, const aeVec3& vNodePosition, aeVec3& vRotationalDir)
  {
    // not used anymore, probably because of flare-rotations
    return;

    aeVec3 vRotationalDir2 = (vertexRing.m_Vertices[0] - vNodePosition).GetNormalizedSafe();

    if (vRotationalDir2.IsZeroVector())
      return;

    const aePlane SlicePlane(vertexRing.m_Vertices[0], vertexRing.m_Vertices[1], vertexRing.m_Vertices[2]);

    if (!SlicePlane.m_vNormal.IsValid())
      return;

    aeVec3 vProjectedRotDir = vRotationalDir;
    vProjectedRotDir.MakeOrthogonalTo(SlicePlane.m_vNormal);

    aeQuaternion qRotateRing;
    qRotateRing.CreateQuaternion(vRotationalDir2, vProjectedRotDir);

    for (aeUInt32 i = 0; i < vertexRing.m_Vertices.size(); ++i)
    {
      vertexRing.m_Vertices[i] = qRotateRing * (vertexRing.m_Vertices[i] - vNodePosition) + vNodePosition;
      vertexRing.m_Normals[i] = qRotateRing * vertexRing.m_Normals[i];

      AE_CHECK_DEV(vertexRing.m_Vertices[i].IsValid(), "AlignVertexRing: Position is degenerate.");
      AE_CHECK_DEV(vertexRing.m_Normals[i].IsValid(), "AlignVertexRing: Normal is degenerate.");
    }

    vRotationalDir = vRotationalDir2;
  }

} // namespace Kraut
