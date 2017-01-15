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