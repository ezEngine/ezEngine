#pragma once

#include <Shaders/Common/Common.h>

#define AMBIENT_CUBE_NUM_DIRS 6

struct AmbientCubeFloat3
{
  float3 m_Values[AMBIENT_CUBE_NUM_DIRS];
};

void FillAmbientCubeFromSample(inout AmbientCubeFloat3 ac, float3 value, float3 dir, float weight)
{
  ac.m_Values[0] = value * saturate(dir.x * weight);
  ac.m_Values[1] = value * saturate(-dir.x * weight);
  ac.m_Values[2] = value * saturate(dir.y * weight);
  ac.m_Values[3] = value * saturate(-dir.y * weight);
  ac.m_Values[4] = value * saturate(dir.z * weight);
  ac.m_Values[5] = value * saturate(-dir.z * weight);
}

void AddAmbientCube(inout AmbientCubeFloat3 a, AmbientCubeFloat3 b)
{
  [unroll]
  for (uint i = 0; i < AMBIENT_CUBE_NUM_DIRS; ++i)
  {
    a.m_Values[i] += b.m_Values[i];
  }
}

float4 EvaluateAmbientCube(Texture2D ambientCubeTexture, int slotIndex, float3 normal)
{
  float3 normalSquared = normal * normal;
  int3 isNegative = normal < 0.0;
  return normalSquared.x * ambientCubeTexture.Load(int3(isNegative.x + 0, slotIndex, 0)) +
         normalSquared.y * ambientCubeTexture.Load(int3(isNegative.y + 2, slotIndex, 0)) +
         normalSquared.z * ambientCubeTexture.Load(int3(isNegative.z + 4, slotIndex, 0));
}
