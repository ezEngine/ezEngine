#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(ezMouseCursorConstants, 3)
{
  FLOAT4(CursorPositionAndSize);    // xy = mouse position in pixels, zw = cursor size in pixels
  FLOAT4(CursorHotspotAndRotation); // xy = normalized hotspot, z = sin(rotation), w = cos(rotation)
  FLOAT4(CursorUvRect);             // xy = top-left UV, zw = bottom-right UV
  FLOAT4(CursorTargetSize);         // xy = render target size in pixels, zw = 1 / size
  COLOR4F(CursorColor);
};
