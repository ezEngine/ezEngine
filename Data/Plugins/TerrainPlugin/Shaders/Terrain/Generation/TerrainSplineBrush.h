#pragma once

#include <Shaders/Terrain/Generation/TerrainBrushData.h>
#include <Shaders/Terrain/Generation/TerrainUtils.h>

/// Polyline nodes of all spline brushes, indexed through TerrainBrushData::FirstSplineNode.
/// Contains a single dummy entry when no brush follows a spline.
StructuredBuffer<TerrainSplineNode> SplineNodes BIND_GROUP(BG_DRAW_CALL);

/// Finds the point on a spline brush's polyline that is closest to p.
///
/// With bPlanar set, the distance is measured in XY only, which is what 2D brushes need: a vertex is
/// affected by the path segment below or above it, regardless of height. The returned position still
/// carries the interpolated height of the path.
///
/// outEnd is -1 or +1 when p lies beyond the start or the end of the spline and that end is capped,
/// 0 otherwise. Within a single segment the frame is constant, except for the up direction, which is
/// interpolated so that banking changes smoothly along the path.
void Spline_FindClosest(TerrainBrushData brush, float3 p, bool bPlanar, out float3 outPos, out float3 outTangent, out float3 outUp, out float outArc, out int outEnd)
{
  const uint firstNode = brush.FirstSplineNode;
  const uint lastNode = firstNode + brush.SplineNodeCount - 1;

  uint bestSeg = firstNode;
  float bestT = 0.0;
  float bestDistSq = 3.0e38;

  [loop] for (uint i = firstNode; i < lastNode; ++i)
  {
    float3 ab = SplineNodes[i + 1].Position - SplineNodes[i].Position;
    float3 ap = p - SplineNodes[i].Position;

    if (bPlanar)
    {
      ab.z = 0.0;
      ap.z = 0.0;
    }

    const float lenSq = dot(ab, ab);
    const float t = (lenSq > 1e-8) ? saturate(dot(ap, ab) / lenSq) : 0.0;
    const float3 d = ap - ab * t;
    const float distSq = dot(d, d);

    if (distSq < bestDistSq)
    {
      bestDistSq = distSq;
      bestSeg = i;
      bestT = t;
    }
  }

  const TerrainSplineNode n0 = SplineNodes[bestSeg];
  const TerrainSplineNode n1 = SplineNodes[bestSeg + 1];

  outPos = lerp(n0.Position, n1.Position, bestT);
  outArc = lerp(n0.ArcLength, n1.ArcLength, bestT);

  const float3 seg = n1.Position - n0.Position;
  outTangent = (dot(seg, seg) > 1e-8) ? normalize(seg) : float3(1.0, 0.0, 0.0);

  const float3 up = lerp(n0.UpDir, n1.UpDir, bestT);
  const float3 upOrtho = up - outTangent * dot(up, outTangent);
  outUp = (dot(upOrtho, upOrtho) > 1e-8) ? normalize(upOrtho) : float3(0.0, 0.0, 1.0);

  outEnd = 0;
  if (bestSeg == firstNode && bestT <= 0.0 && (brush.SplineFlags & ezTerrainSplineFlags_StartCap) != 0)
    outEnd = -1;
  else if (bestSeg + 1 == lastNode && bestT >= 1.0 && (brush.SplineFlags & ezTerrainSplineFlags_EndCap) != 0)
    outEnd = 1;
}

/// Returns the equivalent of a box brush's 'abs(localP.x) - halfExtentX' for a spline brush.
///
/// Beyond a capped end this is the distance past that end. Along the path it is the negative arc
/// distance to the nearer end, so the corners at the ends are shaped exactly like those of a box brush.
/// Closed splines have no ends, which is expressed with a very large negative value.
float Spline_AxialDistance(TerrainBrushData brush, float arc, int end, float3 toPoint, float3 tangent)
{
  if (end != 0)
    return max(dot(toPoint, tangent) * (float)end, 0.0);

  if ((brush.SplineFlags & ezTerrainSplineFlags_Closed) != 0)
    return -1.0e5;

  return -min(arc, brush.SplineLength - arc);
}

/// Spline counterpart of Brush_Sdf3D: the same trapezoid cross-section, swept along the polyline.
float Brush_SplineSdf3D(TerrainBrushData brush, float3 toPoint3D)
{
  UNPACKHALF2(halfExtentX, halfExtentZ, brush.PackedExtentsXZ);
  UNPACKHALF2(halfExtentYBot, halfExtentYTop, brush.PackedExtentsY);
  UNPACKHALF2(innerRadius, outerRadius, brush.PackedRadii);

  const float3 p = toPoint3D + brush.Position;

  float3 pos, tangent, up;
  float arc;
  int end;
  Spline_FindClosest(brush, p, false, pos, tangent, up, arc, end);

  const float3 d = p - pos;
  const float vertical = dot(d, up);

  // Inside the path d is already perpendicular to the tangent. At an outer corner between two segments
  // it is not, and keeping the tangential part rounds that corner instead of leaving a notch.
  const float3 dCross = (end != 0) ? (d - tangent * dot(d, tangent)) : d;
  const float lateral = length(dCross - up * vertical);

  const float trapSdf = sdTrapezoid(float2(lateral, vertical), halfExtentYBot, halfExtentYTop, halfExtentZ);
  const float xDist = Spline_AxialDistance(brush, arc, end, d, tangent) - halfExtentX;
  return length(max(float2(xDist, trapSdf), 0.0)) + min(max(xDist, trapSdf), 0.0) - outerRadius - innerRadius;
}

/// Spline counterpart of Brush_Sdf2D: the rectangular footprint swept along the polyline in XY.
/// dz follows the height of the path and its banking, so brushHeight = brush.Position.z + dz still holds.
float Brush_SplineSdf2D(TerrainBrushData brush, float2 toPointXY, out float dz)
{
  UNPACKHALF2(halfExtentX, halfExtentZ, brush.PackedExtentsXZ);
  UNPACKHALF2(halfExtentYBot, halfExtentYTop, brush.PackedExtentsY);
  UNPACKHALF2(innerRadius, outerRadius, brush.PackedRadii);

  const float3 p = float3(toPointXY + brush.Position.xy, 0.0);

  float3 pos, tangent, up;
  float arc;
  int end;
  Spline_FindClosest(brush, p, true, pos, tangent, up, arc, end);

  // Same as Brush_Sdf2D: intersect the vertical line through the sample with the brush plane, which
  // here is the plane through the closest path point.
  const float2 dXY = p.xy - pos.xy;
  const float planeDz = (abs(up.z) > 0.001) ? -(up.x * dXY.x + up.y * dXY.y) / up.z : 0.0;
  dz = pos.z + planeDz - brush.Position.z;

  const float3 d = float3(dXY, planeDz);
  const float lateral = (end != 0) ? length(d - tangent * dot(d, tangent)) : length(d);

  const float2 q2 = float2(Spline_AxialDistance(brush, arc, end, d, tangent) - halfExtentX, lateral - halfExtentYBot);
  return min(max(q2.x, q2.y), 0.0) + length(max(q2, float2(0.0, 0.0))) - outerRadius - innerRadius;
}

/// Brush_Sdf3D for any brush, dispatching to the spline version where needed.
float Brush_EvalSdf3D(TerrainBrushData brush, float3 toPoint3D)
{
  if (brush.SplineNodeCount >= 2)
    return Brush_SplineSdf3D(brush, toPoint3D);

  return Brush_Sdf3D(brush, toPoint3D);
}

/// Brush_Sdf2D for any brush, dispatching to the spline version where needed.
float Brush_EvalSdf2D(TerrainBrushData brush, float2 toPointXY, out float dz)
{
  if (brush.SplineNodeCount >= 2)
    return Brush_SplineSdf2D(brush, toPointXY, dz);

  return Brush_Sdf2D(brush, toPointXY, dz);
}
