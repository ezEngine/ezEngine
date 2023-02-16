#pragma once

#include <KrautGenerator/KrautGeneratorDLL.h>

#include <KrautFoundation/Math/Vec3.h>

namespace Kraut
{
  using namespace AE_NS_FOUNDATION;

  struct TreeStructureDesc;
  struct LodDesc;
  struct BranchStructure;
  struct BranchStructureLod;
  struct TreeStructureLod;
  struct TreeStructure;
  struct VertexRing;

  class KRAUT_DLL TreeStructureLodGenerator
  {
  public:
    Kraut::TreeStructureLod* m_pTreeStructureLod = nullptr;
    const Kraut::TreeStructure* m_pTreeStructure = nullptr;
    const Kraut::TreeStructureDesc* m_pTreeStructureDesc = nullptr;
    const Kraut::LodDesc* m_pLodDesc = nullptr;

    void GenerateTreeStructureLod();

  private:
    friend class TreeMeshGenerator;

    static void GenerateLodBranchVertexRing(const BranchStructureLod& branchStructureLod, Kraut::VertexRing& out_VertexRing, const Kraut::TreeStructureDesc& structureDesc, const Kraut::BranchStructure& branchStructure, aeUInt32 uiNodeIdx, const aeVec3& vNormalAnchor, float fVertexRingDetail);

    static void GenerateLodTipVertexRing(const BranchStructureLod& branchStructureLod, Kraut::VertexRing& out_VertexRing, const Kraut::TreeStructureDesc& structureDesc, const Kraut::BranchStructure& branchStructure, aeUInt32 uiNodeIdx, const aeVec3& vNormalAnchor, aeUInt32 uiRingVertices);

    /// \brief Sets this LOD up as full detail, meaning no node in the original structure is skipped.
    void GenerateFullDetailLod(const Kraut::TreeStructureDesc& structureDesc, const Kraut::LodDesc& lodDesc, const Kraut::TreeStructure& structure);

    void GenerateTreeStructureLod(const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::LodDesc& lodDesc, const Kraut::TreeStructure& structure);

    void GenerateBranchLod(const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::LodDesc& lodDesc, const Kraut::BranchStructure& branch, Kraut::BranchStructureLod& structureLod);


    /// \brief Sets this LOD up as full detail, meaning no node in the original structure is skipped.
    void GenerateFullDetailLod(BranchStructureLod& branchStructureLod, const Kraut::TreeStructureDesc& structureDesc, const Kraut::LodDesc& lodDesc, const Kraut::BranchStructure& branch);

    void GrowBranchTip(BranchStructureLod& branchStructureLod, const Kraut::TreeStructureDesc& structureDesc, const Kraut::LodDesc& lodDesc, const Kraut::BranchStructure& branch);

    void GenerateLodTipTexCoordV(BranchStructureLod& branchStructureLod, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::BranchStructure& branchStructure, float fVertexRingDetail);
  };

} // namespace Kraut
