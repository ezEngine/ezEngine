#pragma once

#include <Shaders/Common/Common.h>

float4 EvaluateAmbientCube(Texture2D ambientCubeTexture, int slotIndex, float3 normal)
{
  float3 normalSquared = normal * normal;
  int3 isNegative = normal < 0.0;
  return normalSquared.x * ambientCubeTexture.Load(int3(isNegative.x + 0, slotIndex, 0)) +
         normalSquared.y * ambientCubeTexture.Load(int3(isNegative.y + 2, slotIndex, 0)) +
         normalSquared.z * ambientCubeTexture.Load(int3(isNegative.z + 4, slotIndex, 0));
}
