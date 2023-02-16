#pragma once

#include <KrautFoundation/Containers/Array.h>
#include <KrautGenerator/Lod/BranchStructureLod.h>

namespace Kraut
{
  using namespace AE_NS_FOUNDATION;

  struct KRAUT_DLL TreeStructureLod
  {
    TreeStructureLod() = default;
    TreeStructureLod(const TreeStructureLod&) = delete;
    void operator=(const TreeStructureLod&) = delete;

    aeUInt32 GetNumBones() const;

    aeArray<Kraut::BranchStructureLod> m_BranchLODs;
  };

} // namespace Kraut
