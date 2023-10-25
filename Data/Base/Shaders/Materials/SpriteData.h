#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>

struct EZ_SHADER_STRUCT ezPerSpriteData
{
  FLOAT3(WorldSpacePosition);
  FLOAT1(Size);
  FLOAT1(MaxScreenSize);
  FLOAT1(AspectRatio);
  UINT1(ColorRG);
  UINT1(ColorBA);
  UINT1(TexCoordScale);
  UINT1(TexCoordOffset);
  UINT1(GameObjectID);
  UINT1(Reserved);
};
