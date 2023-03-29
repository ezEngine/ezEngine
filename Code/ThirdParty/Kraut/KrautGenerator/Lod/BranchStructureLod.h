#pragma once

#include <KrautFoundation/Containers/Array.h>
#include <KrautGenerator/TreeStructure/BranchNode.h>

namespace Kraut
{
  using namespace AE_NS_FOUNDATION;

  struct KRAUT_DLL BranchStructureLod
  {
    aeArray<aeInt32> m_NodeIDs;
    aeArray<Kraut::BranchNode> m_TipNodes;
  };

} // namespace Kraut
