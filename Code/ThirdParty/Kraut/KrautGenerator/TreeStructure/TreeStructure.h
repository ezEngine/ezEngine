#pragma once

#include <KrautGenerator/Infrastructure/BoundingBox.h>
#include <KrautGenerator/TreeStructure/BranchStructure.h>

namespace Kraut
{
  using namespace AE_NS_FOUNDATION;

  struct SpawnNodeDesc;
  struct TreeStructureDesc;

  struct KRAUT_DLL TreeStructure
  {
    TreeStructure();
    ~TreeStructure();
    TreeStructure(const TreeStructure&) = delete;
    void operator=(const TreeStructure&) = delete;

    void Clear();

    Kraut::BoundingBox ComputeBoundingBox() const;

    aeUInt32 DuplicateBranch(aeUInt32 uiOriginalBranchID, float fRotation, aeInt32 iBranchBuddy);

    aeDeque<Kraut::BranchStructure> m_BranchStructures;
  };
} // namespace Kraut
