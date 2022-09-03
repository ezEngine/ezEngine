#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/LodDesc.h>
#include <KrautGenerator/Description/TreeStructureDesc.h>
#include <KrautGenerator/Lod/BranchStructureLod.h>
#include <KrautGenerator/Mesh/BranchMesh.h>
#include <KrautGenerator/Mesh/Mesh.h>
#include <KrautGenerator/Mesh/TreeMeshGenerator.h>
#include <KrautGenerator/TreeStructure/BranchNode.h>
#include <KrautGenerator/TreeStructure/BranchStructure.h>

namespace Kraut
{
  static void AddFrondQuads(aeUInt32 uiSlices, aeUInt32 uiNode, const aeArray<aeVec3>& positions, const aeArray<float>& posAlongBranch, const aeArray<aeVec3>& upVectors, const aeArray<aeVec3>& OrthoVectors, const aeArray<float>& NodeWidth, const aeArray<float>& NodeHeight, const Kraut::Curve& Contour, Kraut::Mesh& out_Tris, float fSide, aeUInt32 uiFirstVertex, Kraut::SpawnNodeDesc::FrondContourMode FrondMode, const Kraut::BranchStructure& branchStructure, const Kraut::BranchStructureLod& branchStructureLod, const Kraut::SpawnNodeDesc& spawnDesc, float fFrondFract, float fTextureRepeatDivider, float fBranchLength, aeInt32 iCurFrondIndex, const aeArray<aeUInt32>& branchNodeIDs)
  {
    const float fColorVariation = branchStructure.m_fFrondColorVariation;

    bool bAddPoly0 = true;
    bool bAddPoly1 = true;

    // if the frond is not wide enough at this vertex, cut it off
    if (NodeWidth[uiNode] < 0.01f)
      bAddPoly0 = false;
    if (NodeWidth[uiNode + 1] < 0.01f)
      bAddPoly1 = false;

    if (!bAddPoly0 || !bAddPoly1)
      return;

    const aeUInt32 uiTilingX = spawnDesc.m_uiTextureTilingX[Kraut::BranchGeometryType::Frond];

    // this is used for the texture atlas feature to offset the texcoords
    const float fTextureWidth = 1.0f / uiTilingX;
    const float fTextureOffset = fTextureWidth * ((branchStructure.m_uiFrondTextureVariation + iCurFrondIndex) % uiTilingX);


    const aeVec3 vGrowDir = (positions[uiNode + 1] - positions[uiNode]).GetNormalized();

    const float fAlongBranch0 = posAlongBranch[uiNode];
    const float fAlongBranch1 = posAlongBranch[uiNode + 1];

    const float fTexCoord0 = fAlongBranch0 / fTextureRepeatDivider;
    const float fTexCoord1 = fAlongBranch1 / fTextureRepeatDivider;

    const aeVec3 vStartPos0 = positions[uiNode];
    const aeVec3 vStartPos1 = positions[uiNode + 1];

    float fContourCenter = 0;

    if (FrondMode == Kraut::SpawnNodeDesc::Full)
      fContourCenter = Contour.GetValueAt(0.5f);
    else
      fContourCenter = Contour.GetValueAt(0.0f);

    if ((FrondMode == Kraut::SpawnNodeDesc::InverseSymetric) && (fSide < 0.0f))
      fContourCenter = Contour.m_fMaxValue - fContourCenter;

    Kraut::Triangle t;
    Kraut::Vertex vtx[3];

    for (int i = 0; i < 3; ++i)
      vtx[i].m_uiColorVariation = (aeUInt8)(fColorVariation * 255);

    t.m_uiPickingSubID = uiNode;

    aeVec3 vOffset0(0.0f);
    aeVec3 vOffset1(0.0f);

    if (spawnDesc.m_bAlignFrondsOnSurface)
    {
      // thickness at those nodes
      const float fRadius0 = branchStructure.m_Nodes[branchStructureLod.m_NodeIDs[uiNode + 0]].m_fThickness * 0.5f;
      const float fRadius1 = branchStructure.m_Nodes[branchStructureLod.m_NodeIDs[uiNode + 1]].m_fThickness * 0.5f;

      // the flare width along the branch there
      const float fFlareWidth0 = spawnDesc.GetFlareWidthAt(fAlongBranch0 / fBranchLength, fRadius0);
      const float fFlareWidth1 = spawnDesc.GetFlareWidthAt(fAlongBranch1 / fBranchLength, fRadius1);

      // compute the 'angle' around the branch at that position (each frond has two angles)
      fFrondFract /= 2.0f;
      if (fSide > 0)
        fFrondFract += 0.5f;

      // get the actual distances of the flare mesh at those angles
      const float fFlareDistance0 = spawnDesc.GetFlareDistance(fAlongBranch0 / fBranchLength, fRadius0, fFlareWidth0, fFrondFract);
      const float fFlareDistance1 = spawnDesc.GetFlareDistance(fAlongBranch1 / fBranchLength, fRadius1, fFlareWidth1, fFrondFract);

      // compute the offset
      vOffset0 = fSide * OrthoVectors[uiNode + 0] * fFlareDistance0;
      vOffset1 = fSide * OrthoVectors[uiNode + 1] * fFlareDistance1;

      // reduce the offset to prevent holes due to tessellation/interpolation
      vOffset0 -= vOffset0.GetNormalized() * 0.01f;
      vOffset1 -= vOffset1.GetNormalized() * 0.01f;
    }

    for (aeUInt32 slice = 0; slice < uiSlices; ++slice)
    {
      const float fPos0 = (float)slice / (float)uiSlices;
      const float fPos1 = (float)(slice + 1) / (float)uiSlices;

      float fContour0 = 0;
      (Contour.GetValueAt(0.5f + fSide * fPos0 * 0.5f) - fContourCenter);
      float fContour1 = 0;
      (Contour.GetValueAt(0.5f + fSide * fPos1 * 0.5f) - fContourCenter);

      if (FrondMode == Kraut::SpawnNodeDesc::Full)
      {
        fContour0 = Contour.GetValueAt(0.5f + fSide * fPos0 * 0.5f);
        fContour1 = Contour.GetValueAt(0.5f + fSide * fPos1 * 0.5f);
      }
      else
      {
        fContour0 = Contour.GetValueAt(fPos0);
        fContour1 = Contour.GetValueAt(fPos1);
      }

      if ((FrondMode == Kraut::SpawnNodeDesc::InverseSymetric) && (fSide < 0.0f))
      {
        fContour0 = Contour.m_fMaxValue - fContour0;
        fContour1 = Contour.m_fMaxValue - fContour1;
      }

      fContour0 -= fContourCenter;
      fContour1 -= fContourCenter;


      const aeVec3 vCurPos00 = vOffset0 + vStartPos0 + fSide * OrthoVectors[uiNode] * NodeWidth[uiNode] * fPos0 + upVectors[uiNode] * fContour0 * NodeHeight[uiNode];
      const aeVec3 vCurPos01 = vOffset0 + vStartPos0 + fSide * OrthoVectors[uiNode] * NodeWidth[uiNode] * fPos1 + upVectors[uiNode] * fContour1 * NodeHeight[uiNode];
      const aeVec3 vCurPos10 = vOffset1 + vStartPos1 + fSide * OrthoVectors[uiNode + 1] * NodeWidth[uiNode + 1] * fPos0 + upVectors[uiNode + 1] * fContour0 * NodeHeight[uiNode + 1];
      const aeVec3 vCurPos11 = vOffset1 + vStartPos1 + fSide * OrthoVectors[uiNode + 1] * NodeWidth[uiNode + 1] * fPos1 + upVectors[uiNode + 1] * fContour1 * NodeHeight[uiNode + 1];

      const aeVec3 vBiTangent0 = (vCurPos10 - vCurPos00).GetNormalized();
      const aeVec3 vBiTangent1 = (vCurPos11 - vCurPos01).GetNormalized();

      const float fW0 = NodeWidth[uiNode] * 1;
      const float fW1 = NodeWidth[uiNode + 1] * 1;

      if (bAddPoly0)
      {
        vtx[0].m_uiBranchNodeIdx = branchNodeIDs[uiNode];
        vtx[1].m_uiBranchNodeIdx = branchNodeIDs[uiNode];
        vtx[2].m_uiBranchNodeIdx = branchNodeIDs[uiNode + 1];

        vtx[0].m_vPosition = vCurPos00;
        vtx[1].m_vPosition = vCurPos01;
        vtx[2].m_vPosition = vCurPos10;

        aePlane p(vtx[0].m_vPosition, vtx[1].m_vPosition, vtx[2].m_vPosition);

        if (fSide < 0)
          p.FlipPlane();

        vtx[0].m_vNormal = p.m_vNormal;
        vtx[1].m_vNormal = p.m_vNormal;
        vtx[2].m_vNormal = p.m_vNormal;

        vtx[0].m_vTexCoord.x = (fTextureOffset + (0.5f + fSide * fPos0 * 0.5f) * fTextureWidth) * fW0;
        vtx[1].m_vTexCoord.x = (fTextureOffset + (0.5f + fSide * fPos1 * 0.5f) * fTextureWidth) * fW0;
        vtx[2].m_vTexCoord.x = (fTextureOffset + (0.5f + fSide * fPos0 * 0.5f) * fTextureWidth) * fW1;

        vtx[0].m_vTexCoord.y = fTexCoord0 * fW0;
        vtx[1].m_vTexCoord.y = fTexCoord0 * fW0;
        vtx[2].m_vTexCoord.y = fTexCoord1 * fW1;

        vtx[0].m_vTexCoord.z = fW0;
        vtx[1].m_vTexCoord.z = fW0;
        vtx[2].m_vTexCoord.z = fW1;

        vtx[0].m_vBiTangent = vBiTangent0;
        vtx[1].m_vBiTangent = vBiTangent1;
        vtx[2].m_vBiTangent = vBiTangent0;

        t.m_uiVertexIDs[0] = out_Tris.AddVertex(vtx[0], slice + 0, uiSlices + 1, uiNode + 0, uiFirstVertex);
        t.m_uiVertexIDs[1] = out_Tris.AddVertex(vtx[1], slice + 1, uiSlices + 1, uiNode + 0, uiFirstVertex);
        t.m_uiVertexIDs[2] = out_Tris.AddVertex(vtx[2], slice + 0, uiSlices + 1, uiNode + 1, uiFirstVertex);

        if (fSide < 0)
          t.Flip();

        out_Tris.m_Triangles.push_back(t);
      }

      if (bAddPoly1)
      {
        vtx[0].m_uiBranchNodeIdx = branchNodeIDs[uiNode];
        vtx[1].m_uiBranchNodeIdx = branchNodeIDs[uiNode + 1];
        vtx[2].m_uiBranchNodeIdx = branchNodeIDs[uiNode + 1];

        vtx[0].m_vPosition = vCurPos01;
        vtx[1].m_vPosition = vCurPos11;
        vtx[2].m_vPosition = vCurPos10;

        aePlane p(vtx[0].m_vPosition, vtx[1].m_vPosition, vtx[2].m_vPosition);

        if (fSide < 0)
          p.FlipPlane();

        vtx[0].m_vNormal = p.m_vNormal;
        vtx[1].m_vNormal = p.m_vNormal;
        vtx[2].m_vNormal = p.m_vNormal;

        vtx[0].m_vTexCoord.x = (fTextureOffset + (0.5f + fSide * fPos1 * 0.5f) * fTextureWidth) * fW0;
        vtx[1].m_vTexCoord.x = (fTextureOffset + (0.5f + fSide * fPos1 * 0.5f) * fTextureWidth) * fW1;
        vtx[2].m_vTexCoord.x = (fTextureOffset + (0.5f + fSide * fPos0 * 0.5f) * fTextureWidth) * fW1;

        vtx[0].m_vTexCoord.y = fTexCoord0 * fW0;
        vtx[1].m_vTexCoord.y = fTexCoord1 * fW1;
        vtx[2].m_vTexCoord.y = fTexCoord1 * fW1;

        vtx[0].m_vTexCoord.z = fW0;
        vtx[1].m_vTexCoord.z = fW1;
        vtx[2].m_vTexCoord.z = fW1;

        vtx[0].m_vBiTangent = vBiTangent1;
        vtx[1].m_vBiTangent = vBiTangent0;
        vtx[2].m_vBiTangent = vBiTangent1;

        t.m_uiVertexIDs[0] = out_Tris.AddVertex(vtx[0], slice + 1, uiSlices + 1, uiNode + 0, uiFirstVertex);
        t.m_uiVertexIDs[1] = out_Tris.AddVertex(vtx[1], slice + 1, uiSlices + 1, uiNode + 1, uiFirstVertex);
        t.m_uiVertexIDs[2] = out_Tris.AddVertex(vtx[2], slice + 0, uiSlices + 1, uiNode + 1, uiFirstVertex);

        if (fSide < 0)
          t.Flip();

        out_Tris.m_Triangles.push_back(t);
      }
    }
  }

  static void AddSingleFrond(aeUInt32 uiNode, const aeArray<aeVec3>& Positions, const aeArray<float>& PosAlongBranch, const aeArray<aeVec3>& UpVectors, const aeArray<aeVec3>& OrthoVectors, const aeArray<float>& NodeWidth, Kraut::Mesh& out_Tris, aeUInt32 uiFirstVertex, const Kraut::BranchStructure& branchStructure, float fTextureRepeatDivider, aeUInt32 uiTilingX, aeInt32 iCurFrondIndex, const aeArray<aeUInt32>& branchNodeIDs)
  {
    const float fColorVariation = branchStructure.m_fFrondColorVariation;

    // if the frond is not wide enough at this vertex, cut it off
    if ((NodeWidth[uiNode] < 0.05f) || (NodeWidth[uiNode + 1] < 0.05f))
      return;

    // this is used for the texture atlas feature to offset the texcoords
    const float fTextureWidth = 1.0f / uiTilingX;
    const float fTextureOffset = fTextureWidth * ((branchStructure.m_uiFrondTextureVariation + iCurFrondIndex) % uiTilingX);


    const aeVec3 vGrowDir = (Positions[uiNode + 1] - Positions[uiNode]).GetNormalized();

    const float fAlongBranch0 = PosAlongBranch[uiNode];
    const float fAlongBranch1 = PosAlongBranch[uiNode + 1];

    const float fTexCoord0 = fAlongBranch0 / fTextureRepeatDivider;
    const float fTexCoord1 = fAlongBranch1 / fTextureRepeatDivider;

    const aeVec3 vStartPos0 = Positions[uiNode];
    const aeVec3 vStartPos1 = Positions[uiNode + 1];

    Kraut::Triangle t;
    Kraut::Vertex vtx[3];

    for (int i = 0; i < 3; ++i)
    {
      vtx[i].m_uiColorVariation = (aeUInt8)(fColorVariation * 255);
    }

    t.m_uiPickingSubID = uiNode;

    {
      aeVec3 vCurPos00 = vStartPos0 - OrthoVectors[uiNode] * NodeWidth[uiNode];
      aeVec3 vCurPos01 = vStartPos0 + OrthoVectors[uiNode] * NodeWidth[uiNode];
      aeVec3 vCurPos10 = vStartPos1 - OrthoVectors[uiNode + 1] * NodeWidth[uiNode + 1];
      aeVec3 vCurPos11 = vStartPos1 + OrthoVectors[uiNode + 1] * NodeWidth[uiNode + 1];

      float fWidth0 = (vCurPos01 - vCurPos00).GetLength();
      float fWidth1 = (vCurPos11 - vCurPos10).GetLength();

      const aeVec3 vBiTangent0 = (vCurPos10 - vCurPos00).GetNormalized();
      const aeVec3 vBiTangent1 = (vCurPos11 - vCurPos01).GetNormalized();

      {
        vtx[0].m_uiBranchNodeIdx = branchNodeIDs[uiNode];
        vtx[1].m_uiBranchNodeIdx = branchNodeIDs[uiNode];
        vtx[2].m_uiBranchNodeIdx = branchNodeIDs[uiNode + 1];

        vtx[0].m_vPosition = vCurPos00;
        vtx[1].m_vPosition = vCurPos01;
        vtx[2].m_vPosition = vCurPos10;

        aePlane p(vtx[0].m_vPosition, vtx[1].m_vPosition, vtx[2].m_vPosition);

        vtx[0].m_vNormal = p.m_vNormal;
        vtx[1].m_vNormal = p.m_vNormal;
        vtx[2].m_vNormal = p.m_vNormal;

        vtx[0].m_vTexCoord.x = (fTextureOffset + 0.0f * fTextureWidth) * fWidth0;
        vtx[1].m_vTexCoord.x = (fTextureOffset + 1.0f * fTextureWidth) * fWidth0;
        vtx[2].m_vTexCoord.x = (fTextureOffset + 0.0f * fTextureWidth) * fWidth1;

        vtx[0].m_vTexCoord.y = fTexCoord0 * fWidth0;
        vtx[1].m_vTexCoord.y = fTexCoord0 * fWidth0;
        vtx[2].m_vTexCoord.y = fTexCoord1 * fWidth1;

        vtx[0].m_vTexCoord.z = fWidth0;
        vtx[1].m_vTexCoord.z = fWidth0;
        vtx[2].m_vTexCoord.z = fWidth1;

        vtx[0].m_vBiTangent = vBiTangent0;
        vtx[1].m_vBiTangent = vBiTangent1;
        vtx[2].m_vBiTangent = vBiTangent0;

        t.m_uiVertexIDs[0] = out_Tris.AddVertex(vtx[0], 0 + 0, 2, uiNode + 0, uiFirstVertex);
        t.m_uiVertexIDs[1] = out_Tris.AddVertex(vtx[1], 0 + 1, 2, uiNode + 0, uiFirstVertex);
        t.m_uiVertexIDs[2] = out_Tris.AddVertex(vtx[2], 0 + 0, 2, uiNode + 1, uiFirstVertex);

        out_Tris.m_Triangles.push_back(t);
      }

      {
        vtx[0].m_uiBranchNodeIdx = branchNodeIDs[uiNode];
        vtx[1].m_uiBranchNodeIdx = branchNodeIDs[uiNode + 1];
        vtx[2].m_uiBranchNodeIdx = branchNodeIDs[uiNode + 1];

        vtx[0].m_vPosition = vCurPos01;
        vtx[1].m_vPosition = vCurPos11;
        vtx[2].m_vPosition = vCurPos10;

        aePlane p(vtx[0].m_vPosition, vtx[1].m_vPosition, vtx[2].m_vPosition);

        vtx[0].m_vNormal = p.m_vNormal;
        vtx[1].m_vNormal = p.m_vNormal;
        vtx[2].m_vNormal = p.m_vNormal;

        vtx[0].m_vTexCoord.x = (fTextureOffset + 1.0f * fTextureWidth) * fWidth0;
        vtx[1].m_vTexCoord.x = (fTextureOffset + 1.0f * fTextureWidth) * fWidth1;
        vtx[2].m_vTexCoord.x = (fTextureOffset + 0.0f * fTextureWidth) * fWidth1;

        vtx[0].m_vTexCoord.y = fTexCoord0 * fWidth0;
        vtx[1].m_vTexCoord.y = fTexCoord1 * fWidth1;
        vtx[2].m_vTexCoord.y = fTexCoord1 * fWidth1;

        vtx[0].m_vTexCoord.z = fWidth0;
        vtx[1].m_vTexCoord.z = fWidth1;
        vtx[2].m_vTexCoord.z = fWidth1;

        vtx[0].m_vBiTangent = vBiTangent1;
        vtx[1].m_vBiTangent = vBiTangent0;
        vtx[2].m_vBiTangent = vBiTangent1;

        t.m_uiVertexIDs[0] = out_Tris.AddVertex(vtx[0], 0 + 1, 2, uiNode + 0, uiFirstVertex);
        t.m_uiVertexIDs[1] = out_Tris.AddVertex(vtx[1], 0 + 1, 2, uiNode + 1, uiFirstVertex);
        t.m_uiVertexIDs[2] = out_Tris.AddVertex(vtx[2], 0 + 0, 2, uiNode + 1, uiFirstVertex);

        out_Tris.m_Triangles.push_back(t);
      }
    }
  }

  void TreeMeshGenerator::GenerateSingleFrondTriangles(Kraut::BranchMesh& mesh, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::BranchStructure& branchStructure, const Kraut::BranchStructureLod& branchStructureLod, const Kraut::LodDesc& lodDesc, const aeVec3& vStartUpDirection, aeInt32 iCurFrondIndex)
  {
    const Kraut::SpawnNodeDesc& spawnDesc = treeStructureDesc.m_BranchTypes[branchStructure.m_Type];

    if (branchStructureLod.m_NodeIDs.size() < 2)
      return;

    if ((lodDesc.m_AllowTypes[Kraut::BranchGeometryType::Frond] & (1 << branchStructure.m_Type)) == 0)
      return;

    const float fFrondFract = (float)iCurFrondIndex / (float)spawnDesc.m_uiNumFronds;

    const aeUInt32 uiNodeCount = branchStructureLod.m_NodeIDs.size();

    float fLodBranchLength = 0.0f;
    {
      for (aeUInt32 i = 1; i < uiNodeCount; ++i)
      {
        const aeVec3 v0 = branchStructure.m_Nodes[branchStructureLod.m_NodeIDs[i - 1]].m_vPosition;
        const aeVec3 v1 = branchStructure.m_Nodes[branchStructureLod.m_NodeIDs[i - 0]].m_vPosition;

        fLodBranchLength += (v1 - v0).GetLength();
      }
    }

    const aeVec3 vStartGrowDir = (branchStructure.m_Nodes.back().m_vPosition - branchStructure.m_Nodes[0].m_vPosition).GetNormalized();
    const aeVec3 vStartOrtho = vStartGrowDir.Cross(vStartUpDirection).GetNormalized();


    aeArray<aeVec3> Positions(uiNodeCount);
    aeArray<aeVec3> UpVectors(uiNodeCount);
    aeArray<aeVec3> OrthoVectors(uiNodeCount);
    aeArray<float> NodeWidth(uiNodeCount);
    aeArray<float> NodeHeight(uiNodeCount);
    aeArray<float> PosAlongBranch(uiNodeCount);
    aeArray<aeUInt32> BranchNodeIDs(uiNodeCount);
    float fBranchLength = 0.0f;

    // pre-compute all the node up vectors
    {
      aeVec3 vLastGrowDir = vStartGrowDir;
      UpVectors[0] = vStartOrtho.Cross(vStartGrowDir).GetNormalized();
      OrthoVectors[0] = vStartOrtho;

      for (aeUInt32 i = 1; i < uiNodeCount - 1; ++i)
      {
        const aeVec3 vCurGrowDir = (branchStructure.m_Nodes[branchStructureLod.m_NodeIDs[i + 1]].m_vPosition - branchStructure.m_Nodes[branchStructureLod.m_NodeIDs[i]].m_vPosition).GetNormalized();

        aeQuaternion rot;
        rot.CreateQuaternion(vLastGrowDir, vCurGrowDir);

        // rotate the up vector by the amount of change in the frond direction
        UpVectors[i] = rot * UpVectors[i - 1];
        OrthoVectors[i] = rot * OrthoVectors[i - 1];

        vLastGrowDir = vCurGrowDir;
      }

      // copy the last computed value
      UpVectors.back() = UpVectors[UpVectors.size() - 2];
      OrthoVectors.back() = OrthoVectors[OrthoVectors.size() - 2];
    }

    // now smooth all up vectors
    {
      for (aeUInt32 i = 1; i < uiNodeCount - 1; ++i)
      {
        UpVectors[i] = (UpVectors[i - 1] + UpVectors[i]).GetNormalized();
        OrthoVectors[i] = (OrthoVectors[i - 1] + OrthoVectors[i]).GetNormalized();
      }
    }

    // now compute the width and height for all nodes
    {
      for (aeUInt32 i = 0; i < uiNodeCount; ++i)
      {
        const float fPosAlongBranch = (float)i / (float)(uiNodeCount - 1);

        const aeUInt32 uiNodeID = branchStructureLod.m_NodeIDs[i];

        BranchNodeIDs[i] = uiNodeID;
        Positions[i] = branchStructure.m_Nodes[uiNodeID].m_vPosition;
        NodeWidth[i] = aeMath::Max(0.01f, spawnDesc.m_fFrondWidth * spawnDesc.m_FrondWidth.GetValueAt(fPosAlongBranch));
        NodeHeight[i] = spawnDesc.m_fFrondHeight * spawnDesc.m_FrondHeight.GetValueAt(fPosAlongBranch);
      }
    }

    float fTextureRepeatDivider = spawnDesc.m_fTextureRepeat;

    // compute the branch length (in this LOD)
    {
      for (aeUInt32 i = 1; i < uiNodeCount; ++i)
      {
        fBranchLength += (Positions[i - 1] - Positions[i]).GetLength();
        PosAlongBranch[i] = fBranchLength;
      }

      PosAlongBranch[0] = 0.0f;

      if (spawnDesc.m_fTextureRepeat <= 0.01f)
        fTextureRepeatDivider = fBranchLength;
    }

    aeUInt32 uiFirstVertex0 = mesh.m_Mesh[Kraut::BranchGeometryType::Frond].m_Vertices.size();

    aeInt32 iFrondDetail = spawnDesc.m_bAlignFrondsOnSurface ? aeMath::Max<aeInt32>(1, spawnDesc.m_uiFrondDetail) : spawnDesc.m_uiFrondDetail;
    iFrondDetail -= lodDesc.m_iFrondDetailReduction;
    iFrondDetail = aeMath::Clamp<aeInt32>(iFrondDetail, 0, lodDesc.m_iMaxFrondDetail);

    // use the width value of the second last node also for the last node to prevent extreme texture stretching
    NodeWidth.back() = NodeWidth[NodeWidth.size() - 2];

    // now generate all the fronds
    if (iFrondDetail > 0)
    {
      for (aeUInt32 i = 0; i < uiNodeCount - 1; ++i)
      {
        AddFrondQuads(iFrondDetail, i, Positions, PosAlongBranch, UpVectors, OrthoVectors, NodeWidth, NodeHeight, spawnDesc.m_FrondContour, mesh.m_Mesh[Kraut::BranchGeometryType::Frond], 1.0f, uiFirstVertex0, spawnDesc.m_FrondContourMode, branchStructure, branchStructureLod, spawnDesc, fFrondFract, fTextureRepeatDivider, fBranchLength, iCurFrondIndex, BranchNodeIDs);
      }

      aeUInt32 uiFirstVertex1 = mesh.m_Mesh[Kraut::BranchGeometryType::Frond].m_Vertices.size();
      for (aeUInt32 i = 0; i < uiNodeCount - 1; ++i)
      {
        AddFrondQuads(iFrondDetail, i, Positions, PosAlongBranch, UpVectors, OrthoVectors, NodeWidth, NodeHeight, spawnDesc.m_FrondContour, mesh.m_Mesh[Kraut::BranchGeometryType::Frond], -1.0f, uiFirstVertex1, spawnDesc.m_FrondContourMode, branchStructure, branchStructureLod, spawnDesc, fFrondFract, fTextureRepeatDivider, fBranchLength, iCurFrondIndex, BranchNodeIDs);
      }
    }
    else
    {
      for (aeUInt32 i = 0; i < uiNodeCount - 1; ++i)
      {
        AddSingleFrond(i, Positions, PosAlongBranch, UpVectors, OrthoVectors, NodeWidth, mesh.m_Mesh[Kraut::BranchGeometryType::Frond], uiFirstVertex0, branchStructure, fTextureRepeatDivider, spawnDesc.m_uiTextureTilingX[Kraut::BranchGeometryType::Frond], iCurFrondIndex, BranchNodeIDs);
      }
    }

    for (aeUInt32 v = uiFirstVertex0; v < mesh.m_Mesh[Kraut::BranchGeometryType::Frond].m_Vertices.size(); ++v)
    {
      auto& vtx = mesh.m_Mesh[Kraut::BranchGeometryType::Frond].m_Vertices[v];

      AE_CHECK_DEV(vtx.m_vNormal.IsValid(), "");

      vtx.m_vNormal.NormalizeSafe();
      vtx.m_vBiTangent.NormalizeSafe();

      AE_CHECK_DEV(vtx.m_vNormal.IsValid(), "");

      vtx.m_vTangent = vtx.m_vBiTangent.Cross(vtx.m_vNormal);
    }
  }

  void TreeMeshGenerator::GenerateAllFrondTriangles(Kraut::BranchMesh& mesh, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::BranchStructure& branchStructure, const Kraut::BranchStructureLod& branchStructureLod, const Kraut::LodDesc& lodDesc)
  {
    if (branchStructureLod.m_NodeIDs.size() < 2)
      return;

    const Kraut::SpawnNodeDesc& spawnDesc = treeStructureDesc.m_BranchTypes[branchStructure.m_Type];

    const aeVec3 vStartGrowDir = (branchStructure.m_Nodes[1].m_vPosition - branchStructure.m_Nodes[0].m_vPosition).GetNormalized();

    aeQuaternion qRot;
    qRot.CreateQuaternion(vStartGrowDir, 180.0f / spawnDesc.m_uiNumFronds);

    aeVec3 vUp = branchStructure.m_vLeafUpDirection.GetNormalized();

    for (aeUInt32 frontIdx = 0; frontIdx < spawnDesc.m_uiNumFronds; ++frontIdx)
    {
      GenerateSingleFrondTriangles(mesh, treeStructureDesc, branchStructure, branchStructureLod, lodDesc, vUp, frontIdx);

      vUp = qRot * vUp;
    }
  }
} // namespace Kraut
