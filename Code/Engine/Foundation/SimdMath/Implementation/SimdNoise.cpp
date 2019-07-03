#include <FoundationPCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/SimdMath/SimdNoise.h>

ezSimdPerlinNoise::ezSimdPerlinNoise(ezUInt32 uiSeed)
{
  for (ezUInt32 i = 0; i < 256; ++i)
  {
    m_Permutations[i] = i;
  }

  ezRandom rnd;
  rnd.Initialize(uiSeed);

  for (ezUInt32 i = 255; i > 0; --i)
  {
    ezUInt32 uiRandomIndex = rnd.UIntInRange(256);
    ezMath::Swap(m_Permutations[i], m_Permutations[uiRandomIndex]);
  }

  for (ezUInt32 i = 0; i < 256; ++i)
  {
    m_Permutations[i + 256] = m_Permutations[i];
  }
}

ezSimdVec4f ezSimdPerlinNoise::NoiseZeroToOne(const ezSimdVec4f& inX, const ezSimdVec4f& inY, const ezSimdVec4f& inZ, ezUInt32 uiNumOctaves /*= 1*/)
{
  ezSimdVec4f x = inX;
  ezSimdVec4f y = inY;
  ezSimdVec4f z = inZ;
  ezSimdVec4f result = ezSimdVec4f::ZeroVector();
  ezSimdFloat amplitude = 1.0f;

  for (ezUInt32 i = 0; i < uiNumOctaves; ++i)
  {
    result += Noise(x, y, z) * amplitude;
    x *= 2.0f;
    y *= 2.0f;
    z *= 2.0f;
    amplitude *= 0.5f;
  }

  return result * 0.5f + ezSimdVec4f(0.5f);
}

namespace
{
  EZ_FORCE_INLINE ezSimdVec4f Fade(const ezSimdVec4f& t)
  {
    return t.CompMul(t).CompMul(t).CompMul(t.CompMul(t * 6.0f - ezSimdVec4f(15.0f)) + ezSimdVec4f(10.0f));
  }

  EZ_FORCE_INLINE ezSimdVec4f Grad(ezSimdVec4i hash, const ezSimdVec4f& x, const ezSimdVec4f& y, const ezSimdVec4f& z)
  {
    // convert low 4 bits of hash code into 12 gradient directions.
    const ezSimdVec4i h = hash & ezSimdVec4i(15);
    const ezSimdVec4f u = ezSimdVec4f::Select(h < ezSimdVec4i(8), x, y);
    const ezSimdVec4f v = ezSimdVec4f::Select(h < ezSimdVec4i(4), y, ezSimdVec4f::Select(h == ezSimdVec4i(12) || h == ezSimdVec4i(14), x, z));
    return ezSimdVec4f::Select((h & ezSimdVec4i(1)) == ezSimdVec4i::ZeroVector(), u, -u) + ezSimdVec4f::Select((h & ezSimdVec4i(2)) == ezSimdVec4i::ZeroVector(), v, -v);
  }

  EZ_ALWAYS_INLINE ezSimdVec4f Lerp(const ezSimdVec4f& t, const ezSimdVec4f& a, const ezSimdVec4f& b)
  {
    return ezSimdVec4f::Lerp(a, b, t);
  }

} // namespace

// reference: https://mrl.nyu.edu/~perlin/noise/
ezSimdVec4f ezSimdPerlinNoise::Noise(const ezSimdVec4f& inX, const ezSimdVec4f& inY, const ezSimdVec4f& inZ)
{
  ezSimdVec4f x = inX;
  ezSimdVec4f y = inY;
  ezSimdVec4f z = inZ;

  // find unit cube that contains point.
  const ezSimdVec4f xFloored = x.Floor();
  const ezSimdVec4f yFloored = y.Floor();
  const ezSimdVec4f zFloored = z.Floor();

  const ezSimdVec4i maxIndex = ezSimdVec4i(255);
  const ezSimdVec4i X = ezSimdVec4i::Truncate(xFloored) & maxIndex;
  const ezSimdVec4i Y = ezSimdVec4i::Truncate(yFloored) & maxIndex;
  const ezSimdVec4i Z = ezSimdVec4i::Truncate(zFloored) & maxIndex;

  // find relative x,y,z of point in cube.
  x -= xFloored;
  y -= yFloored;
  z -= zFloored;

  // compute fade curves for each of x,y,z.
  const ezSimdVec4f u = Fade(x);
  const ezSimdVec4f v = Fade(y);
  const ezSimdVec4f w = Fade(z);

  // hash coordinates of the 8 cube corners
  const ezSimdVec4i i1 = ezSimdVec4i(1);
  const ezSimdVec4i A = Permute(X) + Y;
  const ezSimdVec4i AA = Permute(A) + Z;
  const ezSimdVec4i AB = Permute(A + i1) + Z;
  const ezSimdVec4i B = Permute(X + i1) + Y;
  const ezSimdVec4i BA = Permute(B) + Z;
  const ezSimdVec4i BB = Permute(B + i1) + Z;

  const ezSimdVec4f f1 = ezSimdVec4f(1.0f);

  // and add blended results from 8 corners of cube.
  const ezSimdVec4f c000 = Grad(Permute(AA), x, y, z);
  const ezSimdVec4f c100 = Grad(Permute(BA), x - f1, y, z);
  const ezSimdVec4f c010 = Grad(Permute(AB), x, y - f1, z);
  const ezSimdVec4f c110 = Grad(Permute(BB), x - f1, y - f1, z);
  const ezSimdVec4f c001 = Grad(Permute(AA + i1), x, y, z - f1);
  const ezSimdVec4f c101 = Grad(Permute(BA + i1), x - f1, y, z - f1);
  const ezSimdVec4f c011 = Grad(Permute(AB + i1), x, y - f1, z - f1);
  const ezSimdVec4f c111 = Grad(Permute(BB + i1), x - f1, y - f1, z - f1);

  const ezSimdVec4f c000_c100 = Lerp(u, c000, c100);
  const ezSimdVec4f c010_c110 = Lerp(u, c010, c110);
  const ezSimdVec4f c001_c101 = Lerp(u, c001, c101);
  const ezSimdVec4f c011_c111 = Lerp(u, c011, c111);

  return Lerp(w, Lerp(v, c000_c100, c010_c110), Lerp(v, c001_c101, c011_c111));
}
