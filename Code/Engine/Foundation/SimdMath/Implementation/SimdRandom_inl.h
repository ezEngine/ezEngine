#pragma once

// static
EZ_FORCE_INLINE ezSimdVec4u ezSimdRandom::UInt(const ezSimdVec4i& vPosition, const ezSimdVec4u& vSeed /*= ezSimdVec4u::ZeroVector()*/)
{
  // Based on Squirrel3 which was introduced by Squirrel Eiserloh at 'Math for Game Programmers: Noise-Based RNG', GDC17.
  const ezSimdVec4u BIT_NOISE1 = ezSimdVec4u(0xb5297a4d);
  const ezSimdVec4u BIT_NOISE2 = ezSimdVec4u(0x68e31da4);
  const ezSimdVec4u BIT_NOISE3 = ezSimdVec4u(0x1b56c4e9);

  ezSimdVec4u mangled = ezSimdVec4u(vPosition);
  mangled = mangled.CompMul(BIT_NOISE1);
  mangled += vSeed;
  mangled ^= (mangled >> 8);
  mangled += BIT_NOISE2;
  mangled ^= (mangled << 8);
  mangled = mangled.CompMul(BIT_NOISE3);
  mangled ^= (mangled >> 8);

  return mangled;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdRandom::FloatZeroToOne(const ezSimdVec4i& vPosition, const ezSimdVec4u& vSeed /*= ezSimdVec4u::ZeroVector()*/)
{
  return UInt(vPosition, vSeed).ToFloat() * (1.0f / 4294967296.0f);
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdRandom::FloatMinMax(const ezSimdVec4i& vPosition, const ezSimdVec4f& vMinValue, const ezSimdVec4f& vMaxValue, const ezSimdVec4u& vSeed /*= ezSimdVec4u::ZeroVector()*/)
{
  return ezSimdVec4f::Lerp(vMinValue, vMaxValue, FloatZeroToOne(vPosition, vSeed));
}
