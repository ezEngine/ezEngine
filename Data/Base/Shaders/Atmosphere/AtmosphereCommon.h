#pragma once

// Atmospheric scattering following "A Scalable and Production Ready Sky and Atmosphere Rendering Technique"
// (Sebastien Hillaire, EGSR 2020) with the transmittance parameterization from Bruneton's "Precomputed Atmospheric Scattering".
//
// The scattering model (Rayleigh, Mie and an ozone layer that peaks at 30 km) and the default coefficients are taken from
// "Atmospheric scattering" by Dimas Leenman (MIT License, Copyright (c) 2019 Dimas Leenman).
//
// Three lookup tables are used:
//  - Transmittance:    transmittance from a point to the top of the atmosphere, by height and zenith angle.
//  - Multi-scattering: isotropic contribution of all higher scattering orders, by height and sun zenith angle.
//  - Sky-view:         in-scattered luminance for every view direction, relative to the sun, for a set of camera heights.
//
// All distances are in kilometers, the planet is centered at the origin and +Z is up.

#include <Shaders/Atmosphere/AtmosphereConstants.h>
#include <Shaders/Common/Common.h>

#define ATMOSPHERE_OZONE_HEIGHT 30.0 // height in km at which the ozone absorption is at its maximum
#define ATMOSPHERE_OZONE_FALLOFF 4.0 // how quickly the ozone absorption falls off around that height, in km
#define ATMOSPHERE_MIN_HEIGHT 0.001  // keeps sample points slightly above the ground sphere to avoid degenerate horizon math

// Distances along a ray (normalized dir) at which it enters (x) and exits (y) a sphere centered at the origin.
// There is no intersection if x > y.
float2 AtmosphereRaySphereIntersect(float3 start, float3 dir, float radius)
{
  float b = dot(dir, start);
  float c = dot(start, start) - radius * radius;
  float d = b * b - c;

  if (d < 0.0)
    return float2(1e9, -1e9);

  d = sqrt(d);
  return float2(-b - d, -b + d);
}

// Distance to the ground along the ray, or -1 if it doesn't hit the ground in front of the start point.
float AtmosphereGroundDistance(float3 start, float3 dir)
{
  float2 t = AtmosphereRaySphereIntersect(start, dir, PlanetRadius);
  return (t.x <= t.y && t.x > 0.0) ? t.x : -1.0;
}

// Distance to the top of the atmosphere along the ray, assuming the start point is inside the atmosphere.
float AtmosphereTopDistance(float3 start, float3 dir)
{
  return max(AtmosphereRaySphereIntersect(start, dir, AtmosphereRadius).y, 0.0);
}

struct AtmosphereMedium
{
  float3 scatteringRayleigh;
  float3 scatteringMie;
  float3 scattering;
  float3 extinction;
};

AtmosphereMedium AtmosphereSampleMedium(float height)
{
  float densityRayleigh = exp(-height / RayleighScaleHeight);
  float densityMie = exp(-height / MieScaleHeight);

  // the ozone layer scales with the rayleigh density but peaks at a specific height
  float denom = (ATMOSPHERE_OZONE_HEIGHT - height) / ATMOSPHERE_OZONE_FALLOFF;
  float densityOzone = densityRayleigh / (denom * denom + 1.0);

  AtmosphereMedium m;
  m.scatteringRayleigh = RayleighScattering * densityRayleigh;
  m.scatteringMie = MieScattering.xxx * densityMie;
  m.scattering = m.scatteringRayleigh + m.scatteringMie;
  m.extinction = max(m.scattering + OzoneAbsorption * densityOzone, 1e-6); // Mie scattering is treated as non-absorbing
  return m;
}

float AtmosphereRayleighPhase(float mu)
{
  return 3.0 / (16.0 * PI) * (1.0 + mu * mu);
}

// Cornette-Shanks phase function
float AtmosphereMiePhase(float mu)
{
  float g = MieAnisotropy;
  float gg = g * g;
  return 3.0 / (8.0 * PI) * ((1.0 - gg) * (1.0 + mu * mu)) / ((2.0 + gg) * pow(max(1.0 + gg - 2.0 * g * mu, 1e-4), 1.5));
}

// Maps [0, 1] to texel centers, so that 0 and 1 land exactly on the first and last texel.
float AtmosphereUnitToSubUv(float u, float resolution)
{
  return (u + 0.5 / resolution) * (resolution / (resolution + 1.0));
}

float AtmosphereSubUvToUnit(float u, float resolution)
{
  return (u - 0.5 / resolution) * (resolution / (resolution - 1.0));
}

//////////////////////////////////////////////////////////////////////////
// Transmittance LUT

float2 AtmosphereTransmittanceRMuToUv(float r, float mu)
{
  float H = sqrt(AtmosphereRadius * AtmosphereRadius - PlanetRadius * PlanetRadius);
  float rho = sqrt(max(r * r - PlanetRadius * PlanetRadius, 0.0));

  float discriminant = r * r * (mu * mu - 1.0) + AtmosphereRadius * AtmosphereRadius;
  float d = max(0.0, -r * mu + sqrt(max(discriminant, 0.0)));

  float dMin = AtmosphereRadius - r;
  float dMax = rho + H;

  float2 uv = float2((d - dMin) / max(dMax - dMin, 1e-6), rho / H);
  return float2(AtmosphereUnitToSubUv(uv.x, ATMOSPHERE_TRANSMITTANCE_LUT_WIDTH), AtmosphereUnitToSubUv(uv.y, ATMOSPHERE_TRANSMITTANCE_LUT_HEIGHT));
}

void AtmosphereTransmittanceUvToRMu(float2 uv, out float r, out float mu)
{
  float xMu = AtmosphereSubUvToUnit(uv.x, ATMOSPHERE_TRANSMITTANCE_LUT_WIDTH);
  float xR = AtmosphereSubUvToUnit(uv.y, ATMOSPHERE_TRANSMITTANCE_LUT_HEIGHT);

  float H = sqrt(AtmosphereRadius * AtmosphereRadius - PlanetRadius * PlanetRadius);
  float rho = H * xR;
  r = sqrt(rho * rho + PlanetRadius * PlanetRadius);

  float dMin = AtmosphereRadius - r;
  float dMax = rho + H;
  float d = dMin + xMu * (dMax - dMin);

  mu = d == 0.0 ? 1.0 : (H * H - rho * rho - d * d) / (2.0 * r * d);
  mu = clamp(mu, -1.0, 1.0);
}

float3 AtmosphereSampleTransmittance(Texture2D lut, float r, float mu)
{
  return lut.SampleLevel(LinearClampSampler, AtmosphereTransmittanceRMuToUv(r, mu), 0.0).rgb;
}

// Transmittance from the given point towards the sun, zero if the planet is in the way.
float3 AtmosphereSunTransmittance(Texture2D lut, float3 pos, float3 sunDir)
{
  float r = length(pos);
  float3 up = pos / r;

  if (AtmosphereGroundDistance(pos, sunDir) >= 0.0)
    return 0.0;

  return AtmosphereSampleTransmittance(lut, r, dot(up, sunDir));
}

// Sun light reflected by the ground towards the viewer, for a ray that hits the ground at groundPos.
// The surface is treated as diffuse with GroundAlbedo, lit by the attenuated sun only.
float3 AtmosphereGroundLuminance(Texture2D lut, float3 groundPos, float3 sunDir)
{
  float3 up = normalize(groundPos);
  float sunZenithCos = dot(up, sunDir);

  if (sunZenithCos <= 0.0)
    return 0.0;

  return AtmosphereSampleTransmittance(lut, PlanetRadius, sunZenithCos) * sunZenithCos * GroundAlbedo / PI;
}

//////////////////////////////////////////////////////////////////////////
// Multi-scattering LUT

float3 AtmosphereSampleMultiScattering(Texture2D lut, float r, float sunZenithCos)
{
  float2 uv;
  uv.x = AtmosphereUnitToSubUv(saturate(sunZenithCos * 0.5 + 0.5), ATMOSPHERE_MULTISCATTERING_LUT_SIZE);
  uv.y = AtmosphereUnitToSubUv(saturate((r - PlanetRadius) / (AtmosphereRadius - PlanetRadius)), ATMOSPHERE_MULTISCATTERING_LUT_SIZE);
  return lut.SampleLevel(LinearClampSampler, uv, 0.0).rgb;
}

//////////////////////////////////////////////////////////////////////////
// Sky-view LUT
//
// x: azimuth relative to the sun, using the symmetry around the sun's vertical plane (0 = towards the sun, 1 = away from it).
// y: view zenith angle, non-linear so that more texels end up close to the horizon, which is always at 0.5.
// z: camera height above the ground, with a cubic distribution so that more slices end up close to the ground.

float AtmosphereSkyViewSliceToHeight(float w)
{
  return max(w * w * w * MaxSkyViewHeight, ATMOSPHERE_MIN_HEIGHT);
}

float AtmosphereSkyViewHeightToW(float height)
{
  return pow(saturate(height / MaxSkyViewHeight), 1.0 / 3.0);
}

void AtmosphereSkyViewUvToDirection(float2 uv, float r, out float viewZenithCos, out float lightViewCos)
{
  uv.x = AtmosphereSubUvToUnit(uv.x, ATMOSPHERE_SKYVIEW_LUT_WIDTH);
  uv.y = AtmosphereSubUvToUnit(uv.y, ATMOSPHERE_SKYVIEW_LUT_HEIGHT);

  float horizonDist = sqrt(max(r * r - PlanetRadius * PlanetRadius, 0.0));
  float beta = acos(clamp(horizonDist / r, -1.0, 1.0));
  float zenithHorizonAngle = PI - beta;

  if (uv.y < 0.5)
  {
    float coord = 1.0 - 2.0 * uv.y;
    coord = 1.0 - coord * coord;
    viewZenithCos = cos(zenithHorizonAngle * coord);
  }
  else
  {
    float coord = uv.y * 2.0 - 1.0;
    coord *= coord;
    viewZenithCos = cos(zenithHorizonAngle + beta * coord);
  }

  float coord = uv.x * uv.x;
  lightViewCos = -(coord * 2.0 - 1.0);
}

float2 AtmosphereDirectionToSkyViewUv(bool bIntersectsGround, float viewZenithCos, float lightViewCos, float r)
{
  float horizonDist = sqrt(max(r * r - PlanetRadius * PlanetRadius, 0.0));
  float beta = acos(clamp(horizonDist / r, -1.0, 1.0));
  float zenithHorizonAngle = PI - beta;
  float viewZenithAngle = acos(clamp(viewZenithCos, -1.0, 1.0));

  float2 uv;

  if (!bIntersectsGround)
  {
    float coord = saturate(viewZenithAngle / zenithHorizonAngle);
    coord = 1.0 - sqrt(1.0 - coord);
    uv.y = coord * 0.5;
  }
  else
  {
    float coord = saturate((viewZenithAngle - zenithHorizonAngle) / max(beta, 1e-6));
    uv.y = sqrt(coord) * 0.5 + 0.5;
  }

  uv.x = sqrt(saturate(-lightViewCos * 0.5 + 0.5));

  return float2(AtmosphereUnitToSubUv(uv.x, ATMOSPHERE_SKYVIEW_LUT_WIDTH), AtmosphereUnitToSubUv(uv.y, ATMOSPHERE_SKYVIEW_LUT_HEIGHT));
}
