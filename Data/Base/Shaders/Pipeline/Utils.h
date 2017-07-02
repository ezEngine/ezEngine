#include <Shaders/Common/GlobalConstants.h>

// Computes linear depth from depth buffer depth.
// Note that computations like this are not set in stone as we may want to move to a different way of storing Z
// (for example flipped near/far plane is quite common for better float precision)
//
// Basically removes the w division from z again.
float LinearizeZBufferDepth(float depthFromZBuffer)
{
  return 1.0f / (depthFromZBuffer * ScreenToCameraMatrix._43 + ScreenToCameraMatrix._44);
}

// Computes view space coordinate for a given pixel position and depth.
//
// This is the same as the following code, but with knowledge/assumptions of the expected zeros and ones in ScreenToCameraMatrix (DX scheme)
// float4 viewPos = mul(ScreenToCameraMatrix, float4(normalizedScreenCor, depthFromZBuffer, 1.0f));
// viewPos.xyz /= viewPos.w;
//
// Note that computations like this are not set in stone as we may want to move to a different way of storing Z
// (for example flipped near/far plane is quite common for better float precision)
float3 FastScreenCoordToViewSpace(float2 normalizedScreenCor, float depthFromZBuffer)
{
  // float4 viewPos = mul(ScreenToCameraMatrix, float4(normalizedScreenCor, depthFromZBuffer, 1.0f));
  //viewPos.xyz /= viewPos.w;
  //return viewPos.xyz;

  float linearZ = LinearizeZBufferDepth(depthFromZBuffer);
  return float3(float2(ScreenToCameraMatrix._11, ScreenToCameraMatrix._22) * normalizedScreenCor * linearZ, linearZ);
}

// Same as FastScreenCoordToViewSpace but uses an already linearized depth value.
float3 FastScreenCoordToViewSpaceLinear(float2 normalizedScreenCor, float linearDepth)
{
  return float3(float2(ScreenToCameraMatrix._11, ScreenToCameraMatrix._22) * normalizedScreenCor * linearDepth, linearDepth);
}

// Returns the distance to mid that has the smallest absolute value.
float SmallerAbsDelta(float left, float mid, float right)
{
  float a = mid - left;
  float b = right - mid;

  return (abs(a) < abs(b)) ? a : b;
}

// Computes view space normal from the given set of depth values.
//
// All depth values come directly from the depth buffer. normalizedCoords is in [-1, 1] space and the position of centerDepth.
// All other depth values should be sampled 1 pixel offset in each direction.
float3 ReconstructViewSpaceNormal(float2 normalizedCoords, float centerDepth, float leftDepth, float rightDepth, float topDepth, float bottomDepth)
{
  float2 texelSize = ViewportSize.zw;

  float zDDX = SmallerAbsDelta(leftDepth, centerDepth, rightDepth);
  float zDDY = SmallerAbsDelta(topDepth, centerDepth, bottomDepth);

  float3 centerPos = FastScreenCoordToViewSpace(normalizedCoords, centerDepth);
  float3 rightDir =	FastScreenCoordToViewSpace(normalizedCoords + float2(texelSize.x * 2, 0), centerDepth + zDDX) - centerPos;
  float3 downDir =  FastScreenCoordToViewSpace(normalizedCoords + float2(0, texelSize.y * 2), centerDepth + zDDY) - centerPos;

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
