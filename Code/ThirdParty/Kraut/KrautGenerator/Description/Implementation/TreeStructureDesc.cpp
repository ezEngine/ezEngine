#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/TreeStructureDesc.h>

namespace Kraut
{
  TreeStructureDesc::TreeStructureDesc()
  {
    for (aeUInt32 i = 0; i < Kraut::BranchType::ENUM_COUNT; ++i)
    {
      m_BranchTypes[i].m_Type = (Kraut::BranchType::Enum)i;
    }
  }

  TreeStructureDesc::~TreeStructureDesc() = default;

  void TreeStructureDesc::Serialize(aeStreamOut& stream) const
  {
    const aeUInt8 uiVersion = 1;
    stream << uiVersion;

    const aeUInt8 uiNumBranchTypes = Kraut::BranchType::ENUM_COUNT;
    stream << uiNumBranchTypes;

    for (aeUInt32 i = 0; i < uiNumBranchTypes; ++i)
    {
      m_BranchTypes[i].Serialize(stream);
    }

    stream << m_uiRandomSeed;
    stream << m_bLeafCardMode;
    stream << m_bGrowProceduralTrunks;

    aeUInt32 uiNumInfluences = 0;
    stream << uiNumInfluences;

    // TODO: serialize influences
  }

  void TreeStructureDesc::Deserialize(aeStreamIn& stream)
  {
    aeUInt8 uiVersion = 0;
    stream >> uiVersion;

    AE_CHECK_DEV(uiVersion == 1, "Unknown version");

    aeUInt8 uiNumBranchTypes = 0;
    stream >> uiNumBranchTypes;

    AE_CHECK_DEV(uiNumBranchTypes == Kraut::BranchType::ENUM_COUNT, "Invalid number of branch types");

    for (aeUInt32 i = 0; i < uiNumBranchTypes; ++i)
    {
      m_BranchTypes[i].Deserialize(stream);
    }

    stream >> m_uiRandomSeed;
    stream >> m_bLeafCardMode;
    stream >> m_bGrowProceduralTrunks;

    aeUInt32 uiNumInfluences = 0;
    stream >> uiNumInfluences;

    AE_CHECK_DEV(uiNumInfluences == 0, "Invalid number of influences");
  }

  float TreeStructureDesc::GetBoundingBoxSizeIncrease() const
  {
    float result = 0.0f;

    for (aeUInt32 typeIdx = 0; typeIdx < Kraut::BranchType::ENUM_COUNT; ++typeIdx)
    {
      const Kraut::SpawnNodeDesc& snd = m_BranchTypes[typeIdx];

      if (!snd.m_bUsed)
        continue;

      // branch geometry is ignored here

      if (snd.m_bEnable[Kraut::BranchGeometryType::Frond])
      {
        // fronds are usually not oriented in a way that they take the maximum area (in the bounding box)
        // so scale them down by 0.5 to get a better approximation (otherwise the bbox gets too big)
        result = aeMath::Max(result, snd.m_fFrondHeight * 0.5f);
        result = aeMath::Max(result, snd.m_fFrondWidth * 0.5f);
      }

      if (snd.m_bEnable[Kraut::BranchGeometryType::Leaf])
      {
        result = aeMath::Max(result, snd.m_fLeafSize);
      }
    }

    return result;
  }


} // namespace Kraut
