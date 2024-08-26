#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/CurveFunctions.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Reflection/Reflection.h>

// Default are D3D convention before a renderer is initialized.
ezClipSpaceDepthRange::Enum ezClipSpaceDepthRange::Default = ezClipSpaceDepthRange::ZeroToOne;
ezClipSpaceYMode::Enum ezClipSpaceYMode::RenderToTextureDefault = ezClipSpaceYMode::Regular;

ezHandedness::Enum ezHandedness::Default = ezHandedness::LeftHanded;

bool ezMath::IsPowerOf(ezInt32 value, ezInt32 iBase)
{
  if (value == 1)
    return true;

  while (value > iBase)
  {
    if (value % iBase == 0)
      value /= iBase;
    else
      return false;
  }

  return (value == iBase);
}

ezUInt32 ezMath::PowerOfTwo_Floor(ezUInt32 uiNpot)
{
  return static_cast<ezUInt32>(PowerOfTwo_Floor(static_cast<ezUInt64>(uiNpot)));
}

ezUInt64 ezMath::PowerOfTwo_Floor(ezUInt64 uiNpot)
{
  if (IsPowerOf2(uiNpot))
    return (uiNpot);

  for (ezUInt32 i = 1; i <= (sizeof(uiNpot) * 8); ++i)
  {
    uiNpot >>= 1;

    if (uiNpot == 1)
      return (uiNpot << i);
  }

  return (1);
}

ezUInt32 ezMath::PowerOfTwo_Ceil(ezUInt32 uiNpot)
{
  return static_cast<ezUInt32>(PowerOfTwo_Ceil(static_cast<ezUInt64>(uiNpot)));
}

ezUInt64 ezMath::PowerOfTwo_Ceil(ezUInt64 uiNpot)
{
  if (IsPowerOf2(uiNpot))
    return (uiNpot);

  for (ezUInt32 i = 1; i <= (sizeof(uiNpot) * 8); ++i)
  {
    uiNpot >>= 1;

    if (uiNpot == 1)
    {
      // note: left shift by 32 bits is undefined behavior and typically just returns the left operand unchanged
      // so for npot values larger than 1^31 we do run into this code path, but instead of returning 0, as one may expect, it will usually return 1
      return uiNpot << (i + 1u);
    }
  }

  return (1u);
}


ezUInt32 ezMath::GreatestCommonDivisor(ezUInt32 a, ezUInt32 b)
{
  // https://lemire.me/blog/2013/12/26/fastest-way-to-compute-the-greatest-common-divisor/
  if (a == 0)
  {
    return b;
  }
  if (b == 0)
  {
    return a;
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

ezResult ezMath::TryMultiply32(ezUInt32& out_uiResult, ezUInt32 a, ezUInt32 b, ezUInt32 c, ezUInt32 d)
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

  out_uiResult = static_cast<ezUInt32>(result & 0xFFFFFFFFllu);
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
}

ezResult ezMath::TryMultiply64(ezUInt64& out_uiResult, ezUInt64 a, ezUInt64 b, ezUInt64 c, ezUInt64 d)
{
  if (a == 0 || b == 0 || c == 0 || d == 0)
  {
    out_uiResult = 0;
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

  out_uiResult = abcd;
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
}

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
size_t ezMath::SafeConvertToSizeT(ezUInt64 uiValue)
{
  size_t result = 0;
  if (TryConvertToSizeT(result, uiValue).Succeeded())
  {
    return result;
  }

  EZ_REPORT_FAILURE("Given value ({}) can't be converted to size_t because it is too big.", uiValue);
  std::terminate();
}
#endif

void ezAngle::NormalizeRange()
{
  constexpr float fTwoPi = 2.0f * Pi<float>();
  constexpr float fTwoPiTen = 10.0f * Pi<float>();

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

float ezMath::ReplaceNaN(float fValue, float fFallback)
{
  // ATTENTION: if this is a template, inline or constexpr function, the current MSVC (17.6)
  // seems to generate incorrect code and the IsNaN check doesn't detect NaNs.
  // As an out-of-line function it works.

  if (ezMath::IsNaN(fValue))
    return fFallback;

  return fValue;
}

double ezMath::ReplaceNaN(double fValue, double fFallback)
{
  // ATTENTION: if this is a template, inline or constexpr function, the current MSVC (17.6)
  // seems to generate incorrect code and the IsNaN check doesn't detect NaNs.
  // As an out-of-line function it works.

  if (ezMath::IsNaN(fValue))
    return fFallback;

  return fValue;
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
      return ezVec3::MakeZero();
  }
}

ezMat3 ezBasisAxis::CalculateTransformationMatrix(Enum forwardDir, Enum rightDir, Enum dir, float fUniformScale /*= 1.0f*/, float fScaleX /*= 1.0f*/, float fScaleY /*= 1.0f*/, float fScaleZ /*= 1.0f*/)
{
  ezMat3 mResult;
  mResult.SetRow(0, ezBasisAxis::GetBasisVector(forwardDir) * fUniformScale * fScaleX);
  mResult.SetRow(1, ezBasisAxis::GetBasisVector(rightDir) * fUniformScale * fScaleY);
  mResult.SetRow(2, ezBasisAxis::GetBasisVector(dir) * fUniformScale * fScaleZ);

  return mResult;
}


ezQuat ezBasisAxis::GetBasisRotation_PosX(Enum axis)
{
  return ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), GetBasisVector(axis));
}

ezQuat ezBasisAxis::GetBasisRotation(Enum identity, Enum axis)
{
  return ezQuat::MakeShortestRotation(GetBasisVector(identity), GetBasisVector(axis));
}

ezBasisAxis::Enum ezBasisAxis::GetOrthogonalAxis(Enum axis1, Enum axis2, bool bFlip)
{
  const ezVec3 a1 = ezBasisAxis::GetBasisVector(axis1);
  const ezVec3 a2 = ezBasisAxis::GetBasisVector(axis2);

  ezVec3 c = a1.CrossRH(a2);

  if (bFlip)
    c = -c;

  if (c.IsEqual(ezVec3::MakeAxisX(), 0.01f))
    return ezBasisAxis::PositiveX;
  if (c.IsEqual(-ezVec3::MakeAxisX(), 0.01f))
    return ezBasisAxis::NegativeX;

  if (c.IsEqual(ezVec3::MakeAxisY(), 0.01f))
    return ezBasisAxis::PositiveY;
  if (c.IsEqual(-ezVec3::MakeAxisY(), 0.01f))
    return ezBasisAxis::NegativeY;

  if (c.IsEqual(ezVec3::MakeAxisZ(), 0.01f))
    return ezBasisAxis::PositiveZ;
  if (c.IsEqual(-ezVec3::MakeAxisZ(), 0.01f))
    return ezBasisAxis::NegativeZ;

  return axis1;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezComparisonOperator, 1)
  EZ_ENUM_CONSTANTS(ezComparisonOperator::Equal, ezComparisonOperator::NotEqual)
  EZ_ENUM_CONSTANTS(ezComparisonOperator::Less, ezComparisonOperator::LessEqual)
  EZ_ENUM_CONSTANTS(ezComparisonOperator::Greater, ezComparisonOperator::GreaterEqual)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezCurveFunction, 1)
 EZ_ENUM_CONSTANT(ezCurveFunction::Linear),
 EZ_ENUM_CONSTANT(ezCurveFunction::ConstantZero),
 EZ_ENUM_CONSTANT(ezCurveFunction::ConstantOne),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInSine),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseOutSine),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInOutSine),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInQuad),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseOutQuad),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInOutQuad),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInCubic),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseOutCubic),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInOutCubic),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInQuartic),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseOutQuartic),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInOutQuartic),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInQuintic),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseOutQuintic),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInOutQuintic),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInExpo),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseOutExpo),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInOutExpo),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInCirc),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseOutCirc),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInOutCirc),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInBack),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseOutBack),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInOutBack), 
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInElastic), 
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseOutElastic),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInOutElastic),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInBounce),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseOutBounce),
 EZ_ENUM_CONSTANT(ezCurveFunction::EaseInOutBounce),
 EZ_ENUM_CONSTANT(ezCurveFunction::Conical),
 EZ_ENUM_CONSTANT(ezCurveFunction::FadeInHoldFadeOut),
 EZ_ENUM_CONSTANT(ezCurveFunction::FadeInFadeOut),
 EZ_ENUM_CONSTANT(ezCurveFunction::Bell),
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

EZ_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Math);
