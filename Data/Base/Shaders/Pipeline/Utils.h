#include <Shaders/Common/GlobalConstants.h>

// Computes linear depth from depth buffer depth.
// Note that computations like this are not set in stone as we may want to move to a different way of storing Z
// (for example flipped near/far plane is quite common for better float precision)
//
// Basically removes the w division from z again.
float LinearizeZBufferDepth(float depthFromZBuffer)
{
  return CameraToScreenMatrix._34 / (depthFromZBuffer - CameraToScreenMatrix._33);
}