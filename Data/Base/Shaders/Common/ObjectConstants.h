#pragma once

#include "Platforms.h"
#include "ConstantBufferMacros.h"

struct EZ_ALIGN_16(ezPerInstanceData)
{
  TRANSFORM(ObjectToWorld);
  TRANSFORM(ObjectToWorldNormal);
  INT1(GameObjectID);
  FLOAT1(GlobalUniformScale); // only used by some renderers, ie ones that need to span billboards

  INT2(Reserved);
  COLOR4F(Color);
};

#if EZ_ENABLED(PLATFORM_DX11)
  StructuredBuffer<ezPerInstanceData> perInstanceData;

  #if defined(USE_SKINNING)
  StructuredBuffer<float4x4> skinningMatrices;
  #endif

#else // C++

  EZ_DEFINE_AS_POD_TYPE(ezPerInstanceData);

  EZ_CHECK_AT_COMPILETIME(sizeof(ezPerInstanceData) == 128);
#endif

CONSTANT_BUFFER(ezObjectConstants, 2)
{
  UINT1(InstanceDataOffset);
};



#if EZ_ENABLED(PLATFORM_DX11)

  // Access to instance should usually go through this macro!
  // It's a macro so it can work with arbitrary input structs (for VS/GS/PS...)
  #if defined(CAMERA_MODE) && CAMERA_MODE == CAMERA_MODE_STEREO
    #define GetInstanceData() perInstanceData[G.Input.InstanceID/2 + InstanceDataOffset]
  #else
    #define GetInstanceData() perInstanceData[G.Input.InstanceID + InstanceDataOffset]
  #endif

#endif
