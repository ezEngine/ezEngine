#include <FoundationPCH.h>

#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec3.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
ezProjectionDepthRange::Enum ezProjectionDepthRange::Default = ezProjectionDepthRange::ZeroToOne; // Default on Windows is D3D convention
#else
ezProjectionDepthRange::Enum ezProjectionDepthRange::Default =
    ezProjectionDepthRange::MinusOneToOne; // Default everywhere else is OpenGL convention
#endif

bool ezMath::IsPowerOf(ezInt32 value, ezInt32 base)
{
  if (value == 1)
    return true;

  while (value > base)
  {
    if (value % base == 0)
      value /= base;
    else
      return false;
  }

  return (value == base);
}

ezUInt32 ezMath::PowerOfTwo_Floor(ezUInt32 npot)
{
  if (IsPowerOf2(npot))
    return (npot);

  for (ezUInt32 i = 1; i <= (sizeof(npot) * 8); ++i)
  {
    npot >>= 1;

    if (npot == 1)
      return (npot << i);
  }

  return (1);
}

ezUInt32 ezMath::PowerOfTwo_Ceil(ezUInt32 npot)
{
  if (IsPowerOf2(npot))
    return (npot);

  for (ezUInt32 i = 1; i <= (sizeof(npot) * 8); ++i)
  {
    npot >>= 1;

    if (npot == 1)
      return (npot << (i + 1));
  }

  return (1U);
}


ezUInt32 ezMath::GreatestCommonDivisor(ezUInt32 a, ezUInt32 b)
{
  // https://lemire.me/blog/2013/12/26/fastest-way-to-compute-the-greatest-common-divisor/
  if (a == 0)
  {
    return a;
  }
  if (b == 0)
  {
    return b;
  }

  ezUInt32 shift = FirstBitLow(a | b);
  a >>= FirstBitLow(a);
  do
  {
    b >>= FirstBitLow(b);
    if (a > b)
    {
      Swap(a, b);
    }
    b = b - a;
  } while (b != 0);
  return a << shift;
}

void ezAngle::NormalizeRange()
{
  const float fTwoPi = 2.0f * Pi<float>();

  const float fTwoPiTen = 10.0f * Pi<float>();

  if (m_fRadian > fTwoPiTen || m_fRadian < -fTwoPiTen)
  {
    m_fRadian = ezMath::Mod(m_fRadian, fTwoPi);
  }

  while (m_fRadian >= fTwoPi)
  {
    m_fRadian -= fTwoPi;
  }

  while (m_fRadian < 0.0f)
  {
    m_fRadian += fTwoPi;
  }
}

ezVec3 ezBasisAxis::GetBasisVector(Enum basisAxis)
{
  switch (basisAxis)
  {
    case ezBasisAxis::PositiveX:
      return ezVec3(1.0f, 0.0f, 0.0f);

    case ezBasisAxis::NegativeX:
      return ezVec3(-1.0f, 0.0f, 0.0f);

    case ezBasisAxis::PositiveY:
      return ezVec3(0.0f, 1.0f, 0.0f);

    case ezBasisAxis::NegativeY:
      return ezVec3(0.0f, -1.0f, 0.0f);

    case ezBasisAxis::PositiveZ:
      return ezVec3(0.0f, 0.0f, 1.0f);

    case ezBasisAxis::NegativeZ:
      return ezVec3(0.0f, 0.0f, -1.0f);

    default:
      EZ_REPORT_FAILURE("Invalid basis dir {0}", basisAxis);
      return ezVec3::ZeroVector();
  }
}

ezMat3 ezBasisAxis::CalculateTransformationMatrix(Enum forwardDir, Enum rightDir, Enum upDir, float fUniformScale /*= 1.0f*/,
                                                  float fScaleX /*= 1.0f*/, float fScaleY /*= 1.0f*/, float fScaleZ /*= 1.0f*/)
{
  ezMat3 mResult;
  mResult.SetRow(0, ezBasisAxis::GetBasisVector(forwardDir) * fUniformScale * fScaleX);
  mResult.SetRow(1, ezBasisAxis::GetBasisVector(rightDir) * fUniformScale * fScaleY);
  mResult.SetRow(2, ezBasisAxis::GetBasisVector(upDir) * fUniformScale * fScaleZ);

  return mResult;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Math);

