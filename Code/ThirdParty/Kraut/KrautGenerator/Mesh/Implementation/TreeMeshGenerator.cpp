#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/LodDesc.h>
#include <KrautGenerator/Description/TreeStructureDesc.h>
#include <KrautGenerator/Lod/TreeStructureLod.h>
#include <KrautGenerator/Lod/TreeStructureLodGenerator.h>
#include <KrautGenerator/Mesh/BranchMesh.h>
#include <KrautGenerator/Mesh/TreeMesh.h>
#include <KrautGenerator/Mesh/TreeMeshGenerator.h>
#include <KrautGenerator/TreeStructure/TreeStructure.h>

namespace Kraut
{
  void TreeMeshGenerator::GenerateTreeMesh()
  {
    const bool bTrunkCap = false;
    const bool bBranchCap = false;

    const auto& treeStructure = *m_pTreeStructure;
    const auto& treeStructureLod = *m_pTreeStructureLod;
    const auto& treeStructureDesc = *m_pTreeStructureDesc;
    const auto& lodDesc = *m_pLodDesc;

    m_pTreeMesh->m_BranchMeshes.clear();
    m_pTreeMesh->m_BranchMeshes.resize(treeStructure.m_BranchStructures.size());

    for (aeUInt32 b = 0; b < treeStructure.m_BranchStructures.size(); ++b)
    {
      if (treeStructure.m_BranchStructures[b].m_Type == Kraut::BranchType::None)
        continue;

      const Kraut::BranchStructureLod& branchStructureLod = treeStructureLod.m_BranchLODs[b];
      const Kraut::BranchStructure& branchStructure = treeStructure.m_BranchStructures[b];
      const Kraut::SpawnNodeDesc& spawnDesc = treeStructureDesc.m_BranchTypes[branchStructure.m_Type];

      Kraut::BranchMesh& mesh = m_pTreeMesh->m_BranchMeshes[b];

      {
        mesh.m_Mesh[Kraut::BranchGeometryType::Branch].Clear();

        if (spawnDesc.m_bEnable[Kraut::BranchGeometryType::Branch])
        {
          GenerateBranchTriangles(mesh, treeStructureDesc, branchStructure, branchStructureLod, lodDesc);
        }
      }

      {
        mesh.m_Mesh[Kraut::BranchGeometryType::Frond].Clear();

        if (spawnDesc.m_bEnable[Kraut::BranchGeometryType::Frond])
        {
          if (spawnDesc.m_BranchTypeMode == Kraut::BranchTypeMode::Umbrella)
          {
            GenerateUmbrellaTriangles(mesh, treeStructureDesc, treeStructure, treeStructureLod, b);
          }
          else
          {
            GenerateAllFrondTriangles(mesh, treeStructureDesc, branchStructure, branchStructureLod, lodDesc);
          }
        }
      }

      {
        mesh.m_Mesh[Kraut::BranchGeometryType::Leaf].Clear();

        if (spawnDesc.m_bEnable[Kraut::BranchGeometryType::Leaf])
        {
          GenerateLeafTriangles(mesh, treeStructureDesc, branchStructure, branchStructureLod, lodDesc);
        }
      }
    }
  }
} // namespace Kraut
