﻿#pragma once

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT

#  include <Foundation/Types/UniquePtr.h>
#  include <WindowsMixedReality/Basics.h>

enum class ezSpatialLocatability
{
  Unavailable,                  ///< Error, can't place holograms!
  PositionalTrackingActivating, ///< The system is preparing to use positional tracking.
  OrientationOnly,              ///< Positional tracking has not been activated.
  PositionalTrackingInhibited, ///< Positional tracking is temporarily inhibited. User action may be required in order to restore positional tracking.
  PositionalTrackingActive,    ///< Positional tracking is active. World-locked content can be rendered.
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

  /// Creates the simplest possible reference frame - stationary and at the current position and orientation of the headset.
  ezUniquePtr<ezWindowsSpatialReferenceFrame> CreateStationaryReferenceFrame_CurrentLocation();

  /// \brief Creates a stationary reference frame. Uses the existing \a origin as reference. \a offset is used as a position offset from \a origin.
  /// For the rotation, the current heading (rotation around the Y axis) is used as the basis on top of which the rotation offset is added. Thus, when
  /// the rotation offset is zero (identity), the reference frame simply uses the current headset heading.
  ezUniquePtr<ezWindowsSpatialReferenceFrame> CreateStationaryReferenceFrame_CurrentHeading(
    const ezWindowsSpatialReferenceFrame& origin, const ezTransform& offset);

  /// \brief Creates a stationary reference frame. Uses the existing \a origin as reference. \a offset adds a position and rotation offset in that
  /// space.
  ezUniquePtr<ezWindowsSpatialReferenceFrame> CreateStationaryReferenceFrame(const ezWindowsSpatialReferenceFrame& origin, const ezTransform& offset);

  /// \brief Creates a new anchor at the same position as the given anchor, but with an additional rotation applied.
  ezUniquePtr<ezWindowsSpatialReferenceFrame> CreateStationaryReferenceFrame_Rotated(
    const ezWindowsSpatialReferenceFrame& origin, ezAngle difference);

  /// \brief Creates a spatial anchor at the given offset relative to the given reference frame.
  ///
  /// If the reference frame is null, the default holographic space reference frame is used.
  /// Only position and rotation from offset are used, scale is ignored.
  ezUniquePtr<ezWindowsSpatialAnchor> CreateSpatialAnchor(const ezTransform& offset, const ezWindowsSpatialReferenceFrame* pReferenceFrame = nullptr);

  /// \brief Persists an existing spatial anchor under a given name. Anchor names need to be unique.
  ezResult SavePersistentAnchor(ezWindowsSpatialAnchor& anchor, const char* szID);

  /// \brief Tries to retrieve a spatial anchor by name. Returns null on failure.
  ezUniquePtr<ezWindowsSpatialAnchor> LoadPersistentAnchor(const char* szID);

  bool IsPersistantAnchorDataLoaded() const { return m_pStore != nullptr; }

private:
  void LoadSpatialAnchorMap();
  HRESULT OnLocatabilityChanged(ABI::Windows::Perception::Spatial::ISpatialLocator* locator, IInspectable* args);


  ComPtr<ABI::Windows::Perception::Spatial::ISpatialLocator> m_pSpatialLocator;
  ComPtr<ABI::Windows::Perception::Spatial::ISpatialAnchorStore> m_pStore;

  EventRegistrationToken m_eventRegistrationLocatabilityChanged;

  ezSpatialLocatability m_currentLocatability;
};

#endif
