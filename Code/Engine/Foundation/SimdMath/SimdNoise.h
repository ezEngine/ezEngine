#pragma once

#include <Foundation/SimdMath/SimdVec4i.h>

class EZ_FOUNDATION_DLL ezSimdPerlinNoise
{
public:
  ezSimdPerlinNoise(ezUInt32 uiSeed);

  ezSimdVec4f NoiseZeroToOne(const ezSimdVec4f& x, const ezSimdVec4f& y, const ezSimdVec4f& z, ezUInt32 uiNumOctaves = 1);

private:
  ezSimdVec4f Noise(const ezSimdVec4f& x, const ezSimdVec4f& y, const ezSimdVec4f& z);

  EZ_FORCE_INLINE ezSimdVec4i Permute(const ezSimdVec4i& v)
  {
    return ezSimdVec4i(m_Permutations[v.x()], m_Permutations[v.y()], m_Permutations[v.z()], m_Permutations[v.w()]);
  }

  ezUInt8 m_Permutations[512];
};
