#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/LodDesc.h>

namespace Kraut
{
  LodDesc::LodDesc()
  {
    for (aeUInt32 i = 0; i < Kraut::BranchGeometryType::ENUM_COUNT; ++i)
    {
      m_AllowTypes[(Kraut::BranchGeometryType::Enum)i] = 0xFFFFFFFF;
    }
  }

  void LodDesc::Serialize(aeStreamOut& s) const
  {
    aeUInt8 uiVersion = 7;

    s << uiVersion;

    s << m_fTipDetail;
    s << m_fCurvatureThreshold;
    s << m_fThicknessThreshold;
    s << m_fVertexRingDetail;

    // Version 3
    s << m_AllowTypes[Kraut::BranchGeometryType::Branch];

    // Version 4
    s << m_iMaxFrondDetail;
    s << m_iFrondDetailReduction;

    // Version 5
    s << m_AllowTypes[Kraut::BranchGeometryType::Frond];
    s << m_uiLodDistance;

    // Version 6
    s << m_AllowTypes[Kraut::BranchGeometryType::Leaf];

    // Version 7
    {
      aeInt8 uiLodMode = m_Mode;
      s << uiLodMode;
    }
  }

  void LodDesc::Deserialize(aeStreamIn& s)
  {
    aeUInt8 uiVersion = 1;

    s >> uiVersion;

    s >> m_fTipDetail;
    s >> m_fCurvatureThreshold;
    s >> m_fThicknessThreshold;

    if (uiVersion >= 2)
      s >> m_fVertexRingDetail;

    if (uiVersion >= 3)
    {
      s >> m_AllowTypes[Kraut::BranchGeometryType::Branch];
    }

    if (uiVersion >= 4)
    {
      s >> m_iMaxFrondDetail;
      s >> m_iFrondDetailReduction;
    }

    if (uiVersion >= 5)
    {
      s >> m_AllowTypes[Kraut::BranchGeometryType::Frond];
      s >> m_uiLodDistance;
    }

    if (uiVersion >= 6)
    {
      s >> m_AllowTypes[Kraut::BranchGeometryType::Leaf];
    }

    if (uiVersion >= 7)
    {
      aeInt8 uiLodMode;
      s >> uiLodMode;
      m_Mode = (Kraut::LodMode::Enum)uiLodMode;
    }
  }
} // namespace Kraut
