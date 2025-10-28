#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <RendererCore/Declarations.h>

/// Compressed representation of ambient cube sky visibility data.
///
/// Packs 6 directional visibility values into a 32-bit integer using variable bit depths
/// (5 bits for the 4 horizontal directions, 6 bits for the 2 vertical directions).
using ezCompressedSkyVisibility = ezUInt32;

namespace ezBakingUtils
{
  /// Generates a point on a unit sphere using Fibonacci spiral sampling.
  ///
  /// This produces evenly distributed points on a sphere, useful for hemisphere sampling
  /// during baking. The distribution minimizes clustering and gaps.
  EZ_RENDERERCORE_DLL ezVec3 FibonacciSphere(ezUInt32 uiSampleIndex, ezUInt32 uiNumSamples);

  /// Compresses ambient cube sky visibility into a 32-bit value.
  ///
  /// Uses variable bit depths per direction (5 or 6 bits) to pack the data.
  /// Values are clamped to [0,1] before compression.
  EZ_RENDERERCORE_DLL ezCompressedSkyVisibility CompressSkyVisibility(const ezAmbientCube<float>& skyVisibility);

  /// Decompresses sky visibility from a 32-bit value back to ambient cube format.
  EZ_RENDERERCORE_DLL void DecompressSkyVisibility(ezCompressedSkyVisibility compressedSkyVisibility, ezAmbientCube<float>& out_skyVisibility);
} // namespace ezBakingUtils
