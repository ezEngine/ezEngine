#include <Foundation/PCH.h>
#include <Foundation/Math/Math.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  ezProjectionDepthRange::Enum ezProjectionDepthRange::Default = ezProjectionDepthRange::ZeroToOne; // Default on Windows is D3D convention
#else
  ezProjectionDepthRange::Enum ezProjectionDepthRange::Default = ezProjectionDepthRange::MinusOneToOne; // Default everywhere else is OpenGL convention
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

ezInt32 ezMath::PowerOfTwo_Floor(ezUInt32 npot)
{
  if (IsPowerOf2 (npot))
    return (npot);

  for (ezInt32 i = 1; i <= (sizeof (npot) * 8); ++i)
  {
    npot >>= 1;

    if (npot == 1)
      return (npot << i);
  }

  return (1);
}

ezInt32 ezMath::PowerOfTwo_Ceil(ezUInt32 npot)
{
  if (IsPowerOf2 (npot))
    return (npot);

  for (int i=1; i <= (sizeof (npot) * 8); ++i)
  {
    npot >>= 1;

    if (npot == 1)
      return (npot << (i + 1));
  }

  return (1);
}

ezAngle ezAngle::AngleBetween(ezAngle a, ezAngle b)
{
  // taken from http://gamedev.stackexchange.com/questions/4467/comparing-angles-and-working-out-the-difference
  return ezAngle(Pi<float>() - ezMath::Abs(ezMath::Abs(a.GetRadian() - b.GetRadian()) - Pi<float>()));
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

EZ_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Math);

