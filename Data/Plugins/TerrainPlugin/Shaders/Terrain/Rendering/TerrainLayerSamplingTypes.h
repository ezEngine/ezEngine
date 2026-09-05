#pragma once

/// Triplanar UV types shared by the terrain and voxel material shaders.
///
/// Separate from TerrainLayerSampling.h because the pixel shaders name TriplanarUVs in their
/// CUSTOM_GLOBALS macro, which MaterialPixelShader.h expands into PS_GLOBALS: the type must be
/// declared before that header, while the sampling functions depend on it and must come after.

/// Precomputed triplanar UVs and their screen-space derivatives.
///
/// The UVs depend only on world position and scale, never on the material index, so all layers of
/// a blend share one of these. Sampling with the explicit gradients is what makes it legal to skip
/// a layer's fetches inside a branch: a plain Sample derives its mip level from neighbouring lanes
/// in the 2x2 quad, which is undefined when those lanes took the other side of the branch.
struct TriplanarUVs
{
  float2 UvX;
  float2 UvY;
  float2 UvZ;
  float2 DdxX, DdyX;
  float2 DdxY, DdyY;
  float2 DdxZ, DdyZ;
  float3 SignN; ///< sign(worldNormal), needed to reorient normal maps.
};

/// Builds the triplanar UV set for a pixel. Must be called from uniform control flow - i.e. before
/// any branch that skips a layer - because the ddx/ddy here are the gradients every later
/// SampleGrad relies on.
TriplanarUVs ComputeTriplanarUVs(float3 worldPos, float3 worldNormal, float scale)
{
  TriplanarUVs r;
  r.SignN = sign(worldNormal);
  const float3 ns = r.SignN * scale;

  r.UvX = worldPos.yz * float2(-ns.x, -scale);
  r.UvY = worldPos.xz * float2(ns.y, -scale);
  r.UvZ = worldPos.xy * float2(ns.z, scale);

  r.DdxX = ddx(r.UvX);
  r.DdyX = ddy(r.UvX);
  r.DdxY = ddx(r.UvY);
  r.DdyY = ddy(r.UvY);
  r.DdxZ = ddx(r.UvZ);
  r.DdyZ = ddy(r.UvZ);
  return r;
}
