#pragma once

// static
EZ_FORCE_INLINE ezSimdVec4i ezSimdRandom::Int(const ezSimdVec4i& initialSeed)
{
  ezSimdVec4i seed = initialSeed;

  // Rand Xor Shift
  seed = seed ^ (seed << 13);
  seed = seed ^ (seed >> 17);
  seed = seed ^ (seed << 5);

  // Wang Hash
  seed = (seed ^ ezSimdVec4i(61)) ^ (seed >> 16);
  seed = seed.CompMul(ezSimdVec4i(9));
  seed = seed ^ (seed >> 4);
  seed = seed.CompMul(ezSimdVec4i(0x27d4eb2d));
  seed = seed ^ (seed >> 15);

  return seed;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdRandom::FloatZeroToOne(const ezSimdVec4i& seed)
{
  return Int(seed).ToFloat() * (1.0f / 2147483648.0f);
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdRandom::FloatMinMax(const ezSimdVec4i& seed, const ezSimdVec4f& minValue, const ezSimdVec4f& maxValue)
{
  return ezSimdVec4f::Lerp(minValue, maxValue, FloatZeroToOne(seed));
}
