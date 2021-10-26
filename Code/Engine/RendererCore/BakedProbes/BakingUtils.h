#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <RendererCore/Declarations.h>

using ezCompressedSkyVisibility = ezUInt32;

namespace ezBakingUtils
{
  EZ_RENDERERCORE_DLL ezVec3 FibonacciSphere(ezUInt32 sampleIndex, ezUInt32 numSamples);

  EZ_RENDERERCORE_DLL ezCompressedSkyVisibility CompressSkyVisibility(const ezAmbientCube<float>& skyVisibility);
  EZ_RENDERERCORE_DLL void DecompressSkyVisibility(ezCompressedSkyVisibility compressedSkyVisibility, ezAmbientCube<float>& out_SkyVisibility);
} // namespace ezBakingUtils
