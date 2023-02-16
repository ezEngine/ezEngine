#pragma once

#include <KrautGenerator/KrautGeneratorDLL.h>

#include <KrautFoundation/Math/Vec3.h>

namespace Kraut
{
  using namespace AE_NS_FOUNDATION;

  struct TreeStructureDesc;
  struct TreeStructureLod;
  struct TreeStructure;
  struct LodDesc;
  struct BranchStructure;
  struct TreeMesh;

  class KRAUT_DLL TreeMeshGenerator
  {
  public:
    const Kraut::TreeStructureDesc* m_pTreeStructureDesc = nullptr;
    const Kraut::TreeStructureLod* m_pTreeStructureLod = nullptr;
    const Kraut::TreeStructure* m_pTreeStructure = nullptr;
    const Kraut::LodDesc* m_pLodDesc = nullptr;

    TreeMesh* m_pTreeMesh = nullptr;

    void GenerateTreeMesh();

  private:
    void GenerateCapTriangles(Kraut::BranchMesh& mesh, const Kraut::VertexRing& vertexRing);

    /// \brief Adds triangles to connect vertexRing0 and vertexRing1.
    ///
    /// Modifies vertexRing0 and vertexRing1 to write back the shared vertex index.
    void GenerateSegmentTriangles(Kraut::BranchMesh& mesh, const Kraut::BranchStructure& branchStructure, Kraut::VertexRing& vertexRing0, Kraut::VertexRing& vertexRing1, float fTexCoordV0, float fTexCoordV1, float fTexCoordUOffset1, float fTexCoordUOffset2, bool bIsLastSegment, aeUInt32 uiBranchNodeID0, aeUInt32 uiBranchNodeID1);

    void AddStaticLeafTriangles(Kraut::BranchMesh& mesh, const Kraut::BranchStructure& branchStructure, aeUInt32 uiNodeID, float fSize, aeUInt8 uiTilingX, aeUInt8 uiTilingY);

    void AddBillboardLeafTriangles(Kraut::BranchMesh& mesh, const Kraut::BranchStructure& branchStructure, aeUInt32 uiNodeID, float fSize, aeUInt8 uiTilingX, aeUInt8 uiTilingY);

    void AddUmbrellaTriangles(Kraut::BranchMesh& mesh, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::BranchStructure& branchStructure, const aeArray<aeVec3>& Positions1, const aeArray<aeVec3>& Positions2, const aeArray<aeVec3>& Normals1, const aeArray<aeVec3>& Normals2, aeUInt32 uiFirstVertex, aeUInt32 uiSlice, aeUInt32 uiMaxSlices, float fTexCoordBase, float fTexCoordFraction, const aeVec3& vTexCoordDir0, const aeVec3& vTexCoordDir1, const aeArray<aeUInt32>& nodeIDs);

    void GenerateUmbrellaTriangles(Kraut::BranchMesh& mesh, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::TreeStructure& treeStructure, const Kraut::TreeStructureLod& treeStructureLod, aeUInt32 uiBranch);

    void GenerateLeafTriangles(Kraut::BranchMesh& mesh, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::BranchStructure& branchStructure, const Kraut::BranchStructureLod& branchStructureLod, const Kraut::LodDesc& lodDesc);

    void GenerateSingleFrondTriangles(Kraut::BranchMesh& mesh, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::BranchStructure& branchStructure, const Kraut::BranchStructureLod& branchStructureLod, const Kraut::LodDesc& lodDesc, const aeVec3& vStartUpDirection, aeInt32 iCurFrondIndex);

    void GenerateAllFrondTriangles(Kraut::BranchMesh& mesh, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::BranchStructure& branchStructure, const Kraut::BranchStructureLod& branchStructureLod, const Kraut::LodDesc& lodDesc);

    void GenerateBranchTriangles(Kraut::BranchMesh& mesh, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::BranchStructure& branchStructure, const Kraut::BranchStructureLod& branchStructureLod, const Kraut::LodDesc& lodDesc);

    Kraut::RandomNumberGenerator m_RNG;
  };

} // namespace Kraut
