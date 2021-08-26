#pragma once

#include <Shaders/Common/Common.h>

#define AMBIENT_CUBE_NUM_DIRS 6

struct AmbientCubeFloat
{
  // value order is +X, -X, +Y, -Y, +Z, -Z
  float m_Values[AMBIENT_CUBE_NUM_DIRS];
};

struct AmbientCubeFloat3
{
  // value order is +X, -X, +Y, -Y, +Z, -Z
  float3 m_Values[AMBIENT_CUBE_NUM_DIRS];
};

void AddSampleToAmbientCube(inout AmbientCubeFloat3 ac, float3 value, float3 dir, float weight)
{
  ac.m_Values[0] += value * saturate(dir.x * weight);
  ac.m_Values[1] += value * saturate(-dir.x * weight);
  ac.m_Values[2] += value * saturate(dir.y * weight);
  ac.m_Values[3] += value * saturate(-dir.y * weight);
  ac.m_Values[4] += value * saturate(dir.z * weight);
  ac.m_Values[5] += value * saturate(-dir.z * weight);
}

void AddAmbientCube(inout AmbientCubeFloat3 a, AmbientCubeFloat3 b)
{
  [unroll]
  for (uint i = 0; i < AMBIENT_CUBE_NUM_DIRS; ++i)
  {
    a.m_Values[i] += b.m_Values[i];
  }
}

float EvaluateAmbientCube(in AmbientCubeFloat ac, float3 normal)
{
  float3 normalSquared = normal * normal;
  int3 isNegative = normal < 0.0;
  return normalSquared.x * ac.m_Values[isNegative.x + 0] +
         normalSquared.y * ac.m_Values[isNegative.y + 2] +
         normalSquared.z * ac.m_Values[isNegative.z + 4];
}

float3 EvaluateAmbientCube(in AmbientCubeFloat3 ac, float3 normal)
{
  float3 normalSquared = normal * normal;
  int3 isNegative = normal < 0.0;
  return normalSquared.x * ac.m_Values[isNegative.x + 0] +
         normalSquared.y * ac.m_Values[isNegative.y + 2] +
         normalSquared.z * ac.m_Values[isNegative.z + 4];
}

float4 EvaluateAmbientCube(Texture2D ambientCubeTexture, int slotIndex, float3 normal)
{
  float3 normalSquared = normal * normal;
  int3 isNegative = normal < 0.0;
  return normalSquared.x * ambientCubeTexture.Load(int3(isNegative.x + 0, slotIndex, 0)) +
         normalSquared.y * ambientCubeTexture.Load(int3(isNegative.y + 2, slotIndex, 0)) +
         normalSquared.z * ambientCubeTexture.Load(int3(isNegative.z + 4, slotIndex, 0));
}

float EvaluateCompressedSkyVisibility(uint compressedSkyVisibility, float3 normal)
{
  // skyvisibility is compressed as 5,5,5,5,6,6 bits with +X stored in the lowest bits
  float3 normalSquared = normal * normal * float3(1.0 / 31.0, 1.0 / 31.0, 1.0 / 63.0);
  uint3 bitShift = normal < 0.0 ? uint3(5, 15, 26) : uint3(0, 10, 20);
  return normalSquared.x * float((compressedSkyVisibility >> bitShift.x) & 0x1F) +
         normalSquared.y * float((compressedSkyVisibility >> bitShift.y) & 0x1F) +
         normalSquared.z * float((compressedSkyVisibility >> bitShift.z) & 0x3F);
}
