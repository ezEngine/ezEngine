#pragma once

/// Shared pixel-shader resources and triplanar sampling helpers used by all terrain and voxel material shaders.
/// Include this after MaterialPixelShader.h so that DecodeNormalTexture is available.

/// Triplanar blend weights matching the formula used by the engine's SampleTexture3Way helper.
float3 TriplanarWeights(float3 worldNormal)
{
  float3 w = abs(worldNormal);
  w = max((w - 0.2) * 7.0, 0.0);
  return w / (w.x + w.y + w.z);
}

/// Samples one layer of a 2D array texture using triplanar projection.
/// UV sign flips follow SampleTexture3Way convention so textures are consistent on both sides of each axis.
/// matIndex: x = top/Z-facing texture array layer, y = side/XY-facing texture array layer.
float4 TriplanarSampleColorArray(Texture2DArray tex, SamplerState samp, float3 worldPos, float3 worldNormal, float3 weights, float scale, uint2 matIndex)
{
  float2 layer = (float2)matIndex;
  float3 ns = sign(worldNormal) * scale;

  float4 cX = tex.Sample(samp, float3(worldPos.yz * float2(-ns.x, -scale), layer.y));
  float4 cY = tex.Sample(samp, float3(worldPos.xz * float2(ns.y, -scale), layer.y));
  float4 cZ = tex.Sample(samp, float3(worldPos.xy * float2(ns.z, scale), layer.x));

  return cX * weights.x + cY * weights.y + cZ * weights.z;
}

/// Samples one layer of a 2D array normal map using triplanar projection and returns a world-space normal.
/// UV sign flips match TriplanarSampleColorArray; tangent frames are derived from those same UVs so the
/// normals are consistent with the color textures on both sides of each axis.
/// matIndex: x = top/Z-facing texture array layer, y = side/XY-facing texture array layer.
float3 TriplanarSampleNormalArray(Texture2DArray tex, SamplerState samp, float3 worldPos, float3 worldNormal, float3 weights, float scale, uint2 matIndex)
{
  float2 layer = (float2)matIndex;
  float3 sn = sign(worldNormal);
  float3 ns = sn * scale;

  float3 nX = DecodeNormalTexture(tex.Sample(samp, float3(worldPos.yz * float2(-ns.x, -scale), layer.y)));
  float3 nY = DecodeNormalTexture(tex.Sample(samp, float3(worldPos.xz * float2(ns.y, -scale), layer.y)));
  float3 nZ = DecodeNormalTexture(tex.Sample(samp, float3(worldPos.xy * float2(ns.z, scale), layer.x)));

  // Reorient each tangent-space normal into world space using the tangent frame implied by the UV mapping above.
  // Tangent frames (T, B, N) are derived as cross(dPos/dU, dPos/dV):
  //   X proj: T=(0,-sn.x,0), B=(0,0,-1), N=(sn.x,0,0)  ->  (nX.z*sn.x, -nX.x*sn.x, -nX.y)
  //   Y proj: T=(sn.y, 0,0), B=(0,0,-1), N=(0,sn.y,0)  ->  (nY.x*sn.y,  nY.z*sn.y,  -nY.y)
  //   Z proj: T=(sn.z, 0,0), B=(0, 1,0), N=(0,0,sn.z)  ->  (nZ.x*sn.z,  nZ.y,        nZ.z*sn.z)
  float3 wsX = float3(nX.z * sn.x, -nX.x * sn.x, -nX.y);
  float3 wsY = float3(nY.x * sn.y, nY.z * sn.y, -nY.y);
  float3 wsZ = float3(nZ.x * sn.z, nZ.y, nZ.z * sn.z);

  return normalize(wsX * weights.x + wsY * weights.y + wsZ * weights.z);
}

/// Maximum number of layers TerrainHeightBlendWeights can process.
#define TERRAIN_MAX_BLEND_LAYERS 5

/// Redistributes linear splat weights using per-layer height (displacement) values, so that the
/// layer with the highest local height wins a pixel instead of all layers averaging together.
///
/// This turns the transition between two layers from a linear cross-fade into an interlocking one:
/// where a rock texture has raised stones its height is large, so the rock keeps those pixels even
/// at a low splat weight, while the crevices between the stones fall below the neighbouring layer's
/// height and show sand or dirt instead.
///
/// Each weight is biased by its height, then only layers within 'depth' of the highest biased value
/// survive; the survivors are renormalized so the result still sums to 1.
///
/// The bias is scaled by the incoming weight, so a layer that is not painted at a pixel (weight 0)
/// can never appear there no matter how tall its height map is.
///
/// depth: transition width. Small values (~0.02) give hard, per-stone cutout edges, large values
/// (~0.5) approach the plain linear blend. Must be > 0, otherwise ties would produce zero total
/// weight and the original weights are kept.
/// strength: how far a height value may push a layer. 0 reproduces the linear blend exactly.
///
/// Weights whose corresponding entry in 'heights' is unused (layer disabled via TERRAIN_LAYER_COUNT)
/// must be passed as 0 so they drop out of the maximum.
void TerrainHeightBlendWeights(inout float weights[TERRAIN_MAX_BLEND_LAYERS], float heights[TERRAIN_MAX_BLEND_LAYERS], float depth, float strength)
{
  float biased[TERRAIN_MAX_BLEND_LAYERS];
  float peak = 0.0;

  [unroll] for (uint i = 0; i < TERRAIN_MAX_BLEND_LAYERS; ++i)
  {
    // Scaling the height term by the weight keeps unpainted layers (weight 0) out of the result
    // and makes the bias fade in smoothly as a layer is painted in.
    biased[i] = weights[i] * (1.0 + heights[i] * strength);
    peak = max(peak, biased[i]);
  }

  const float cutoff = peak - depth;

  float total = 0.0;
  float result[TERRAIN_MAX_BLEND_LAYERS];

  [unroll] for (uint j = 0; j < TERRAIN_MAX_BLEND_LAYERS; ++j)
  {
    result[j] = max(biased[j] - cutoff, 0.0);
    total += result[j];
  }

  // total can only reach 0 if every weight was 0 (a pixel with no layers at all). Keep the
  // input weights in that case rather than dividing by zero.
  if (total <= 0.0)
    return;

  const float rcpTotal = 1.0 / total;

  [unroll] for (uint k = 0; k < TERRAIN_MAX_BLEND_LAYERS; ++k)
  {
    weights[k] = result[k] * rcpTotal;
  }
}
