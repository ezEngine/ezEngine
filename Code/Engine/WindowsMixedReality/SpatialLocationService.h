#pragma once

#include <WindowsMixedReality/Basics.h>

namespace ABI
{
  namespace Windows
  {
    namespace Perception
    {
      namespace Spatial
      {
        struct ISpatialLocator;
        struct ISpatialStationaryFrameOfReference;
      }
    }
  }
}

enum class ezSpatialLocatability
{
  Unavailable,                    ///< Error, can't place holograms!
  PositionalTrackingActivating,   ///< The system is preparing to use positional tracking.
  OrientationOnly,                ///< Positional tracking has not been activated.
  PositionalTrackingInhibited,    ///< Positional tracking is temporarily inhibited. User action may be required in order to restore positional tracking.
  PositionalTrackingActive,       ///< Positional tracking is active. World-locked content can be rendered.
};

/// \brief Provides locations for holographic environments.
///
/// See https://docs.microsoft.com/en-us/uwp/api/windows.perception.spatial.spatiallocator
class EZ_WINDOWSMIXEDREALITY_DLL ezWindowsSpatialLocationService
{
public:
  ezWindowsSpatialLocationService(const ComPtr<ABI::Windows::Perception::Spatial::ISpatialLocator>& pDefaultSpatialLocator);
  ~ezWindowsSpatialLocationService();

  // Notes on spatial tracking APIs from Microsoft's holographic app sample:
  // * Stationary reference frames are designed to provide a best-fit position relative to the
  //   overall space. Individual positions within that reference frame are allowed to drift slightly
  //   as the device learns more about the environment.
  // * When precise placement of individual holograms is required, a SpatialAnchor should be used to
  //   anchor the individual hologram to a position in the real world - for example, a point the user
  //   indicates to be of special interest. Anchor positions do not drift, but can be corrected; the
  //   anchor will use the corrected position starting in the next frame after the correction has
  //   occurred.

  /// Retrieves the current status of the location system.
  ezSpatialLocatability GetCurrentLocatability() { return m_currentLocatability; }

private:

  HRESULT OnLocatabilityChanged(ABI::Windows::Perception::Spatial::ISpatialLocator* locator, IInspectable* args);


  ComPtr<ABI::Windows::Perception::Spatial::ISpatialLocator> m_pSpatialLocator;
  EventRegistrationToken m_eventRegistrationLocatabilityChanged;

  ezSpatialLocatability m_currentLocatability;
};


/// \brief A frame of reference on windows holographic.
///
/// \see ezWindowsSpatialLocationService
class EZ_WINDOWSMIXEDREALITY_DLL ezWindowsHolographicReferenceFrame
{
public:
  ezWindowsHolographicReferenceFrame(const ComPtr<ABI::Windows::Perception::Spatial::ISpatialStationaryFrameOfReference>& pReferenceFrame);

  // todo

private:

  ComPtr<ABI::Windows::Perception::Spatial::ISpatialStationaryFrameOfReference> m_pReferenceFrame;
};


