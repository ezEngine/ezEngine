#pragma once

#include "ConstantBufferMacros.h"
#include "Platforms.h"

struct EZ_SHADER_STRUCT ezPerInstanceData
{
  TRANSFORM(ObjectToWorld);
  TRANSFORM(ObjectToWorldNormal);
  FLOAT1(BoundingSphereRadius);
  UINT1(GameObjectID);
  UINT1(VertexColorAccessData);

  INT1(Reserved);
  COLOR4F(Color);
  FLOAT4(CustomData);
};

#if EZ_ENABLED(PLATFORM_SHADER)
#  include "Common.h"
StructuredBuffer<ezPerInstanceData> perInstanceData BIND_GROUP(BG_DRAW_CALL);

#  if defined(USE_SKINNING)
StructuredBuffer<Transform> skinningTransforms BIND_GROUP(BG_DRAW_CALL);
#  endif

#  if EZ_ENABLED(SUPPORTS_TEXEL_BUFFER)
Buffer<float4> perInstanceVertexColors BIND_GROUP(BG_DRAW_CALL);
#  else
StructuredBuffer<uint> perInstanceVertexColors BIND_GROUP(BG_DRAW_CALL);
#  endif


#else // C++

EZ_DEFINE_AS_POD_TYPE(ezPerInstanceData);

static_assert(sizeof(ezPerInstanceData) == 144);
#endif

CONSTANT_BUFFER2(ezObjectConstants, 2, BG_DRAW_CALL)
{
  UINT1(InstanceDataOffset);
};



#if EZ_ENABLED(PLATFORM_SHADER)

// Access to instance should usually go through this macro!
// It's a macro so it can work with arbitrary input structs (for VS/GS/PS...)
#  if defined(CAMERA_MODE) && CAMERA_MODE == CAMERA_MODE_STEREO
#    define GetInstanceData() perInstanceData[G.Input.InstanceID / 2 + InstanceDataOffset]
#  else
#    define GetInstanceData() perInstanceData[G.Input.InstanceID + InstanceDataOffset]
#  endif

#  define VERTEX_COLOR_ACCESS_OFFSET_BITS 28
#  define VERTEX_COLOR_ACCESS_OFFSET_MASK ((1 << VERTEX_COLOR_ACCESS_OFFSET_BITS) - 1)

uint GetNumInstanceVertexColorsHelper(uint accessData)
{
  return accessData >> VERTEX_COLOR_ACCESS_OFFSET_BITS;
}

float4 GetInstanceVertexColorsHelper(uint accessData, uint vertexID, uint colorIndex)
{
  uint numColorsPerVertex = GetNumInstanceVertexColorsHelper(accessData);
  uint offset = (accessData & VERTEX_COLOR_ACCESS_OFFSET_MASK) + (vertexID * numColorsPerVertex + colorIndex);
#  if EZ_ENABLED(SUPPORTS_TEXEL_BUFFER)
  return (colorIndex < numColorsPerVertex) ? perInstanceVertexColors[offset] : 0;
#  else
  uint packedColor = (colorIndex < numColorsPerVertex) ? perInstanceVertexColors[offset] : 0;
  return RGBA8ToFloat4(packedColor);
#  endif
}

#  define GetNumInstanceVertexColors() GetNumInstanceVertexColorsHelper(GetInstanceData().VertexColorAccessData)
#  define GetInstanceVertexColors(colorIndex) GetInstanceVertexColorsHelper(GetInstanceData().VertexColorAccessData, G.Input.VertexID, colorIndex)

#endif
