#include <Shaders/Common/GlobalConstants.h>

// Returns the distance to mid that has the smallest absolute value.
float SmallerAbsDelta(float left, float mid, float right)
{
  float a = mid - left;
  float b = right - mid;

  return (abs(a) < abs(b)) ? a : b;
}

/// Reconstructs a camera-space normal for a centered perspective projection.
///
/// ndcXY is the center position in [-1, +1]. Depths are raw device values sampled
/// at the center and one pixel to its left, right, top, and bottom.
float3 ReconstructCameraNormalFromDeviceDepth(float2 ndcXY, float centerDepth, float leftDepth, float rightDepth, float topDepth, float bottomDepth)
{
  float2 texelSize = ViewportSize.zw;

  float zDDX = SmallerAbsDelta(leftDepth, centerDepth, rightDepth);
  float zDDY = SmallerAbsDelta(topDepth, centerDepth, bottomDepth);

  float3 centerPos = ReconstructPerspectiveCameraPositionFromDeviceDepth(ndcXY, centerDepth);
  float3 rightDir = ReconstructPerspectiveCameraPositionFromDeviceDepth(ndcXY + float2(texelSize.x * 2, 0), centerDepth + zDDX) - centerPos;
  float3 downDir = ReconstructPerspectiveCameraPositionFromDeviceDepth(ndcXY + float2(0, texelSize.y * 2), centerDepth + zDDY) - centerPos;

  return normalize(cross(downDir, rightDir));
}

// Computes a weight for the given sample position according to the view space normal of the center position.
// Samples that are on the same plane as constructed by centerPos and viewSpaceNormal will be weighted higher than
// those smaples that are above or below the plane.
float ComputeNormalWeight(float3 viewSpaceNormal, float3 centerPos, float3 samplePos, float multiplier)
{
  float3 dir = (samplePos - centerPos);
  // We do not normalize dir as that produces noise in the distance due to precision issues.
  // Leaving the length in nicely compensates for that.
  float angle = abs(dot(dir, viewSpaceNormal));
  return exp(-angle * angle * multiplier * multiplier);
}
