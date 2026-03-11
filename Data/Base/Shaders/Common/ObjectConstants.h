#pragma once

#include "ConstantBufferMacros.h"
#include "Platforms.h"

struct EZ_SHADER_STRUCT ezPerInstanceData
{
  TRANSFORM(ObjectToWorld);
  TRANSFORM(ObjectToWorldNormal);
  FLOAT1(BoundingSphereRadius);
  UINT1(GameObjectID);
  UINT1(RandomSeed);

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

#else // C++

EZ_DEFINE_AS_POD_TYPE(ezPerInstanceData);

static_assert(sizeof(ezPerInstanceData) == 144);
#endif

#if EZ_ENABLED(PLATFORM_SHADER)

// Access to instance should usually go through this macro!
// It's a macro so it can work with arbitrary input structs (for VS/GS/PS...)
#  define GetInstanceData() perInstanceData[G.Input.DataOffsets.x]
#  define GetCustomInstanceData() perInstanceData[G.Input.DataOffsets.y]

#endif
