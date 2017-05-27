#pragma once

#include "Platforms.h"
#include "ConstantBufferMacros.h"

struct EZ_ALIGN_16(ezPerInstanceData)
{
  TRANSFORM(ObjectToWorld);
  TRANSFORM(ObjectToWorldNormal);
  INT1(GameObjectID);

  INT3(Reserved);
  FLOAT4(Reserved2);
};

#if EZ_ENABLED(PLATFORM_DX11)
  StructuredBuffer<ezPerInstanceData> perInstanceData;
#else
  EZ_CHECK_AT_COMPILETIME(sizeof(ezPerInstanceData) == 128);
#endif

CONSTANT_BUFFER(ezObjectConstants, 2)
{
  UINT1(InstanceDataOffset);
};
  
