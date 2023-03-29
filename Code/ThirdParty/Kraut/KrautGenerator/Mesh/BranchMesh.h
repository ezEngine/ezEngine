#pragma once

#include <KrautGenerator/Description/DescriptionEnums.h>
#include <KrautGenerator/Infrastructure/BoundingBox.h>
#include <KrautGenerator/Mesh/Mesh.h>

namespace Kraut
{
  struct KRAUT_DLL BranchMesh
  {
    // For things like the Kraut::BranchType and which material to use for this branch, reference the TreeStructure from which
    // the mesh was generated.
    // The index of this branch in the parent TreeMesh is the same index as in the TreeStructure, meaning the two arrays are always of the
    // same size, even if a branch mesh might be entirely empty.

    Kraut::BoundingBox m_BoundingBox[Kraut::BranchGeometryType::ENUM_COUNT];
    Kraut::Mesh m_Mesh[Kraut::BranchGeometryType::ENUM_COUNT];
  };

} // namespace Kraut
