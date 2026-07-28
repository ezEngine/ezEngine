#pragma once

#include "MouseCursorConstants.h"

// Shared code for shaders that render the custom mouse cursor.
//
// The cursor is a single quad that is placed directly in clip space from ezMouseCursorConstants.
// The quad is generated from SV_VertexID, so no vertex or index buffer is needed
// (the renderer calls ezRenderContext::BindNullMeshBuffer(Triangles, 2)).

struct ezMouseCursorVSOutput
{
  float4 Position : SV_Position;
  float2 TexCoord0 : TEXCOORD0;
};

/// Computes the cursor quad's clip space position and UV for one vertex.
ezMouseCursorVSOutput ezMouseCursorVertex(uint VertexID)
{
  const float2 corners[6] =
  {
    float2(0, 0), float2(1, 0), float2(0, 1),
    float2(1, 0), float2(1, 1), float2(0, 1),
  };

  const float2 corner = corners[VertexID];

  // Position the corner relative to the hotspot, so that the rotation pivots around it.
  // The hotspot is normalized, so that it stays correct at any cursor size.
  float2 offset = (corner - CursorHotspotAndRotation.xy) * CursorPositionAndSize.zw;

  // Screen space is top-down, so a positive angle rotates clockwise on screen.
  const float sinA = CursorHotspotAndRotation.z;
  const float cosA = CursorHotspotAndRotation.w;
  offset = float2(offset.x * cosA - offset.y * sinA, offset.x * sinA + offset.y * cosA);

  const float2 pixelPos = CursorPositionAndSize.xy + offset;

  // Pixels to normalized device coordinates. Y is flipped, because screen space is top-down.
  float2 ndcPos = pixelPos * CursorTargetSize.zw * 2.0 - 1.0;
  ndcPos.y = -ndcPos.y;

  ezMouseCursorVSOutput Output;
  Output.Position = float4(ndcPos, 0.0, 1.0);
  Output.TexCoord0 = lerp(CursorUvRect.xy, CursorUvRect.zw, corner);

  return Output;
}
