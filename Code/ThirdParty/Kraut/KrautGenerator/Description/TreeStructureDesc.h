#pragma once

#include <KrautGenerator/Description/Influence.h>
#include <KrautGenerator/Description/SpawnNodeDesc.h>
#include <KrautFoundation/Containers/Deque.h>

namespace Kraut
{
  struct KRAUT_DLL TreeStructureDesc
  {
    TreeStructureDesc();
    ~TreeStructureDesc();
    TreeStructureDesc(TreeStructureDesc&&) = delete;
    void operator=(TreeStructureDesc&&) = delete;

    void Serialize(aeStreamOut& stream) const;
    void Deserialize(aeStreamIn& stream);

    float GetBoundingBoxSizeIncrease() const;

    aeUInt32 m_uiRandomSeed = 0;
    Kraut::SpawnNodeDesc m_BranchTypes[Kraut::BranchType::ENUM_COUNT];
    bool m_bLeafCardMode = false;
    bool m_bGrowProceduralTrunks = true;

    // deque, instead of array, because of the unique_ptr
    aeDeque<std::unique_ptr<Kraut::Influence>> m_Influences;
  };

} // namespace Kraut
