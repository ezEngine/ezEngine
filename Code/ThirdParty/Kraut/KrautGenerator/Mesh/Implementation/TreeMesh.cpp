#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/TreeStructureDesc.h>
#include <KrautGenerator/Lod/TreeStructureLod.h>
#include <KrautGenerator/Mesh/TreeMesh.h>
#include <KrautGenerator/TreeStructure/TreeStructure.h>

namespace Kraut
{
  TreeMesh::TreeMesh() = default;
  TreeMesh::~TreeMesh() = default;

  void TreeMesh::Clear()
  {
    m_BranchMeshes.clear();
  }

  aeUInt32 TreeMesh::GetNumTriangles() const
  {
    aeUInt32 result = 0;

    for (aeUInt32 gt = 0; gt < Kraut::BranchGeometryType::ENUM_COUNT; ++gt)
    {
      result += GetNumTriangles(static_cast<Kraut::BranchGeometryType::Enum>(gt));
    }

    return result;
  }

  aeUInt32 TreeMesh::GetNumTriangles(Kraut::BranchGeometryType::Enum geometryType) const
  {
    aeUInt32 result = 0;

    for (aeUInt32 b = 0; b < m_BranchMeshes.size(); ++b)
    {
      result += m_BranchMeshes[b].m_Mesh[geometryType].GetNumTriangles();
    }

    return result;
  }


} // namespace Kraut
