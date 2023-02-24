#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>
#include <Shaders/Common/Platforms.h>

// Resolutions of the lookup tables. Shared between C++ (texture creation, dispatch sizes) and the shaders.
#define ATMOSPHERE_TRANSMITTANCE_LUT_WIDTH 256
#define ATMOSPHERE_TRANSMITTANCE_LUT_HEIGHT 64
#define ATMOSPHERE_MULTISCATTERING_LUT_SIZE 32
#define ATMOSPHERE_SKYVIEW_LUT_WIDTH 192
#define ATMOSPHERE_SKYVIEW_LUT_HEIGHT 108
#define ATMOSPHERE_SKYVIEW_LUT_DEPTH 16 ///< Number of camera height slices.

#define ATMOSPHERE_LUT_THREADS 8

/// Parameters of the atmosphere, used by the LUT compute shaders and the sky shader.
///
/// All distances are in kilometers and all coefficients are per kilometer, except where noted.
/// The planet is centered at the origin, +Z is up.
CONSTANT_BUFFER2(ezAtmosphereConstants, 3, BG_DRAW_CALL)
{
  FLOAT3(RayleighScattering);
  FLOAT1(PlanetRadius);

  FLOAT3(OzoneAbsorption);
  FLOAT1(AtmosphereRadius);

  FLOAT3(SunDirection); ///< Normalized direction towards the sun, in world space.
  FLOAT1(MieScattering);

  FLOAT1(RayleighScaleHeight);
  FLOAT1(MieScaleHeight);
  FLOAT1(MieAnisotropy);
  FLOAT1(MaxSkyViewHeight); ///< Camera height (above ground) covered by the top slice of the sky-view LUT.

  FLOAT1(SunIntensity);
  FLOAT1(SunDiscCosAngle);
  FLOAT1(SunDiscIntensity);
  FLOAT1(GroundHeight); ///< In meters, world-space Z of the planet surface.

  FLOAT3(GroundAlbedo);    ///< Linear diffuse reflectance of the planet surface.
  FLOAT1(VirtualDistance); ///< In meters, used for the fog.

  UINT1(UseFog);
};
