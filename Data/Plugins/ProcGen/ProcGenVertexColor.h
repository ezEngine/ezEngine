#pragma once

#include <Shaders/Common/Platforms.h>

#define VERTEX_COLOR_ACCESS_OFFSET_BITS 28
#define VERTEX_COLOR_ACCESS_OFFSET_MASK ((1 << VERTEX_COLOR_ACCESS_OFFSET_BITS) - 1)

#if EZ_ENABLED(PLATFORM_SHADER)
#  if EZ_ENABLED(SUPPORTS_TEXEL_BUFFER)
Buffer<float4> perInstanceDataCustom BIND_GROUP(BG_DRAW_CALL);
#  else
StructuredBuffer<uint> perInstanceDataCustom BIND_GROUP(BG_DRAW_CALL);
#  endif

uint GetNumInstanceVertexColorsHelper(uint accessData)
{
  return accessData >> VERTEX_COLOR_ACCESS_OFFSET_BITS;
}

float4 GetInstanceVertexColorsHelper(uint accessData, uint vertexID, uint colorIndex)
{
  uint numColorsPerVertex = GetNumInstanceVertexColorsHelper(accessData);
  uint offset = (accessData & VERTEX_COLOR_ACCESS_OFFSET_MASK) + (vertexID * numColorsPerVertex + colorIndex);
#  if EZ_ENABLED(SUPPORTS_TEXEL_BUFFER)
  return (colorIndex < numColorsPerVertex) ? perInstanceDataCustom[offset] : 0;
#  else
  uint packedColor = (colorIndex < numColorsPerVertex) ? perInstanceDataCustom[offset] : 0;
  return RGBA8ToFloat4(packedColor);
#  endif
}

#  define GetNumInstanceVertexColors() GetNumInstanceVertexColorsHelper(G.Input.DataOffsets.y)
#  define GetInstanceVertexColors(colorIndex) GetInstanceVertexColorsHelper(G.Input.DataOffsets.y, G.Input.VertexID, colorIndex)
#endif
