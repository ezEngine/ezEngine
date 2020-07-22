#pragma once

#include <GameEngine/GameEngineDLL.h>

typedef ezGenericId<32, 16> ezXRSpatialAnchorID;

/// \brief XR spatial anchors interface.
///
/// Aquire interface via ezSingletonRegistry::GetSingletonInstance<ezXRSpatialAnchorsInterface>().
class ezXRSpatialAnchorsInterface
{
public:
  /// \brief Creates a spatial anchor at the given world space position.
  /// Returns an invalid handle if anchors can't be created right now. Retry next frame.
  virtual ezXRSpatialAnchorID CreateAnchor(const ezTransform& globalTransform) = 0;

  /// \brief Destroys a previously created anchor.
  virtual ezResult DestroyAnchor(ezXRSpatialAnchorID id) = 0;

  /// \brief Tries to resolve the anchor position. Can fail of the anchor is invalid or tracking is
  /// currently lost.
  virtual ezResult TryGetAnchorTransform(ezXRSpatialAnchorID id, ezTransform& out_globalTransform) = 0;
};
