﻿#pragma once

namespace ABI
{
  namespace Windows
  {
    namespace Foundation
    {
      struct IDeferral;
    }

    namespace Graphics
    {
      namespace Holographic
      {
        struct IHolographicSpaceStatics;
        struct IHolographicSpace;

        struct IHolographicSpaceCameraAddedEventArgs;
        struct IHolographicSpaceCameraRemovedEventArgs;

        struct IHolographicCamera;
        struct IHolographicFrame;
      } // namespace Holographic
    }   // namespace Graphics

    namespace Perception
    {
      struct IPerceptionTimestamp;

      namespace Spatial
      {
        struct ISpatialStationaryFrameOfReference;
        struct ISpatialCoordinateSystem;
        struct ISpatialLocator;
        struct ISpatialAnchor;
        struct ISpatialAnchorStore;
      } // namespace Spatial
    }   // namespace Perception
  }     // namespace Windows
} // namespace ABI

using namespace ABI::Windows::Foundation;

class ezWindowsSpatialReferenceFrame;
class ezWindowsSpatialAnchor;
class ezWindowsSpatialLocationService;
class ezWindowsMixedRealityCamera;
struct IDXGIAdapter3;