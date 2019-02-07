#includde <WindowsMixedRealityPCH.h>
#include <WindowsMixedReality/SpatialReferenceFrame.h>

#include <windows.perception.spatial.h>

ezWindowsSpatialReferenceFrame::ezWindowsSpatialReferenceFrame(const ComPtr<ABI::Windows::Perception::Spatial::ISpatialStationaryFrameOfReference>& pReferenceFrame)
  : m_pReferenceFrame(pReferenceFrame)
{
}

ezWindowsSpatialReferenceFrame::~ezWindowsSpatialReferenceFrame()
{
}

void ezWindowsSpatialReferenceFrame::GetInternalCoordinateSystem(ComPtr<ABI::Windows::Perception::Spatial::ISpatialCoordinateSystem>& outCoordinateSystem) const
{
  EZ_HRESULT_TO_ASSERT(m_pReferenceFrame->get_CoordinateSystem(outCoordinateSystem.GetAddressOf()));
}

