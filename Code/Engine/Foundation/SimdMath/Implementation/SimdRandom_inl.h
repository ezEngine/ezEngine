#pragma once

// static
EZ_FORCE_INLINE ezSimdVec4u ezSimdRandom::UInt(const ezSimdVec4u& initialSeed)
{
  ezSimdVec4u seed = initialSeed;

  // Rand Xor Shift
  seed = seed ^ (seed << 13);
  seed = seed ^ (seed >> 17);
  seed = seed ^ (seed << 5);

  // Wang Hash
  seed = (seed ^ ezSimdVec4u(61)) ^ (seed >> 16);
  seed = seed.CompMul(ezSimdVec4u(9));
  seed = seed ^ (seed >> 4);
  seed = seed.CompMul(ezSimdVec4u(0x27d4eb2d));
  seed = seed ^ (seed >> 15);

  return seed;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdRandom::FloatZeroToOne(const ezSimdVec4u& seed)
{
  return UInt(seed).ToFloat() * (1.0f / 4294967296.0f);
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdRandom::FloatMinMax(const ezSimdVec4u& seed, const ezSimdVec4f& minValue, const ezSimdVec4f& maxValue)
{
  return ezSimdVec4f::Lerp(minValue, maxValue, FloatZeroToOne(seed));
}
