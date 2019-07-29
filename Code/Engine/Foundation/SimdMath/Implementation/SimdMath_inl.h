#pragma once

///\todo optimize these methods if needed

// static
EZ_FORCE_INLINE ezSimdVec4f ezSimdMath::Sin(const ezSimdVec4f& f)
{
  return ezSimdVec4f(ezMath::Sin(ezAngle::Radian(f.x())), ezMath::Sin(ezAngle::Radian(f.y())), ezMath::Sin(ezAngle::Radian(f.z())),
    ezMath::Sin(ezAngle::Radian(f.w())));
}

// static
EZ_FORCE_INLINE ezSimdVec4f ezSimdMath::Cos(const ezSimdVec4f& f)
{
  return ezSimdVec4f(ezMath::Cos(ezAngle::Radian(f.x())), ezMath::Cos(ezAngle::Radian(f.y())), ezMath::Cos(ezAngle::Radian(f.z())),
    ezMath::Cos(ezAngle::Radian(f.w())));
}

// static
EZ_FORCE_INLINE ezSimdVec4f ezSimdMath::Tan(const ezSimdVec4f& f)
{
  return ezSimdVec4f(ezMath::Tan(ezAngle::Radian(f.x())), ezMath::Tan(ezAngle::Radian(f.y())), ezMath::Tan(ezAngle::Radian(f.z())),
    ezMath::Tan(ezAngle::Radian(f.w())));
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdMath::ASin(const ezSimdVec4f& f)
{
  return ezSimdVec4f(ezMath::Pi<float>() * 0.5f) - ACos(f);
}

// 4th order polynomial approximation
// 7 * 10^-5 radians precision
// Reference : Handbook of Mathematical Functions (chapter : Elementary Transcendental Functions), M. Abramowitz and I.A. Stegun, Ed.
// static
EZ_FORCE_INLINE ezSimdVec4f ezSimdMath::ACos(const ezSimdVec4f& f)
{
  ezSimdVec4f x1 = f.Abs();
  ezSimdVec4f x2 = x1.CompMul(x1);
  ezSimdVec4f x3 = x2.CompMul(x1);

  ezSimdVec4f s = x1 * -0.2121144f + ezSimdVec4f(1.5707288f);
  s += x2 * 0.0742610f;
  s += x3 * -0.0187293f;
  s = s.CompMul((ezSimdVec4f(1.0f) - x1).GetSqrt());

  return ezSimdVec4f::Select(f >= ezSimdVec4f::ZeroVector(), s, ezSimdVec4f(ezMath::Pi<float>()) - s);
}

// Reference: https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/
// static
EZ_FORCE_INLINE ezSimdVec4f ezSimdMath::ATan(const ezSimdVec4f& f)
{
  ezSimdVec4f x = f.Abs();
  ezSimdVec4f t0 = ezSimdVec4f::Select(x < ezSimdVec4f(1.0f), x, x.GetReciprocal());
  ezSimdVec4f t1 = t0.CompMul(t0);
  ezSimdVec4f poly = ezSimdVec4f(0.0872929f);
  poly = ezSimdVec4f(-0.301895f) + poly.CompMul(t1);
  poly = ezSimdVec4f(1.0f) + poly.CompMul(t1);
  poly = poly.CompMul(t0);
  t0 = ezSimdVec4f::Select(x < ezSimdVec4f(1.0f), poly, ezSimdVec4f(ezMath::Pi<float>() * 0.5f) - poly);

  return ezSimdVec4f::Select(f < ezSimdVec4f::ZeroVector(), -t0, t0);
}
