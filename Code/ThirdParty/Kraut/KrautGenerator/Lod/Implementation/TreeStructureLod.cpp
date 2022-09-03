#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Lod/TreeStructureLod.h>

namespace Kraut
{
  aeUInt32 TreeStructureLod::GetNumBones() const
  {
    aeUInt32 result = 0;

    for (aeUInt32 b = 0; b < m_BranchLODs.size(); ++b)
    {
      result += m_BranchLODs[b].m_NodeIDs.size();
    }

    return result;
  }

} // namespace Kraut
