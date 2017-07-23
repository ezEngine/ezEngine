#pragma once

#include <WindowsMixedReality/Basics.h>

/// \brief Represents a fixed location in space.
///
/// The OS may adjust the internal coordinate system as knowledge about the world becomes more precise.
/// Spatial anchors can be persisted and restored during the next application run.
class EZ_WINDOWSMIXEDREALITY_DLL ezWindowsSpatialAnchor
{
public:
  ezWindowsSpatialAnchor(const ComPtr<ABI::Windows::Perception::Spatial::ISpatialAnchor>& pSpatialAnchor);
  ~ezWindowsSpatialAnchor();

  /// \brief Returns the current coordinate system of the anchor.
  ezResult GetInternalCoordinateSystem(ComPtr<ABI::Windows::Perception::Spatial::ISpatialCoordinateSystem>& outCoordinateSystem) const;

  /// \brief Stores the current anchor state in persistent storage.
  ezResult PersistCurrentLocation(const char* szID);

  /// \brief Tries to load a previously persisted anchor location.
  /// Returns a valid spatial anchor on success, null on failure.
  static ezUniquePtr<ezWindowsSpatialAnchor> LoadPersistedLocation(const char* szID);

private:
  friend class ezWindowsSpatialLocationService;
  ComPtr<ABI::Windows::Perception::Spatial::ISpatialAnchor> m_pSpatialAnchor;
};

