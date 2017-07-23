#pragma once

#include <WindowsMixedReality/Basics.h>

/// \brief A frame of reference on windows holographic.
///
/// \see ezWindowsSpatialLocationService
class EZ_WINDOWSMIXEDREALITY_DLL ezWindowsSpatialReferenceFrame
{
public:
  ezWindowsSpatialReferenceFrame(const ComPtr<ABI::Windows::Perception::Spatial::ISpatialStationaryFrameOfReference>& pReferenceFrame);
  ~ezWindowsSpatialReferenceFrame();

  void GetInternalCoordinateSystem(ComPtr<ABI::Windows::Perception::Spatial::ISpatialCoordinateSystem>& outCoordinateSystem) const;

private:

  ComPtr<ABI::Windows::Perception::Spatial::ISpatialStationaryFrameOfReference> m_pReferenceFrame;
};


