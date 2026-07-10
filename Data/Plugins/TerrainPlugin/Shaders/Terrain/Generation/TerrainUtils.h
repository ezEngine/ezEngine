#pragma once

#include <Shaders/Terrain/Generation/TerrainBrushData.h>

/// 2D hash producing a pseudo-random value in [0, 1) from an integer-lattice point.
/// Integer-only arithmetic — fully deterministic across all GPU hardware and driver versions.
float Hash2D(float2 p)
{
  uint2 q = (uint2)(int2(p));
  q *= uint2(1597334677u, 3812015801u);
  uint n = (q.x ^ q.y) * 1597334677u;
  return float(n) * (1.0 / 4294967296.0);
}

/// 2D value noise on a unit lattice, output in [0, 1].
float ValueNoise2D(float2 p)
{
  float2 i = floor(p);
  float2 f = frac(p);
  float2 u = f * f * (3.0 - 2.0 * f);

  float a = Hash2D(i + float2(0.0, 0.0));
  float b = Hash2D(i + float2(1.0, 0.0));
  float c = Hash2D(i + float2(0.0, 1.0));
  float d = Hash2D(i + float2(1.0, 1.0));

  return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

/// 2-octave fbm, output remapped to [-1, 1].
float FBM2D(float2 p)
{
  float n = ValueNoise2D(p) * 0.6667 + ValueNoise2D(p * 2.03) * 0.3333;
  return n * 2.0 - 1.0;
}

/// 3D hash producing a pseudo-random value in [0, 1).
/// Integer-only arithmetic — fully deterministic across all GPU hardware and driver versions.
float Hash3D(float3 p)
{
  uint3 q = (uint3)(int3(p));
  q *= uint3(1597334677u, 3812015801u, 2912667907u);
  uint n = (q.x ^ q.y ^ q.z) * 1597334677u;
  return float(n) * (1.0 / 4294967296.0);
}

/// 3D value noise on a unit lattice, output in [0, 1].
float ValueNoise3D(float3 p)
{
  float3 i = floor(p);
  float3 f = frac(p);
  float3 u = f * f * (3.0 - 2.0 * f);

  float a = Hash3D(i + float3(0.0, 0.0, 0.0));
  float b = Hash3D(i + float3(1.0, 0.0, 0.0));
  float c = Hash3D(i + float3(0.0, 1.0, 0.0));
  float d = Hash3D(i + float3(1.0, 1.0, 0.0));
  float e = Hash3D(i + float3(0.0, 0.0, 1.0));
  float g = Hash3D(i + float3(1.0, 0.0, 1.0));
  float h = Hash3D(i + float3(0.0, 1.0, 1.0));
  float k = Hash3D(i + float3(1.0, 1.0, 1.0));

  float ab = lerp(a, b, u.x);
  float cd = lerp(c, d, u.x);
  float eg = lerp(e, g, u.x);
  float hk = lerp(h, k, u.x);
  return lerp(lerp(ab, cd, u.y), lerp(eg, hk, u.y), u.z);
}

/// 2-octave 3D fbm, output remapped to [-1, 1].
float FBM3D(float3 p)
{
  float n = ValueNoise3D(p) * 0.6667 + ValueNoise3D(p * 2.03) * 0.3333;
  return n * 2.0 - 1.0;
}

// 2D isosceles trapezoid SDF (Inigo Quilez).
// p.x = lateral offset, p.y = vertical offset along the trapezoid height axis.
// r1 = half-width at bottom (p.y = -h), r2 = half-width at top (p.y = +h), h = half-height.
// When r1 == r2 degenerates to a 2D box SDF.
float sdTrapezoid(float2 p, float r1, float r2, float h)
{
  const float2 k1 = float2(r2, h);
  const float2 k2 = float2(r2 - r1, 2.0 * h);
  p.x = abs(p.x);
  const float2 ca = float2(p.x - min(p.x, (p.y < 0.0) ? r1 : r2), abs(p.y) - h);
  const float denom = dot(k2, k2);
  const float2 cb = p - k1 + k2 * clamp((denom > 1e-6) ? (dot(k1 - p, k2) / denom) : 0.0, 0.0, 1.0);
  const float s = (cb.x < 0.0 && ca.y < 0.0) ? -1.0 : 1.0;
  return s * sqrt(min(dot(ca, ca), dot(cb, cb)));
}

/// Returns a blend weight in [0, 1] for a point inside a brush (distanceToBrush <= 0).
/// Weight is 1 deep inside and falls to 0 at the outer boundary (distanceToBrush == 0).
/// falloff is a power exponent: 1 = smooth curve, >1 = more center-concentrated, <1 = flatter.
float CalculateOuterInfluence(float distanceToBrush, float outerRadius, float falloff)
{
  const float t = saturate(-distanceToBrush / outerRadius);
  return pow(smoothstep(0.0, 1.0, t), falloff);
}

// Brush Utilities

/// Perturbs distanceToBrushXY with FBM noise faded toward the brush edge.
/// Returns the raw noise value in [-1, 1] for downstream use (e.g. height offset).
/// No-op when noiseStrength == 0 or outerRadius == 0.
float Brush_ApplyNoise2D(float2 worldXY, float noiseStrength, float noiseFrequency, float outerRadius, float outerRadiusClamped, inout float distanceToBrushXY)
{
  float noise = 0.0;
  if (noiseStrength > 0.0 && outerRadius > 0.0)
  {
    noise = FBM2D(worldXY / noiseFrequency);
    const float tEdge = saturate(-distanceToBrushXY / outerRadiusClamped);
    const float envelope = 1.0 - exp(-6.0 * tEdge);
    distanceToBrushXY += noise * envelope * noiseStrength * outerRadiusClamped;
  }
  return noise;
}

/// Perturbs distanceToBrush with FBM noise faded toward the brush edge.
/// No-op when noiseStrength == 0 or brushRadius == 0.
/// brushRadius controls the fade envelope width (callers typically pass InnerRadius).
float Brush_ApplyNoise3D(float3 worldXYZ, float noiseStrength, float noiseFrequency, float brushRadius, inout float distanceToBrush)
{
  float noise = 0.0;
  if (noiseStrength > 0.0 && brushRadius > 0.0)
  {
    noise = FBM3D(worldXYZ / noiseFrequency);
    const float tEdge = saturate(-distanceToBrush / brushRadius);
    const float envelope = 1.0 - exp(-6.0 * tEdge);
    distanceToBrush += noise * envelope * noiseStrength * brushRadius;
  }
  return noise;
}

/// Returns the inverse-rotation matrix for a brush, built from its three packed row vectors.
float3x3 Brush_GetInvRotMat(TerrainBrushData brush)
{
  return float3x3(brush.InvRotRow0, brush.InvRotRow1, brush.InvRotRow2);
}

/// 3D SDF of the brush shape: a trapezoid cross-section in the local YZ plane, extruded along local X.
/// The shape is expanded by InnerRadius (rounds the trapezoid corners) then by OuterRadius (falloff zone).
/// Returns 0 at the outer boundary, negative inside. Use CalculateOuterInfluence with OuterRadius for falloff.
/// toPoint3D: world-space vector from brush position to the sample point.
float Brush_Sdf3D(TerrainBrushData brush, float3 toPoint3D)
{
  UNPACKHALF2(halfExtentX, halfExtentZ, brush.PackedExtentsXZ);
  UNPACKHALF2(halfExtentYBot, halfExtentYTop, brush.PackedExtentsY);
  UNPACKHALF2(innerRadius, outerRadius, brush.PackedRadii);
  const float3 localP = mul(Brush_GetInvRotMat(brush), toPoint3D);
  const float trapSdf = sdTrapezoid(float2(localP.y, localP.z), halfExtentYBot, halfExtentYTop, halfExtentZ);
  const float xDist = abs(localP.x) - halfExtentX;
  return length(max(float2(xDist, trapSdf), 0.0)) + min(max(xDist, trapSdf), 0.0) - outerRadius - innerRadius;
}

/// 2D SDF of the brush footprint projected onto its local Z=0 plane.
/// toPointXY: world-space XY vector from brush position to the sample point.
/// dz: output — the Z delta from the brush origin to the plane at this XY;
///     brushHeight = brush.Position.z + dz.
/// The vertical line at toPointXY is intersected with the brush plane (normal = InvRotRow2) so
/// the footprint SDF is a true 2D test in brush-local XY regardless of brush tilt.
/// Returns negative when inside; InnerRadius (corner rounding) and OuterRadius (falloff zone) are already subtracted.
float Brush_Sdf2D(TerrainBrushData brush, float2 toPointXY, out float dz)
{
  UNPACKHALF2(halfExtentX, halfExtentZ, brush.PackedExtentsXZ);
  UNPACKHALF2(halfExtentYBot, halfExtentYTop, brush.PackedExtentsY);
  UNPACKHALF2(innerRadius, outerRadius, brush.PackedRadii);
  const float3x3 invRot = Brush_GetInvRotMat(brush);
  const float3 brushNormal = invRot[2];
  dz = (abs(brushNormal.z) > 0.001)
         ? -(brushNormal.x * toPointXY.x + brushNormal.y * toPointXY.y) / brushNormal.z
         : 0.0;
  const float2 localP = mul(invRot, float3(toPointXY, dz)).xy;
  const float2 q2 = abs(localP) - float2(halfExtentX, halfExtentYBot);

  return min(max(q2.x, q2.y), 0.0) + length(max(q2, float2(0.0, 0.0))) - outerRadius - innerRadius;
}
