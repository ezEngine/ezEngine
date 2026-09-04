#pragma once

/// Triplanar sampling helpers shared by the terrain and voxel material shaders.
/// Include after MaterialPixelShader.h, which provides DecodeNormalTexture.
///
/// Sampling only exists in explicit-gradient form. The shaders skip zero-weight layers inside
/// branches, where a plain Sample's implicit derivatives would be undefined.

/// Weight at which one projection covers the pixel on its own and the other two fetches are
/// skipped. Weights sum to 1, so this leaves under a 1000th for the other two - less than an
/// 8-bit texture can represent.
#define TERRAIN_TRIPLANAR_DOMINANT 0.999

/// Triplanar blend weights matching the formula used by the engine's SampleTexture3Way helper.
float3 TriplanarWeights(float3 worldNormal)
{
  float3 w = abs(worldNormal);
  w = max((w - 0.2) * 7.0, 0.0);
  return w / (w.x + w.y + w.z);
}

/// Samples one layer of a 2D array texture using precomputed triplanar UVs and gradients.
/// Safe inside divergent flow control.
float4 TriplanarSampleColorArrayGrad(Texture2DArray tex, SamplerState samp, TriplanarUVs uv, float3 weights, uint2 matIndex)
{
  const float2 layer = (float2)matIndex;

  // TriplanarWeights drives the off-axis weights to exactly 0 on near-axis-aligned surfaces,
  // which on a heightfield is most of the screen.
  if (weights.z >= TERRAIN_TRIPLANAR_DOMINANT)
    return tex.SampleGrad(samp, float3(uv.UvZ, layer.x), uv.DdxZ, uv.DdyZ);

  const float4 cX = tex.SampleGrad(samp, float3(uv.UvX, layer.y), uv.DdxX, uv.DdyX);
  const float4 cY = tex.SampleGrad(samp, float3(uv.UvY, layer.y), uv.DdxY, uv.DdyY);
  const float4 cZ = tex.SampleGrad(samp, float3(uv.UvZ, layer.x), uv.DdxZ, uv.DdyZ);

  return cX * weights.x + cY * weights.y + cZ * weights.z;
}

/// Normal-map counterpart of TriplanarSampleColorArrayGrad. Each projection's tangent-space normal
/// is rotated into world space by swizzling, with the axis signs applied so that back faces of a
/// projection are not mirrored.
float3 TriplanarSampleNormalArrayGrad(Texture2DArray tex, SamplerState samp, TriplanarUVs uv, float3 weights, uint2 matIndex)
{
  const float2 layer = (float2)matIndex;
  const float3 sn = uv.SignN;

  if (weights.z >= TERRAIN_TRIPLANAR_DOMINANT)
  {
    const float3 nOnly = DecodeNormalTexture(tex.SampleGrad(samp, float3(uv.UvZ, layer.x), uv.DdxZ, uv.DdyZ));
    return normalize(float3(nOnly.x * sn.z, nOnly.y, nOnly.z * sn.z));
  }

  const float3 nX = DecodeNormalTexture(tex.SampleGrad(samp, float3(uv.UvX, layer.y), uv.DdxX, uv.DdyX));
  const float3 nY = DecodeNormalTexture(tex.SampleGrad(samp, float3(uv.UvY, layer.y), uv.DdxY, uv.DdyY));
  const float3 nZ = DecodeNormalTexture(tex.SampleGrad(samp, float3(uv.UvZ, layer.x), uv.DdxZ, uv.DdyZ));

  const float3 wsX = float3(nX.z * sn.x, -nX.x * sn.x, -nX.y);
  const float3 wsY = float3(nY.x * sn.y, nY.z * sn.y, -nY.y);
  const float3 wsZ = float3(nZ.x * sn.z, nZ.y, nZ.z * sn.z);

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
/// depth: transition width, in the same 0..1 scale as the weights (the biased values are
/// renormalized to sum to 1 before it is applied, so it does not drift with 'strength' or with how
/// much height is present). Small values (~0.02) give hard, per-stone cutout edges, large values
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
  float biasedTotal = 0.0;

  [unroll] for (uint i = 0; i < TERRAIN_MAX_BLEND_LAYERS; ++i)
  {
    // Scaling the height term by the weight keeps unpainted layers (weight 0) out of the result
    // and makes the bias fade in smoothly as a layer is painted in.
    biased[i] = weights[i] * (1.0 + heights[i] * strength);
    biasedTotal += biased[i];
    peak = max(peak, biased[i]);
  }

  // Rescale so the biased values sum to 1 again, matching the incoming weights. The height term
  // above inflates the total by an amount that depends on how much height happens to be present at
  // this pixel, and 'depth' is an absolute offset below, so without this the same 'depth' would
  // produce a different band width from pixel to pixel, and would also shift whenever 'strength'
  // changes. Normalizing first keeps 'depth' meaning one thing across the whole surface.
  if (biasedTotal > 0.0)
  {
    const float rcpBiasedTotal = 1.0 / biasedTotal;
    [unroll] for (uint n = 0; n < TERRAIN_MAX_BLEND_LAYERS; ++n)
      biased[n] *= rcpBiasedTotal;
    peak *= rcpBiasedTotal;
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
