#pragma once

#include <KrautFoundation/Streams/Streams.h>
#include <KrautGenerator/Description/DescriptionEnums.h>

namespace Kraut
{
  using namespace AE_NS_FOUNDATION;

  struct KRAUT_DLL LodDesc
  {
    LodDesc();

    void Deserialize(aeStreamIn& s);
    void Serialize(aeStreamOut& s) const;

    Kraut::LodMode::Enum m_Mode = LodMode::Full;                   //!< How to represent the tree in this lod.
    float m_fTipDetail = 0.04f;                                    //!< in 'meters per ring', e.g. 0.04 for one ring every 4 centimeters
    float m_fCurvatureThreshold = 5.0f;                            //!< in degree deviation, 0 -> full detail, 5 -> merge branches with less than 5 degree difference
    float m_fThicknessThreshold = 0.2f;                            //!< in percent deviation, between 0.0 and 1.0
    float m_fVertexRingDetail = 0.2f;                              //!< in 'distance between vertices on the vertex ring'
    aeUInt32 m_AllowTypes[Kraut::BranchGeometryType::ENUM_COUNT]; //!< Bitfield that says which branch types are allowed in this LOD

    aeInt8 m_iMaxFrondDetail = 32;      //!< The value to clamp the "frond detail" value to
    aeInt8 m_iFrondDetailReduction = 0; //!<By how much to reduce the frond detail (before clamping)

    aeUInt32 m_uiLodDistance = 0; //!< At which distance (in meters) this LOD should be used

    Kraut::BranchSpikeTipMode::Enum m_BranchSpikeTipMode = BranchSpikeTipMode::FullDetail;
  };
} // namespace Kraut
