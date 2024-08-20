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
StructuredBuffer<ezPerInstanceData> perInstanceData;

#  if defined(USE_SKINNING)
StructuredBuffer<Transform> skinningTransforms;
#  endif

// TODO: this doesn't work at the moment
// Buffer<uint> perInstanceVertexColors;
StructuredBuffer<uint> perInstanceVertexColors;

#else // C++

EZ_DEFINE_AS_POD_TYPE(ezPerInstanceData);

static_assert(sizeof(ezPerInstanceData) == 144);
#endif

CONSTANT_BUFFER(ezObjectConstants, 2)
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

uint GetInstanceVertexColorsHelper(uint accessData, uint vertexID, uint colorIndex)
{
  uint numColorsPerVertex = GetNumInstanceVertexColorsHelper(accessData);
  uint offset = (accessData & VERTEX_COLOR_ACCESS_OFFSET_MASK) + (vertexID * numColorsPerVertex + colorIndex);
  return colorIndex < numColorsPerVertex ? perInstanceVertexColors[offset] : 0;
}

#  define GetNumInstanceVertexColors() GetNumInstanceVertexColorsHelper(GetInstanceData().VertexColorAccessData)
#  define GetInstanceVertexColors(colorIndex) GetInstanceVertexColorsHelper(GetInstanceData().VertexColorAccessData, G.Input.VertexID, colorIndex)

#endif
