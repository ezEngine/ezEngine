#pragma once

///\todo optimize these methods if needed

// static
EZ_FORCE_INLINE ezSimdVec4f ezSimdMath::Exp(const ezSimdVec4f& f)
{
#if EZ_ENABLED(EZ_COMPILER_MSVC) && EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
  return _mm_exp_ps(f.m_v);
#else
  return ezSimdVec4f(ezMath::Exp(f.x()), ezMath::Exp(f.y()), ezMath::Exp(f.z()), ezMath::Exp(f.w()));
#endif
}

// static
EZ_FORCE_INLINE ezSimdVec4f ezSimdMath::Ln(const ezSimdVec4f& f)
{
#if EZ_ENABLED(EZ_COMPILER_MSVC) && EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
  return _mm_log_ps(f.m_v);
#else
  return ezSimdVec4f(ezMath::Ln(f.x()), ezMath::Ln(f.y()), ezMath::Ln(f.z()), ezMath::Ln(f.w()));
#endif
}

// static
EZ_FORCE_INLINE ezSimdVec4f ezSimdMath::Log2(const ezSimdVec4f& f)
{
#if EZ_ENABLED(EZ_COMPILER_MSVC) && EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
  return _mm_log2_ps(f.m_v);
#else
  return ezSimdVec4f(ezMath::Log2(f.x()), ezMath::Log2(f.y()), ezMath::Log2(f.z()), ezMath::Log2(f.w()));
#endif
}

// static
EZ_FORCE_INLINE ezSimdVec4i ezSimdMath::Log2i(const ezSimdVec4i& i)
{
  return ezSimdVec4i(ezMath::Log2i(i.x()), ezMath::Log2i(i.y()), ezMath::Log2i(i.z()), ezMath::Log2i(i.w()));
}

// static
EZ_FORCE_INLINE ezSimdVec4f ezSimdMath::Log10(const ezSimdVec4f& f)
{
#if EZ_ENABLED(EZ_COMPILER_MSVC) && EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
  return _mm_log10_ps(f.m_v);
#else
  return ezSimdVec4f(ezMath::Log10(f.x()), ezMath::Log10(f.y()), ezMath::Log10(f.z()), ezMath::Log10(f.w()));
#endif
}

// static
EZ_FORCE_INLINE ezSimdVec4f ezSimdMath::Pow2(const ezSimdVec4f& f)
{
#if EZ_ENABLED(EZ_COMPILER_MSVC) && EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
  return _mm_exp2_ps(f.m_v);
#else
  return ezSimdVec4f(ezMath::Pow2(f.x()), ezMath::Pow2(f.y()), ezMath::Pow2(f.z()), ezMath::Pow2(f.w()));
#endif
}

// static
EZ_FORCE_INLINE ezSimdVec4f ezSimdMath::Sin(const ezSimdVec4f& f)
{
#if EZ_ENABLED(EZ_COMPILER_MSVC) && EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
  return _mm_sin_ps(f.m_v);
#else
  return ezSimdVec4f(ezMath::Sin(ezAngle::MakeFromRadian(f.x())), ezMath::Sin(ezAngle::MakeFromRadian(f.y())), ezMath::Sin(ezAngle::MakeFromRadian(f.z())),
    ezMath::Sin(ezAngle::MakeFromRadian(f.w())));
#endif
}

// static
EZ_FORCE_INLINE ezSimdVec4f ezSimdMath::Cos(const ezSimdVec4f& f)
{
#if EZ_ENABLED(EZ_COMPILER_MSVC) && EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
  return _mm_cos_ps(f.m_v);
#else
  return ezSimdVec4f(ezMath::Cos(ezAngle::MakeFromRadian(f.x())), ezMath::Cos(ezAngle::MakeFromRadian(f.y())), ezMath::Cos(ezAngle::MakeFromRadian(f.z())),
    ezMath::Cos(ezAngle::MakeFromRadian(f.w())));
#endif
}

// static
EZ_FORCE_INLINE ezSimdVec4f ezSimdMath::Tan(const ezSimdVec4f& f)
{
#if EZ_ENABLED(EZ_COMPILER_MSVC) && EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
  return _mm_tan_ps(f.m_v);
#else
  return ezSimdVec4f(ezMath::Tan(ezAngle::MakeFromRadian(f.x())), ezMath::Tan(ezAngle::MakeFromRadian(f.y())), ezMath::Tan(ezAngle::MakeFromRadian(f.z())),
    ezMath::Tan(ezAngle::MakeFromRadian(f.w())));
#endif
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

  return ezSimdVec4f::Select(f >= ezSimdVec4f::MakeZero(), s, ezSimdVec4f(ezMath::Pi<float>()) - s);
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

  return ezSimdVec4f::Select(f < ezSimdVec4f::MakeZero(), -t0, t0);
}
