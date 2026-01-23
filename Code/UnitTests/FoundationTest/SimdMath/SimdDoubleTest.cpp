#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdDouble.h>


EZ_CREATE_SIMPLE_TEST(SimdMath, SimdDouble)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    ezSimdDouble vDefCtor;
    EZ_TEST_BOOL(ezMath::IsNaN((double)vDefCtor));
#else

// GCC assumes that the contents of the memory before calling the default constructor are irrelevant.
// So it optimizes away the 1,2,3,4 initializer completely.
#  if EZ_DISABLED(EZ_COMPILER_GCC)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(32) double testBlock[4] = {1, 2, 3, 4};
    ezSimdDouble* pDefCtor = ::new ((void*)&testBlock[0]) ezSimdDouble;
    EZ_TEST_BOOL_MSG((double)(*pDefCtor) == 1.0f, "Default constructed value is %lf", (double)(*pDefCtor));
#  endif
#endif

    // Make sure the class didn't accidentally change in size.
#if (EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE) && EZ_ENABLED(EZ_COMPILER_MSVC)
#  if (EZ_SSE_LEVEL >= EZ_SSE_AVX)
    static_assert(sizeof(ezSimdDouble) == 32);
    static_assert(alignof(ezSimdDouble) == 32);
#  else
    static_assert(sizeof(ezSimdDouble) == 32);
    static_assert(alignof(ezSimdDouble) == 16);
#  endif
#endif

    ezSimdDouble vInit1F(2.0f);
    EZ_TEST_BOOL(vInit1F == 2.0f);

    // Make sure all components are set to the same value
#if (EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE) && EZ_ENABLED(EZ_COMPILER_MSVC)
#  if (EZ_SSE_LEVEL >= EZ_SSE_AVX)
    EZ_TEST_BOOL(vInit1F.m_v.m256d_f64[0] == 2.0 && vInit1F.m_v.m256d_f64[1] == 2.0 && vInit1F.m_v.m256d_f64[2] == 2.0 && vInit1F.m_v.m256d_f64[3] == 2.0);
#  else
    EZ_TEST_BOOL(vInit1F.m_v.xy.m128d_f64[0] == 2.0 && vInit1F.m_v.xy.m128d_f64[1] == 2.0 && vInit1F.m_v.zw.m128d_f64[0] == 2.0 && vInit1F.m_v.zw.m128d_f64[1] == 2.0);
#  endif
#endif

    ezSimdDouble vInit1I(1);
    EZ_TEST_BOOL(vInit1I == 1.0);

    // Make sure all components are set to the same value
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE && EZ_ENABLED(EZ_COMPILER_MSVC)
#  if (EZ_SSE_LEVEL >= EZ_SSE_AVX)
    EZ_TEST_BOOL(vInit1I.m_v.m256d_f64[0] == 1.0 && vInit1I.m_v.m256d_f64[1] == 1.0 && vInit1I.m_v.m256d_f64[2] == 1.0 && vInit1I.m_v.m256d_f64[3] == 1.0);
#  else
    EZ_TEST_BOOL(vInit1I.m_v.xy.m128d_f64[0] == 1.0 && vInit1I.m_v.xy.m128d_f64[1] == 1.0 && vInit1I.m_v.zw.m128d_f64[0] == 1.0 && vInit1I.m_v.zw.m128d_f64[1] == 1.0);
#  endif
#endif

    ezSimdDouble vInit1U(4553u);
    EZ_TEST_BOOL(vInit1U == 4553.0);

    // Make sure all components are set to the same value
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE && EZ_ENABLED(EZ_COMPILER_MSVC)
#  if (EZ_SSE_LEVEL >= EZ_SSE_AVX)
    EZ_TEST_BOOL(vInit1U.m_v.m256d_f64[0] == 4553.0 && vInit1U.m_v.m256d_f64[1] == 4553.0 && vInit1U.m_v.m256d_f64[2] == 4553.0 && vInit1U.m_v.m256d_f64[3] == 4553.0);
#  else
    EZ_TEST_BOOL(vInit1U.m_v.xy.m128d_f64[0] == 4553.0 && vInit1U.m_v.xy.m128d_f64[1] == 4553.0 && vInit1U.m_v.zw.m128d_f64[0] == 4553.0 && vInit1U.m_v.zw.m128d_f64[1] == 4553.0);
#  endif
#endif

    ezSimdDouble z = ezSimdDouble::MakeZero();
    EZ_TEST_BOOL(z == 0.0);

    // Make sure all components are set to the same value
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE && EZ_ENABLED(EZ_COMPILER_MSVC)
#  if (EZ_SSE_LEVEL >= EZ_SSE_AVX)
    EZ_TEST_BOOL(z.m_v.m256d_f64[0] == 0.0 && z.m_v.m256d_f64[1] == 0.0 && z.m_v.m256d_f64[2] == 0.0 && z.m_v.m256d_f64[3] == 0.0);
#  else
    EZ_TEST_BOOL(z.m_v.xy.m128d_f64[0] == 0.0 && z.m_v.xy.m128d_f64[1] == 0.0 && z.m_v.zw.m128d_f64[0] == 0.0 && z.m_v.zw.m128d_f64[1] == 0.0);
#  endif
#endif
  }

  {
    ezSimdDouble z = ezSimdDouble::MakeNaN();

    // Make sure all components are set to the same value
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE && EZ_ENABLED(EZ_COMPILER_MSVC)
#  if (EZ_SSE_LEVEL >= EZ_SSE_AVX)
    EZ_TEST_BOOL(ezMath::IsNaN(z.m_v.m256d_f64[0]));
    EZ_TEST_BOOL(ezMath::IsNaN(z.m_v.m256d_f64[1]));
    EZ_TEST_BOOL(ezMath::IsNaN(z.m_v.m256d_f64[2]));
    EZ_TEST_BOOL(ezMath::IsNaN(z.m_v.m256d_f64[3]));
#  else
    EZ_TEST_BOOL(ezMath::IsNaN(z.m_v.xy.m128d_f64[0]));
    EZ_TEST_BOOL(ezMath::IsNaN(z.m_v.xy.m128d_f64[1]));
    EZ_TEST_BOOL(ezMath::IsNaN(z.m_v.zw.m128d_f64[0]));
    EZ_TEST_BOOL(ezMath::IsNaN(z.m_v.zw.m128d_f64[1]));
#  endif
#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operators")
  {
    ezSimdDouble a = 5.0;
    ezSimdDouble b = 2.0;

    EZ_TEST_DOUBLE(a + b, 7.0, ezMath::SmallEpsilon<double>());
    EZ_TEST_DOUBLE(a - b, 3.0, ezMath::SmallEpsilon<double>());
    EZ_TEST_DOUBLE(a * b, 10.0, ezMath::SmallEpsilon<double>());
    EZ_TEST_DOUBLE(a / b, 2.5, ezMath::SmallEpsilon<double>());

    ezSimdDouble c = 1.0;
    c += a;
    EZ_TEST_DOUBLE(c, 6.0, ezMath::SmallEpsilon<double>());

    c = 1.0;
    c -= b;
    EZ_TEST_DOUBLE(c, -1.0, ezMath::SmallEpsilon<double>());

    c = 1.0;
    c *= a;
    EZ_TEST_DOUBLE(c, 5.0, ezMath::SmallEpsilon<double>());

    c = 1.0;
    c /= a;
    EZ_TEST_DOUBLE(c, 0.2, ezMath::SmallEpsilon<double>());

    EZ_TEST_BOOL(c.IsEqual(0.201, 0.001));
    EZ_TEST_BOOL(c.IsEqual(0.199, 0.001));
    EZ_TEST_BOOL(!c.IsEqual(0.202, 0.001));
    EZ_TEST_BOOL(!c.IsEqual(0.198, 0.001));

    c = b;
    EZ_TEST_BOOL(c == b);
    EZ_TEST_BOOL(c != a);
    EZ_TEST_BOOL(a > b);
    EZ_TEST_BOOL(c >= b);
    EZ_TEST_BOOL(b < a);
    EZ_TEST_BOOL(b <= c);

    EZ_TEST_BOOL(c == 2.0);
    EZ_TEST_BOOL(c != 5.0);
    EZ_TEST_BOOL(a > 2.0);
    EZ_TEST_BOOL(c >= 2.0);
    EZ_TEST_BOOL(b < 5.0);
    EZ_TEST_BOOL(b <= 2.0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Misc")
  {
    ezSimdDouble a = 2.0;

    EZ_TEST_DOUBLE(a.GetReciprocal(), 0.5, ezMath::SmallEpsilon<double>());
    
    EZ_TEST_DOUBLE(a.GetSqrt(), 1.41421356237309504880, ezMath::SmallEpsilon<double>());
    
    EZ_TEST_DOUBLE(a.GetInvSqrt(), 0.70710678118654752440, ezMath::SmallEpsilon<double>());
    
    ezSimdDouble b = 5.0;
    EZ_TEST_BOOL(a.Max(b) == b);
    EZ_TEST_BOOL(a.Min(b) == a);

    ezSimdDouble c = -4.0;
    EZ_TEST_DOUBLE(c.Abs(), 4.0, ezMath::SmallEpsilon<double>());
  }
}
