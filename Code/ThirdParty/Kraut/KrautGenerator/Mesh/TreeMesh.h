#pragma once

#include <KrautFoundation/Containers/Array.h>
#include <KrautGenerator/Mesh/BranchMesh.h>

namespace Kraut
{
  using namespace AE_NS_FOUNDATION;

  struct KRAUT_DLL TreeMesh
  {
    TreeMesh();
    ~TreeMesh();
    TreeMesh(TreeMesh&&) = delete;
    void operator=(TreeMesh&&) = delete;

    void Clear();

    aeUInt32 GetNumTriangles() const;
    aeUInt32 GetNumTriangles(Kraut::BranchGeometryType::Enum geometryType) const;

    // the index of each BranchMesh correlates 1:1 to TreeStructure::m_BranchStructures and can be used
    // to look up things like the branch type and material from the TreeStructure
    aeArray<Kraut::BranchMesh> m_BranchMeshes;
  };
} // namespace Kraut
