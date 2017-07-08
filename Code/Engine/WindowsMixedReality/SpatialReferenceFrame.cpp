#include <PCH.h>
#include <WindowsMixedReality/SpatialReferenceFrame.h>

#include <windows.perception.spatial.h>

ezWindowsSpatialReferenceFrame::ezWindowsSpatialReferenceFrame(const ComPtr<ABI::Windows::Perception::Spatial::ISpatialStationaryFrameOfReference>& pReferenceFrame)
  : m_pReferenceFrame(pReferenceFrame)
{
}

ezWindowsSpatialReferenceFrame::~ezWindowsSpatialReferenceFrame()
{
}

ezResult ezWindowsSpatialReferenceFrame::GetInternalCoordinateSystem(ComPtr<ABI::Windows::Perception::Spatial::ISpatialCoordinateSystem>& outCoordinateSystem) const
{
  EZ_HRESULT_TO_FAILURE_LOG(m_pReferenceFrame->get_CoordinateSystem(outCoordinateSystem.GetAddressOf()));
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(WindowsMixedReality, WindowsMixedReality_SpatialReferenceFrame);
