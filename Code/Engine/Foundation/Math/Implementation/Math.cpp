#include <FoundationPCH.h>

#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
// Default on Windows is D3D convention
ezClipSpaceDepthRange::Enum ezClipSpaceDepthRange::Default = ezClipSpaceDepthRange::ZeroToOne;
ezClipSpaceYMode::Enum ezClipSpaceYMode::RenderToTextureDefault = ezClipSpaceYMode::Regular;
#else
// Default everywhere else is OpenGL convention
ezClipSpaceDepthRange::Enum ezClipSpaceDepthRange::Default = ezClipSpaceDepthRange::MinusOneToOne;
ezClipSpaceYMode::Enum ezClipSpaceYMode::RenderToTextureDefault = ezClipSpaceYMode::Flipped;
#endif

ezHandedness::Enum ezHandedness::Default = ezHandedness::LeftHanded;

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
    {
      // note: left shift by 32 bits is undefined behavior and typically just returns the left operand unchanged
      // so for npot values larger than 1^31 we do run into this code path, but instead of returning 0, as one may expect, it will usually return 1
      return npot << (i + 1u);
    }
  }

  return (1u);
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

ezResult ezMath::TryMultiply32(ezUInt32& out_Result, ezUInt32 a, ezUInt32 b, ezUInt32 c, ezUInt32 d)
{
  ezUInt64 result = static_cast<ezUInt64>(a) * static_cast<ezUInt64>(b);

  if (result > 0xFFFFFFFFllu)
  {
    return EZ_FAILURE;
  }

  result *= static_cast<ezUInt64>(c);

  if (result > 0xFFFFFFFFllu)
  {
    return EZ_FAILURE;
  }

  result *= static_cast<ezUInt64>(d);

  if (result > 0xFFFFFFFFllu)
  {
    return EZ_FAILURE;
  }

  out_Result = static_cast<ezUInt32>(result & 0xFFFFFFFFllu);
  return EZ_SUCCESS;
}

ezUInt32 ezMath::SafeMultiply32(ezUInt32 a, ezUInt32 b, ezUInt32 c, ezUInt32 d)
{
  ezUInt32 result = 0;
  if (TryMultiply32(result, a, b, c, d).Succeeded())
  {
    return result;
  }

  EZ_REPORT_FAILURE("Safe multiplication failed: {0} * {1} * {2} * {3} exceeds UInt32 range.", a, b, c, d);
  std::terminate();
  return 0;
}

ezResult ezMath::TryMultiply64(ezUInt64& out_Result, ezUInt64 a, ezUInt64 b, ezUInt64 c, ezUInt64 d)
{
  if (a == 0 || b == 0 || c == 0 || d == 0)
  {
    out_Result = 0;
    return EZ_SUCCESS;
  }

#if EZ_ENABLED(EZ_PLATFORM_ARCH_X86) && EZ_ENABLED(EZ_PLATFORM_64BIT) && EZ_ENABLED(EZ_COMPILER_MSVC)

  ezUInt64 uiHighBits = 0;

  const ezUInt64 ab = _umul128(a, b, &uiHighBits);
  if (uiHighBits != 0)
  {
    return EZ_FAILURE;
  }

  const ezUInt64 abc = _umul128(ab, c, &uiHighBits);
  if (uiHighBits != 0)
  {
    return EZ_FAILURE;
  }

  const ezUInt64 abcd = _umul128(abc, d, &uiHighBits);
  if (uiHighBits != 0)
  {
    return EZ_FAILURE;
  }

#else
  const ezUInt64 ab = a * b;
  const ezUInt64 abc = ab * c;
  const ezUInt64 abcd = abc * d;

  if (a > 1 && b > 1 && (ab / a != b))
  {
    return EZ_FAILURE;
  }

  if (c > 1 && (abc / c != ab))
  {
    return EZ_FAILURE;
  }

  if (d > 1 && (abcd / d != abc))
  {
    return EZ_FAILURE;
  }

#endif

  out_Result = abcd;
  return EZ_SUCCESS;
}

ezUInt64 ezMath::SafeMultiply64(ezUInt64 a, ezUInt64 b, ezUInt64 c, ezUInt64 d)
{
  ezUInt64 result = 0;
  if (TryMultiply64(result, a, b, c, d).Succeeded())
  {
    return result;
  }

  EZ_REPORT_FAILURE("Safe multiplication failed: {0} * {1} * {2} * {3} exceeds ezUInt64 range.", a, b, c, d);
  std::terminate();
  return 0;
}

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
EZ_ALWAYS_INLINE size_t ezMath::SafeConvertToSizeT(ezUInt64 uiValue)
{
  size_t result = 0;
  if (TryConvertToSizeT(result, uiValue).Succeeded())
  {
    return result;
  }

  EZ_REPORT_FAILURE("Given value ({}) can't be converted to size_t because it is too big.", uiValue);
  std::terminate();
  return 0;
}
#endif

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


ezQuat ezBasisAxis::GetBasisRotation_PosX(Enum axis)
{
  ezQuat rotAxis;
  switch (axis)
  {
    case ezBasisAxis::PositiveX:
      rotAxis.SetIdentity();
      break;
    case ezBasisAxis::PositiveY:
      rotAxis.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));
      break;
    case ezBasisAxis::PositiveZ:
      rotAxis.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(-90));
      break;
    case ezBasisAxis::NegativeX:
      rotAxis.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(180));
      break;
    case ezBasisAxis::NegativeY:
      rotAxis.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(-90));
      break;
    case ezBasisAxis::NegativeZ:
      rotAxis.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));
      break;
  }

  return rotAxis;
}

ezQuat ezBasisAxis::GetBasisRotation(Enum identity, Enum axis)
{
  ezQuat rotId;
  switch (identity)
  {
    case ezBasisAxis::PositiveX:
      rotId.SetIdentity();
      break;
    case ezBasisAxis::PositiveY:
      rotId.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(-90));
      break;
    case ezBasisAxis::PositiveZ:
      rotId.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));
      break;
    case ezBasisAxis::NegativeX:
      rotId.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(180));
      break;
    case ezBasisAxis::NegativeY:
      rotId.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));
      break;
    case ezBasisAxis::NegativeZ:
      rotId.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));
      break;
  }

  ezQuat rotAxis;
  switch (axis)
  {
    case ezBasisAxis::PositiveX:
      rotAxis.SetIdentity();
      break;
    case ezBasisAxis::PositiveY:
      rotAxis.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));
      break;
    case ezBasisAxis::PositiveZ:
      rotAxis.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(-90));
      break;
    case ezBasisAxis::NegativeX:
      rotAxis.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(180));
      break;
    case ezBasisAxis::NegativeY:
      rotAxis.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(-90));
      break;
    case ezBasisAxis::NegativeZ:
      rotAxis.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));
      break;
  }

  return rotAxis * rotId;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Math);
