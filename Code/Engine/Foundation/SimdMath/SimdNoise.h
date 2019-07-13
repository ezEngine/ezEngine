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
#if 0
    ezArrayPtr<ezUInt8> p = ezMakeArrayPtr(m_Permutations);
#else
    ezUInt8* p = m_Permutations;
#endif

    ezSimdVec4i i = v & ezSimdVec4i(EZ_ARRAY_SIZE(m_Permutations) - 1);
    return ezSimdVec4i(p[i.x()], p[i.y()], p[i.z()], p[i.w()]);
  }

  ezUInt8 m_Permutations[256];
};
