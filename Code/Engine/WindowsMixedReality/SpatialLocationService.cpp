#include <PCH.h>
#include <WindowsMixedReality/SpatialLocationService.h>

#include <windows.perception.spatial.h>

#include <wrl/event.h>

ezWindowsSpatialLocationService::ezWindowsSpatialLocationService(const ComPtr<ABI::Windows::Perception::Spatial::ISpatialLocator>& pSpatialLocator)
  : m_pSpatialLocator(pSpatialLocator)
  , m_currentLocatability(ezSpatialLocatability::Unavailable)
{
  using OnLocatabilityChangedFunc = __FITypedEventHandler_2_Windows__CPerception__CSpatial__CSpatialLocator_IInspectable;
  if (FAILED(m_pSpatialLocator->add_LocatabilityChanged(Callback<OnLocatabilityChangedFunc>(this, &ezWindowsSpatialLocationService::OnLocatabilityChanged).Get(), &m_eventRegistrationLocatabilityChanged)))
  {
    ezLog::Error("Failed to subscribe for locatability changes on spatial locator.");
  }

  // Update internal state.
  OnLocatabilityChanged(m_pSpatialLocator.Get(), nullptr);
}

ezWindowsSpatialLocationService::~ezWindowsSpatialLocationService()
{
  if (m_pSpatialLocator)
    m_pSpatialLocator->remove_LocatabilityChanged(m_eventRegistrationLocatabilityChanged);
}

HRESULT ezWindowsSpatialLocationService::OnLocatabilityChanged(ABI::Windows::Perception::Spatial::ISpatialLocator* locator, IInspectable* args)
{
  ABI::Windows::Perception::Spatial::SpatialLocatability locatability;
  EZ_HRESULT_TO_FAILURE_LOG(locator->get_Locatability(&locatability));

  switch (locatability)
  {
  case ABI::Windows::Perception::Spatial::SpatialLocatability::SpatialLocatability_Unavailable:
    m_currentLocatability = ezSpatialLocatability::Unavailable;
    ezLog::SeriousWarning("Spatial locator unavailable, can't place holograms!");
    break;

  case ABI::Windows::Perception::Spatial::SpatialLocatability::SpatialLocatability_OrientationOnly:
    m_currentLocatability = ezSpatialLocatability::OrientationOnly;
    ezLog::Debug("Spatial locator is orientation only - the system is preparing to use positional tracking.");
    break;

  case ABI::Windows::Perception::Spatial::SpatialLocatability::SpatialLocatability_PositionalTrackingActivating:
    m_currentLocatability = ezSpatialLocatability::PositionalTrackingActivating;
    ezLog::Debug("Spatial locator is activating positional tracking.");
    break;

  case ABI::Windows::Perception::Spatial::SpatialLocatability::SpatialLocatability_PositionalTrackingActive:
    m_currentLocatability = ezSpatialLocatability::PositionalTrackingActive;
    ezLog::Debug("Spatial locator fully functional.");
    break;

  case ABI::Windows::Perception::Spatial::SpatialLocatability::SpatialLocatability_PositionalTrackingInhibited:
    m_currentLocatability = ezSpatialLocatability::PositionalTrackingInhibited;
    ezLog::Warning("Positional tracking is temporarily inhibited. User action may be required in order to restore positional tracking..");
    break;
  }

  return S_OK;
}

EZ_STATICLINK_FILE(WindowsMixedReality, WindowsMixedReality_HolographicLocationService);
