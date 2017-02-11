#include <PCH.h>
#include <Foundation/SimdMath/SimdFloat.h>

EZ_CREATE_SIMPLE_TEST_GROUP(SimdMath);

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdFloat)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    ezSimdFloat vDefCtor;
    EZ_TEST_BOOL(ezMath::IsNaN((float)vDefCtor));
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    float EZ_ALIGN_16(testBlock[4]) = { 1, 2, 3, 4 };
    ezSimdFloat* pDefCtor = ::new ((void*)&testBlock[0]) ezSimdFloat;
    EZ_TEST_BOOL((float)(*pDefCtor) == 1.0f);
#endif

    // Make sure the class didn't accidentally change in size.
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
    EZ_CHECK_AT_COMPILETIME(sizeof(ezSimdFloat) == 16);
    EZ_CHECK_AT_COMPILETIME(EZ_ALIGNMENT_OF(ezSimdFloat) == 16);
#endif

    ezSimdFloat vInit1F(2.0f);
    EZ_TEST_BOOL(vInit1F == 2.0f);

    // Make sure all components are set to the same value
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
    EZ_TEST_BOOL(vInit1F.m_v.m128_f32[0] == 2.0f && vInit1F.m_v.m128_f32[1] == 2.0f &&
      vInit1F.m_v.m128_f32[2] == 2.0f && vInit1F.m_v.m128_f32[3] == 2.0f);
#endif

    ezSimdFloat vInit1I(1);
    EZ_TEST_BOOL(vInit1I == 1.0f);

    // Make sure all components are set to the same value
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
    EZ_TEST_BOOL(vInit1I.m_v.m128_f32[0] == 1.0f && vInit1I.m_v.m128_f32[1] == 1.0f &&
      vInit1I.m_v.m128_f32[2] == 1.0f && vInit1I.m_v.m128_f32[3] == 1.0f);
#endif

    ezSimdFloat vInit1U(4553u);
    EZ_TEST_BOOL(vInit1U == 4553.0f);

    // Make sure all components are set to the same value
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
    EZ_TEST_BOOL(vInit1U.m_v.m128_f32[0] == 4553.0f && vInit1U.m_v.m128_f32[1] == 4553.0f &&
      vInit1U.m_v.m128_f32[2] == 4553.0f && vInit1U.m_v.m128_f32[3] == 4553.0f);
#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operators")
  {
    ezSimdFloat a = 5.0f;
    ezSimdFloat b = 2.0f;

    EZ_TEST_FLOAT(a + b, 7.0f, ezMath::BasicType<float>::SmallEpsilon());
    EZ_TEST_FLOAT(a - b, 3.0f, ezMath::BasicType<float>::SmallEpsilon());
    EZ_TEST_FLOAT(a * b, 10.0f, ezMath::BasicType<float>::SmallEpsilon());
    EZ_TEST_FLOAT(a / b, 2.5f, ezMath::BasicType<float>::SmallEpsilon());

    ezSimdFloat c = 1.0f;
    c += a;
    EZ_TEST_FLOAT(c, 6.0f, ezMath::BasicType<float>::SmallEpsilon());

    c = 1.0f;
    c -= b;
    EZ_TEST_FLOAT(c, -1.0f, ezMath::BasicType<float>::SmallEpsilon());

    c = 1.0f;
    c *= a;
    EZ_TEST_FLOAT(c, 5.0f, ezMath::BasicType<float>::SmallEpsilon());

    c = 1.0f;
    c /= a;
    EZ_TEST_FLOAT(c, 0.2f, ezMath::BasicType<float>::SmallEpsilon());

    EZ_TEST_BOOL(c.IsEqual(0.201f, ezMath::BasicType<float>::HugeEpsilon()));
    EZ_TEST_BOOL(c.IsEqual(0.199f, ezMath::BasicType<float>::HugeEpsilon()));
    EZ_TEST_BOOL(!c.IsEqual(0.202f, ezMath::BasicType<float>::HugeEpsilon()));
    EZ_TEST_BOOL(!c.IsEqual(0.198f, ezMath::BasicType<float>::HugeEpsilon()));

    c = b;
    EZ_TEST_BOOL(c == b);
    EZ_TEST_BOOL(c != a);
    EZ_TEST_BOOL(a > b);
    EZ_TEST_BOOL(c >= b);
    EZ_TEST_BOOL(b < a);
    EZ_TEST_BOOL(b <= c);

    EZ_TEST_BOOL(c == 2.0f);
    EZ_TEST_BOOL(c != 5.0f);
    EZ_TEST_BOOL(a > 2.0f);
    EZ_TEST_BOOL(c >= 2.0f);
    EZ_TEST_BOOL(b < 5.0f);
    EZ_TEST_BOOL(b <= 2.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Misc")
  {
    ezSimdFloat a = 2.0f;

    EZ_TEST_FLOAT(a.GetReciprocal(), 0.5f, ezMath::BasicType<float>::SmallEpsilon());
    EZ_TEST_FLOAT(a.GetReciprocal<ezMathAcc::FULL>(), 0.5f, ezMath::BasicType<float>::SmallEpsilon());
    EZ_TEST_FLOAT(a.GetReciprocal<ezMathAcc::BITS_23>(), 0.5f, ezMath::BasicType<float>::DefaultEpsilon());
    EZ_TEST_FLOAT(a.GetReciprocal<ezMathAcc::BITS_12>(), 0.5f, ezMath::BasicType<float>::HugeEpsilon());

    EZ_TEST_FLOAT(a.GetSqrt(), 1.41421356f, ezMath::BasicType<float>::SmallEpsilon());
    EZ_TEST_FLOAT(a.GetSqrt<ezMathAcc::FULL>(), 1.41421356f, ezMath::BasicType<float>::SmallEpsilon());
    EZ_TEST_FLOAT(a.GetSqrt<ezMathAcc::BITS_23>(), 1.41421356f, ezMath::BasicType<float>::DefaultEpsilon());
    EZ_TEST_FLOAT(a.GetSqrt<ezMathAcc::BITS_12>(), 1.41421356f, ezMath::BasicType<float>::HugeEpsilon());

    EZ_TEST_FLOAT(a.GetInvSqrt(), 0.70710678f, ezMath::BasicType<float>::SmallEpsilon());
    EZ_TEST_FLOAT(a.GetInvSqrt<ezMathAcc::FULL>(), 0.70710678f, ezMath::BasicType<float>::SmallEpsilon());
    EZ_TEST_FLOAT(a.GetInvSqrt<ezMathAcc::BITS_23>(), 0.70710678f, ezMath::BasicType<float>::DefaultEpsilon());
    EZ_TEST_FLOAT(a.GetInvSqrt<ezMathAcc::BITS_12>(), 0.70710678f, ezMath::BasicType<float>::HugeEpsilon());

    ezSimdFloat b = 5.0f;
    EZ_TEST_BOOL(a.Max(b) == b);
    EZ_TEST_BOOL(a.Min(b) == a);

    ezSimdFloat c = -4.0f;
    EZ_TEST_FLOAT(c.Abs(), 4.0f, ezMath::BasicType<float>::SmallEpsilon());
  }
}
