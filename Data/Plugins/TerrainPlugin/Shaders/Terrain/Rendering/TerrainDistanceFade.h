#pragma once

/// Distance-driven quality reduction shared by the terrain and voxel material shaders.
///
/// Driven by distance to the camera rather than by the geometric LOD level on purpose: LOD is
/// chosen from geometric error, while the right moment to drop a texture layer or damp a highlight
/// depends on how many texels land in a pixel. Tying them together would make one system's tuning
/// leak into the other's.

/// Fade band for a layer, as distances from the camera. Full strength below 'fadeStart', gone
/// above 'fadeEnd'. An empty or inverted band never fades.
float TerrainLayerDistanceFade(float distance, float fadeStart, float fadeEnd)
{
  if (fadeEnd <= fadeStart)
    return 1.0;

  return 1.0 - smoothstep(fadeStart, fadeEnd, distance);
}

/// Raises roughness with distance, so that distant specular highlights dim instead of flickering
/// as the camera moves.
///
/// 'amount' is how far roughness is pushed toward 1 at and beyond 'fadeEnd'; 0 disables the effect.
float TerrainDistanceRoughness(float roughness, float distance, float fadeStart, float fadeEnd, float amount)
{
  if (amount <= 0.0 || fadeEnd <= fadeStart)
    return roughness;

  const float t = smoothstep(fadeStart, fadeEnd, distance);
  return saturate(roughness + t * amount * (1.0 - roughness));
}

/// Converts sub-pixel normal variance into additional roughness (a Toksvig-style estimate).
///
/// 'blendedNormalLength' is the length of the summed, not-yet-normalized normal. Averaging normals
/// that disagree shortens the sum, so a short vector means the surface varies faster than a pixel
/// can resolve. Unlike TerrainDistanceRoughness this helps at any distance, wherever the terrain is
/// higher frequency than a pixel. A length of 1 means the normals agreed and roughness is unchanged.
float TerrainNormalVarianceRoughness(float roughness, float blendedNormalLength, float strength)
{
  if (strength <= 0.0)
    return roughness;

  const float len = saturate(blendedNormalLength);
  if (len >= 0.999)
    return roughness;

  return saturate(roughness + (1.0 - len) * strength * (1.0 - roughness));
}
