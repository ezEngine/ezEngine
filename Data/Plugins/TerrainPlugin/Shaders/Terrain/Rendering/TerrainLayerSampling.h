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
