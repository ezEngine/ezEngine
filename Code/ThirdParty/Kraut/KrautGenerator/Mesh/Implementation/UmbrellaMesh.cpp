#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/TreeStructureDesc.h>
#include <KrautGenerator/Lod/BranchStructureLod.h>
#include <KrautGenerator/Lod/TreeStructureLod.h>
#include <KrautGenerator/Mesh/BranchMesh.h>
#include <KrautGenerator/Mesh/Mesh.h>
#include <KrautGenerator/Mesh/TreeMeshGenerator.h>
#include <KrautGenerator/TreeStructure/BranchNode.h>
#include <KrautGenerator/TreeStructure/BranchStructure.h>
#include <KrautGenerator/TreeStructure/TreeStructure.h>

namespace Kraut
{
  static void ComputeNormals(const aeArray<aeVec3>& Positions, aeArray<aeVec3>& out_Normals, const aeMatrix& mRot, const aeMatrix& mInvRot)
  {
    aeHybridArray<aeVec3, 64> OrthoDirs;

    out_Normals.clear(false);
    out_Normals.resize(Positions.size(), aeVec3::ZeroVector());
    OrthoDirs.resize(Positions.size());

    OrthoDirs.front().SetVector(0.0f);

    aeVec3 vLastOrtho(0.0f);

    // first compute the orthogonal vectors for each point
    // for every point that cannot be compute properly, pass through the last valid orthogonal vector
    for (aeUInt32 i = 0; i < Positions.size(); ++i)
    {
      const aeVec3 vPrevPos = mInvRot * Positions[i];
      const aeVec3 vPos = Positions[i];
      const aeVec3 vNextPos = mRot * Positions[i];

      const aeVec3 vOrtho1 = vNextPos - vPos;
      const aeVec3 vOrtho2 = vPos - vPrevPos;
      const aeVec3 vOrtho = vOrtho1 + vOrtho2;

      if (vOrtho.GetLengthSquared() > 0.00001f)
      {
        vLastOrtho = vOrtho;
        OrthoDirs[i] = vOrtho;
      }
      else
        OrthoDirs[i] = vLastOrtho;
    }

    vLastOrtho = OrthoDirs.back();
    for (aeInt32 i = Positions.size() - 2; i >= 0; --i)
    {
      if (OrthoDirs[i].IsZeroVector())
        OrthoDirs[i] = vLastOrtho;
      else
        vLastOrtho = OrthoDirs[i];
    }

    // now we have all the orthogonal vectors, compute the smooth normals now
    // first do this for all points except the first and last
    for (aeUInt32 i = 1; i < Positions.size() - 1; ++i)
    {
      const aeVec3 vPrevPos = Positions[i - 1];
      const aeVec3 vPos = Positions[i];
      const aeVec3 vNextPos = Positions[i + 1];

      const aeVec3 vDir1 = (vPos - vPrevPos).GetNormalized();
      const aeVec3 vDir2 = (vNextPos - vPos).GetNormalized();
      const aeVec3 vAvgDir = (vDir1 + vDir2).GetNormalized();

      out_Normals[i] = OrthoDirs[i].Cross(vAvgDir).GetNormalized();
    }

    // special handling of the first and last position
    {
      const aeVec3 vDirStart = (Positions[1] - Positions[0]);
      out_Normals[0] = OrthoDirs[0].Cross(vDirStart).GetNormalized();
    }

    {
      const aeVec3 vDirEnd = (Positions.back() - Positions[Positions.size() - 2]);
      out_Normals.back() = OrthoDirs[0].Cross(vDirEnd).GetNormalized();
    }

    // all normals that vary too strongly from the previous ones get flipped to ensure one smooth ride
    for (aeUInt32 i = 1; i < out_Normals.size(); ++i)
    {
      if (out_Normals[i].Dot(out_Normals[i - 1]) < 0)
        out_Normals[i] = -out_Normals[i];
    }
  }

  static aeVec3 GetParentAxis(const Kraut::TreeStructure& treeStructure, const Kraut::BranchStructure& branchStructure)
  {
    const aeVec3 vOrigin = branchStructure.m_Nodes[0].m_vPosition;
    aeVec3 vParentDirection(0, 1, 0);

    if (branchStructure.m_iParentBranchID >= 0)
    {
      const Kraut::BranchStructure& parentBranch = treeStructure.m_BranchStructures[branchStructure.m_iParentBranchID];

      const aeVec3 vPrevOrigin = parentBranch.m_Nodes[branchStructure.m_uiParentBranchNodeID - 1].m_vPosition;
      vParentDirection = (vOrigin - vPrevOrigin).GetNormalized();
    }

    return vParentDirection;
  }

  static aeMatrix GetRotationMatrix(const Kraut::TreeStructure& treeStructure, const Kraut::BranchStructure& branchStructure, float fRotation)
  {
    const aeVec3 vOrigin = branchStructure.m_Nodes[0].m_vPosition;
    const aeVec3 vAxis = GetParentAxis(treeStructure, branchStructure);

    aeMatrix mRotation, mTransI, mTrans;
    mTransI.SetTranslationMatrix(-vOrigin);
    mTrans.SetTranslationMatrix(vOrigin);
    mRotation.SetRotationMatrix(vAxis, fRotation);

    return mTrans * mRotation * mTransI;
  }

  static void GetBranchSkeleton(const Kraut::BranchStructure& branchStructure, const Kraut::BranchStructureLod& branchStructureLod, aeArray<aeVec3>& out_Positions, aeArray<aeUInt32>& out_NodeIDs)
  {
    out_Positions.resize(branchStructureLod.m_NodeIDs.size());
    out_NodeIDs.resize(branchStructureLod.m_NodeIDs.size());

    for (aeUInt32 i = 0; i < branchStructureLod.m_NodeIDs.size(); ++i)
    {
      out_NodeIDs[i] = branchStructureLod.m_NodeIDs[i];
      out_Positions[i] = branchStructure.m_Nodes[out_NodeIDs[i]].m_vPosition;
    }
  }

  static void RotateBranchSkeleton(aeArray<aeVec3>& inout_Positions, aeArray<aeVec3>& inout_Normals, const aeMatrix& mRot)
  {
    for (aeUInt32 i = 0; i < inout_Positions.size(); ++i)
    {
      inout_Positions[i] = mRot * inout_Positions[i];
      inout_Normals[i] = mRot.TransformDirection(inout_Normals[i]);
    }
  }

  static void FixNormals(const aeArray<aeVec3>& Positions, aeArray<aeVec3>& inout_Normals, const aeMatrix& mRot)
  {
    aeVec3 vTriangle[3];
    vTriangle[0] = Positions[0];
    vTriangle[1] = Positions[1];
    vTriangle[2] = mRot * Positions[1];

    aeVec3 vTriNormal;
    vTriNormal.CalculateNormal(vTriangle[0], vTriangle[1], vTriangle[2]);

    // this checks whether the smooth normals along the branch skeleton are inverted, compared to the triangles normals
    if (vTriNormal.Dot(inout_Normals[0]) < 0)
    {
      // if so, flip all the normals

      for (aeUInt32 i = 0; i < inout_Normals.size(); ++i)
        inout_Normals[i] = -inout_Normals[i];
    }
  }

  void TreeMeshGenerator::AddUmbrellaTriangles(Kraut::BranchMesh& mesh0, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::BranchStructure& branchStructure, const aeArray<aeVec3>& Positions1, const aeArray<aeVec3>& Positions2, const aeArray<aeVec3>& Normals1, const aeArray<aeVec3>& Normals2, aeUInt32 uiFirstVertex, aeUInt32 uiSlice, aeUInt32 uiMaxSlices, float fTexCoordBase, float fTexCoordFraction, const aeVec3& vTexCoordDir0, const aeVec3& vTexCoordDir1, const aeArray<aeUInt32>& nodeIDs)
  {
    const Kraut::SpawnNodeDesc& spawnDesc = treeStructureDesc.m_BranchTypes[branchStructure.m_Type];

    const bool bTexCoordsTopDown = spawnDesc.m_fTextureRepeat == 0.0f;

    const float fFrondHeightFactor = spawnDesc.m_fFrondHeight;

    const aeUInt8 uiTilingX = spawnDesc.m_uiTextureTilingX[Kraut::BranchGeometryType::Frond];
    const aeUInt8 uiTilingY = spawnDesc.m_uiTextureTilingY[Kraut::BranchGeometryType::Frond];

    const float fTextureWidth = 1.0f / uiTilingX;
    const float fTextureHeight = 1.0f / uiTilingY;
    const float fTextureOffsetX = fTextureWidth * (branchStructure.m_uiFrondTextureVariation % uiTilingX);
    const float fTextureOffsetY = fTextureHeight * ((branchStructure.m_uiFrondTextureVariation / uiTilingX) % uiTilingY);

    Kraut::Triangle t;
    Kraut::Vertex vtx[3];

    for (int i = 0; i < 3; ++i)
    {
      vtx[i].m_uiColorVariation = (aeUInt8)(branchStructure.m_fFrondColorVariation * 255.0f);
      vtx[i].m_vBiTangent.SetZero();
      vtx[i].m_vTangent.SetZero();
      vtx[i].m_vTexCoord.SetVector(0, 0, 1);
    }

    t.m_uiPickingSubID = 0;

    Kraut::Mesh& mesh = mesh0.m_Mesh[Kraut::BranchGeometryType::Frond];

    const float fPosAlongUmbrella0 = (float)(uiSlice + 0) / (float)(uiMaxSlices);
    const float fPosAlongUmbrella1 = (float)(uiSlice + 1) / (float)(uiMaxSlices);

    const float fBaseContourValue = spawnDesc.m_FrondContour.GetValueAt(0.0f);

    const float fContour0 = spawnDesc.m_FrondContour.GetValueAtMirrored(fPosAlongUmbrella0) - fBaseContourValue;
    const float fContour1 = spawnDesc.m_FrondContour.GetValueAtMirrored(fPosAlongUmbrella1) - fBaseContourValue;

    float fTexCoordU[2];
    fTexCoordU[0] = fTexCoordBase + fTexCoordFraction * uiSlice;
    fTexCoordU[1] = fTexCoordBase + fTexCoordFraction * (uiSlice + 1);

    float fCurBranchLength0 = 0.0f;
    float fCurBranchLength1 = 0.0f;

    float fBranchLength = 0.0f;
    for (aeUInt32 i = 0; i < Positions1.size() - 1; ++i)
      fBranchLength += (Positions1[i + 1] - Positions1[i]).GetLength();

    for (aeUInt32 i = 0; i < Positions1.size() - 1; ++i)
    {
      fCurBranchLength1 = fCurBranchLength0 + (Positions1[i + 1] - Positions1[i]).GetLength();

      const float fTexCoordDiv0 = fCurBranchLength0 / fBranchLength;
      const float fTexCoordDiv1 = fCurBranchLength1 / fBranchLength;

      const float fFrondHeight0 = fFrondHeightFactor * spawnDesc.m_FrondHeight.GetValueAt(fTexCoordDiv0);
      const float fFrondHeight1 = fFrondHeightFactor * spawnDesc.m_FrondHeight.GetValueAt(fTexCoordDiv1);

      aeVec3 bt[2];

      float fTexCoordQ[2];
      fTexCoordQ[0] = (Positions1[i + 0] - Positions2[i + 0]).GetLength();
      fTexCoordQ[1] = (Positions1[i + 1] - Positions2[i + 1]).GetLength();

      float fTexCoordV[2];
      fTexCoordV[0] = fTexCoordDiv0;
      fTexCoordV[1] = fTexCoordDiv1;

      bt[0] = (Positions1[i + 1] - Positions1[i + 0]).GetNormalized();
      bt[1] = (Positions2[i + 1] - Positions2[i + 0]).GetNormalized();

      aeVec3 vTexCoord[4];
      vTexCoord[0] = vTexCoordDir0 * fTexCoordDiv0;
      vTexCoord[1] = vTexCoordDir0 * fTexCoordDiv1;
      vTexCoord[2] = vTexCoordDir1 * fTexCoordDiv1;
      vTexCoord[3] = vTexCoordDir1 * fTexCoordDiv0;

      vTexCoord[0] = vTexCoord[0] * 0.5f + aeVec3(0.5f);
      vTexCoord[1] = vTexCoord[1] * 0.5f + aeVec3(0.5f);
      vTexCoord[2] = vTexCoord[2] * 0.5f + aeVec3(0.5f);
      vTexCoord[3] = vTexCoord[3] * 0.5f + aeVec3(0.5f);

      vTexCoord[0] = vTexCoord[0].CompMult(aeVec3(fTextureWidth, fTextureHeight)) + aeVec3(fTextureOffsetX, fTextureOffsetY);
      vTexCoord[1] = vTexCoord[1].CompMult(aeVec3(fTextureWidth, fTextureHeight)) + aeVec3(fTextureOffsetX, fTextureOffsetY);
      vTexCoord[2] = vTexCoord[2].CompMult(aeVec3(fTextureWidth, fTextureHeight)) + aeVec3(fTextureOffsetX, fTextureOffsetY);
      vTexCoord[3] = vTexCoord[3].CompMult(aeVec3(fTextureWidth, fTextureHeight)) + aeVec3(fTextureOffsetX, fTextureOffsetY);

      vTexCoord[0].z = 1;
      vTexCoord[1].z = 1;
      vTexCoord[2].z = 1;
      vTexCoord[3].z = 1;

      vtx[0].m_uiBranchNodeIdx = nodeIDs[i + 0];
      vtx[1].m_uiBranchNodeIdx = nodeIDs[i + 1];
      vtx[2].m_uiBranchNodeIdx = nodeIDs[i + 1];

      vtx[0].m_vPosition = Positions1[i + 0];
      vtx[1].m_vPosition = Positions1[i + 1];
      vtx[2].m_vPosition = Positions2[i + 1];


      aeVec3 vNormal;
      vNormal.CalculateNormalSafe(vtx[0].m_vPosition, vtx[1].m_vPosition, vtx[2].m_vPosition);

      vtx[0].m_vNormal = Normals1[i + 0];
      vtx[1].m_vNormal = Normals1[i + 1];
      vtx[2].m_vNormal = Normals2[i + 1];

      vtx[0].m_vBiTangent = bt[0];
      vtx[1].m_vBiTangent = bt[0];
      vtx[2].m_vBiTangent = bt[1];

      vtx[0].m_vTexCoord.SetVector(0, 0, 1);
      vtx[1].m_vTexCoord.SetVector(0, 0, 1);
      vtx[2].m_vTexCoord.SetVector(0, 0, 1);

      if (!bTexCoordsTopDown)
      {
        vtx[0].m_vTexCoord.x = (fTexCoordU[0] * fTextureWidth) + fTextureOffsetX;
        vtx[1].m_vTexCoord.x = (fTexCoordU[0] * fTextureWidth) + fTextureOffsetX;
        vtx[2].m_vTexCoord.x = (fTexCoordU[1] * fTextureWidth) + fTextureOffsetX;

        vtx[0].m_vTexCoord.y = (fTexCoordV[0] * fTextureHeight) + fTextureOffsetY;
        vtx[1].m_vTexCoord.y = (fTexCoordV[1] * fTextureHeight) + fTextureOffsetY;
        vtx[2].m_vTexCoord.y = (fTexCoordV[1] * fTextureHeight) + fTextureOffsetY;
      }
      else
      {
        vtx[0].m_vTexCoord = vTexCoord[0];
        vtx[1].m_vTexCoord = vTexCoord[1];
        vtx[2].m_vTexCoord = vTexCoord[2];
      }

      vtx[0].m_vTexCoord *= fTexCoordQ[0];
      vtx[1].m_vTexCoord *= fTexCoordQ[1];
      vtx[2].m_vTexCoord *= fTexCoordQ[1];

      vtx[0].m_vPosition += vtx[0].m_vNormal * fFrondHeight0 * fContour0;
      vtx[1].m_vPosition += vtx[1].m_vNormal * fFrondHeight1 * fContour0;
      vtx[2].m_vPosition += vtx[2].m_vNormal * fFrondHeight1 * fContour1;

      if ((uiSlice == 0) || (uiSlice == uiMaxSlices - 1))
      {
        vtx[0].m_vNormal = Normals1[i + 0];
        vtx[1].m_vNormal = Normals1[i + 1];
        vtx[2].m_vNormal = Normals2[i + 1];
      }
      else
      {
        vtx[0].m_vNormal = vNormal;
        vtx[1].m_vNormal = vNormal;
        vtx[2].m_vNormal = vNormal;
      }

      t.m_uiVertexIDs[0] = mesh.AddVertex(vtx[0], i + 0, Positions1.size(), uiSlice + 0, uiFirstVertex);
      t.m_uiVertexIDs[1] = mesh.AddVertex(vtx[1], i + 1, Positions1.size(), uiSlice + 0, uiFirstVertex);
      t.m_uiVertexIDs[2] = mesh.AddVertex(vtx[2], i + 1, Positions1.size(), uiSlice + 1, uiFirstVertex);

      mesh.m_Triangles.push_back(t);

      vtx[0].m_uiBranchNodeIdx = nodeIDs[i + 0];
      vtx[1].m_uiBranchNodeIdx = nodeIDs[i + 1];
      vtx[2].m_uiBranchNodeIdx = nodeIDs[i + 0];

      vtx[0].m_vPosition = Positions1[i + 0];
      vtx[1].m_vPosition = Positions2[i + 1];
      vtx[2].m_vPosition = Positions2[i + 0];

      vtx[0].m_vNormal = Normals1[i + 0];
      vtx[1].m_vNormal = Normals2[i + 1];
      vtx[2].m_vNormal = Normals2[i + 0];

      vtx[0].m_vBiTangent = bt[0];
      vtx[1].m_vBiTangent = bt[1];
      vtx[2].m_vBiTangent = bt[1];

      vtx[0].m_vTexCoord.SetVector(0, 0, 1);
      vtx[1].m_vTexCoord.SetVector(0, 0, 1);
      vtx[2].m_vTexCoord.SetVector(0, 0, 1);

      if (!bTexCoordsTopDown)
      {
        vtx[0].m_vTexCoord.x = fTexCoordU[0];
        vtx[1].m_vTexCoord.x = fTexCoordU[1];
        vtx[2].m_vTexCoord.x = fTexCoordU[1];

        vtx[0].m_vTexCoord.y = fTexCoordV[0];
        vtx[1].m_vTexCoord.y = fTexCoordV[1];
        vtx[2].m_vTexCoord.y = fTexCoordV[0];
      }
      else
      {
        vtx[0].m_vTexCoord = vTexCoord[0];
        vtx[1].m_vTexCoord = vTexCoord[2];
        vtx[2].m_vTexCoord = vTexCoord[3];
      }

      vtx[0].m_vTexCoord *= fTexCoordQ[0];
      vtx[1].m_vTexCoord *= fTexCoordQ[1];
      vtx[2].m_vTexCoord *= fTexCoordQ[0];

      vtx[0].m_vPosition += vtx[0].m_vNormal * fFrondHeight0 * fContour0;
      vtx[1].m_vPosition += vtx[1].m_vNormal * fFrondHeight1 * fContour1;
      vtx[2].m_vPosition += vtx[2].m_vNormal * fFrondHeight0 * fContour1;


      vNormal.CalculateNormalSafe(vtx[0].m_vPosition, vtx[1].m_vPosition, vtx[2].m_vPosition);

      if ((uiSlice == 0) || (uiSlice == uiMaxSlices - 1))
      {
        vtx[0].m_vNormal = Normals1[i + 0];
        vtx[1].m_vNormal = Normals2[i + 1];
        vtx[2].m_vNormal = Normals2[i + 0];
      }
      else
      {
        vtx[0].m_vNormal = vNormal;
        vtx[1].m_vNormal = vNormal;
        vtx[2].m_vNormal = vNormal;
      }

      t.m_uiVertexIDs[0] = mesh.AddVertex(vtx[0], i + 0, Positions1.size(), uiSlice + 0, uiFirstVertex);
      t.m_uiVertexIDs[1] = mesh.AddVertex(vtx[1], i + 1, Positions1.size(), uiSlice + 1, uiFirstVertex);
      t.m_uiVertexIDs[2] = mesh.AddVertex(vtx[2], i + 0, Positions1.size(), uiSlice + 1, uiFirstVertex);

      mesh.m_Triangles.push_back(t);

      fCurBranchLength0 = fCurBranchLength1;
    }
  }

  void TreeMeshGenerator::GenerateUmbrellaTriangles(Kraut::BranchMesh& mesh, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::TreeStructure& treeStructure, const Kraut::TreeStructureLod& treeStructureLod, aeUInt32 uiBranch)
  {
    const Kraut::BranchStructure& branchStructure = treeStructure.m_BranchStructures[uiBranch];

    if (branchStructure.m_iUmbrellaBuddyID == -1)
      return;

    const Kraut::SpawnNodeDesc& spawnDesc = treeStructureDesc.m_BranchTypes[branchStructure.m_Type];
    const Kraut::BranchStructure& buddyBranchStructure = treeStructure.m_BranchStructures[branchStructure.m_iUmbrellaBuddyID];
    const Kraut::BranchStructureLod& branchLod = treeStructureLod.m_BranchLODs[uiBranch];
    const Kraut::BranchStructureLod& buddyLod = treeStructureLod.m_BranchLODs[branchStructure.m_iUmbrellaBuddyID];

    if (branchLod.m_NodeIDs.size() != buddyLod.m_NodeIDs.size())
    {
      AE_CHECK_DEV(false, "LODs are not generated equally.");
      return;
    }

    const float fRotation0 = buddyBranchStructure.m_fUmbrellaBranchRotation;
    const float fRotation1 = (aeMath::IsFloatEqual(branchStructure.m_fUmbrellaBranchRotation, 0.0f, 0.0001f)) ? 360.0f : branchStructure.m_fUmbrellaBranchRotation;
    const float fDiffRotation = fRotation1 - fRotation0;

    const float fRotationStep = fDiffRotation / aeMath::Max<aeInt32>(1, spawnDesc.m_uiFrondDetail);
    const aeMatrix mRot = GetRotationMatrix(treeStructure, branchStructure, fRotationStep);
    const aeMatrix mInvRot = GetRotationMatrix(treeStructure, branchStructure, -fRotationStep);

    aeArray<aeVec3> Positions1, Positions2;
    aeArray<aeUInt32> NodeIDs;
    GetBranchSkeleton(branchStructure, branchLod, Positions1, NodeIDs);

    Positions1[0] += (Positions1[1] - Positions1[0]).GetNormalized() * 0.01f;

    aeArray<aeVec3> Normals1, Normals2;
    ComputeNormals(Positions1, Normals1, mRot, mInvRot);

    FixNormals(Positions1, Normals1, mRot);

    const aeUInt32 uiMaxSlices = aeMath::Max<aeInt32>(1, spawnDesc.m_uiFrondDetail);
    const float fTexCoordBase = 0.0f;
    const float fTexCoordFraction = spawnDesc.m_fTextureRepeat / uiMaxSlices;
    const aeUInt32 uiFirstVertex = mesh.m_Mesh[Kraut::BranchGeometryType::Frond].m_Vertices.size();

    for (aeUInt32 i = 0; i < uiMaxSlices; ++i)
    {
      const aeVec3 vTexCoordDir0(aeMath::CosDeg(fRotation0 + (i + 0) * fRotationStep), aeMath::SinDeg(fRotation0 + (i + 0) * fRotationStep), 0.0f);
      const aeVec3 vTexCoordDir1(aeMath::CosDeg(fRotation0 + (i + 1) * fRotationStep), aeMath::SinDeg(fRotation0 + (i + 1) * fRotationStep), 0.0f);

      Positions2 = Positions1;
      Normals2 = Normals1;
      RotateBranchSkeleton(Positions2, Normals2, mRot);

      AddUmbrellaTriangles(mesh, treeStructureDesc, branchStructure, Positions1, Positions2, Normals1, Normals2, uiFirstVertex, i, uiMaxSlices, fTexCoordBase, fTexCoordFraction, vTexCoordDir0, vTexCoordDir1, NodeIDs);

      Positions1 = Positions2;
      Normals1 = Normals2;
    }

    for (aeUInt32 v = uiFirstVertex; v < mesh.m_Mesh[Kraut::BranchGeometryType::Frond].m_Vertices.size(); ++v)
    {
      auto& vtx = mesh.m_Mesh[Kraut::BranchGeometryType::Frond].m_Vertices[v];

      vtx.m_vNormal.Normalize();
      vtx.m_vBiTangent.Normalize();

      vtx.m_vTangent = vtx.m_vBiTangent.Cross(vtx.m_vNormal);
    }
  }

} // namespace Kraut
