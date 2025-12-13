#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Vec4.h>
#include <Foundation/SimdMath/SimdVec4d.h>

namespace
{
  static bool AllCompSame(const ezSimdDouble& a)
  {
    // Make sure all components are the same
    ezSimdVec4d test;
    test.m_v = a.m_v;
    return test.x() == test.y() && test.x() == test.z() && test.x() == test.w();
  }


  static void TestLength(const ezSimdVec4d& a, double r[4], const ezSimdDouble& fEps)
  {
    ezSimdDouble l1 = a.GetLength<1>();
    ezSimdDouble l2 = a.GetLength<2>();
    ezSimdDouble l3 = a.GetLength<3>();
    ezSimdDouble l4 = a.GetLength<4>();
    EZ_TEST_DOUBLE(l1, r[0], fEps);
    EZ_TEST_DOUBLE(l2, r[1], fEps);
    EZ_TEST_DOUBLE(l3, r[2], fEps);
    EZ_TEST_DOUBLE(l4, r[3], fEps);
    EZ_TEST_BOOL(AllCompSame(l1));
    EZ_TEST_BOOL(AllCompSame(l2));
    EZ_TEST_BOOL(AllCompSame(l3));
    EZ_TEST_BOOL(AllCompSame(l4));
  }


  static void TestInvLength(const ezSimdVec4d& a, double r[4], const ezSimdDouble& fEps)
  {
    ezSimdDouble l1 = a.GetInvLength<1>();
    ezSimdDouble l2 = a.GetInvLength<2>();
    ezSimdDouble l3 = a.GetInvLength<3>();
    ezSimdDouble l4 = a.GetInvLength<4>();
    EZ_TEST_DOUBLE(l1, r[0], fEps);
    EZ_TEST_DOUBLE(l2, r[1], fEps);
    EZ_TEST_DOUBLE(l3, r[2], fEps);
    EZ_TEST_DOUBLE(l4, r[3], fEps);
    EZ_TEST_BOOL(AllCompSame(l1));
    EZ_TEST_BOOL(AllCompSame(l2));
    EZ_TEST_BOOL(AllCompSame(l3));
    EZ_TEST_BOOL(AllCompSame(l4));
  }


  static void TestNormalize(const ezSimdVec4d& a, ezSimdVec4d n[4], ezSimdDouble r[4], const ezSimdDouble& fEps)
  {
    ezSimdVec4d n1 = a.GetNormalized<1>();
    ezSimdVec4d n2 = a.GetNormalized<2>();
    ezSimdVec4d n3 = a.GetNormalized<3>();
    ezSimdVec4d n4 = a.GetNormalized<4>();
    EZ_TEST_BOOL(n1.IsEqual(n[0], fEps).AllSet());
    EZ_TEST_BOOL(n2.IsEqual(n[1], fEps).AllSet());
    EZ_TEST_BOOL(n3.IsEqual(n[2], fEps).AllSet());
    EZ_TEST_BOOL(n4.IsEqual(n[3], fEps).AllSet());

    ezSimdVec4d a1 = a;
    ezSimdVec4d a2 = a;
    ezSimdVec4d a3 = a;
    ezSimdVec4d a4 = a;

    ezSimdDouble l1 = a1.GetLengthAndNormalize<1>();
    ezSimdDouble l2 = a2.GetLengthAndNormalize<2>();
    ezSimdDouble l3 = a3.GetLengthAndNormalize<3>();
    ezSimdDouble l4 = a4.GetLengthAndNormalize<4>();
    EZ_TEST_DOUBLE(l1, r[0], fEps);
    EZ_TEST_DOUBLE(l2, r[1], fEps);
    EZ_TEST_DOUBLE(l3, r[2], fEps);
    EZ_TEST_DOUBLE(l4, r[3], fEps);
    EZ_TEST_BOOL(AllCompSame(l1));
    EZ_TEST_BOOL(AllCompSame(l2));
    EZ_TEST_BOOL(AllCompSame(l3));
    EZ_TEST_BOOL(AllCompSame(l4));

    EZ_TEST_BOOL(a1.IsEqual(n[0], fEps).AllSet());
    EZ_TEST_BOOL(a2.IsEqual(n[1], fEps).AllSet());
    EZ_TEST_BOOL(a3.IsEqual(n[2], fEps).AllSet());
    EZ_TEST_BOOL(a4.IsEqual(n[3], fEps).AllSet());

    EZ_TEST_BOOL(a1.IsNormalized<1>(fEps));
    EZ_TEST_BOOL(a2.IsNormalized<2>(fEps));
    EZ_TEST_BOOL(a3.IsNormalized<3>(fEps));
    EZ_TEST_BOOL(a4.IsNormalized<4>(fEps));
    EZ_TEST_BOOL(!a1.IsNormalized<2>(fEps));
    EZ_TEST_BOOL(!a2.IsNormalized<3>(fEps));
    EZ_TEST_BOOL(!a3.IsNormalized<4>(fEps));

    a1 = a;
    a1.Normalize<1>();
    a2 = a;
    a2.Normalize<2>();
    a3 = a;
    a3.Normalize<3>();
    a4 = a;
    a4.Normalize<4>();
    EZ_TEST_BOOL(a1.IsEqual(n[0], fEps).AllSet());
    EZ_TEST_BOOL(a2.IsEqual(n[1], fEps).AllSet());
    EZ_TEST_BOOL(a3.IsEqual(n[2], fEps).AllSet());
    EZ_TEST_BOOL(a4.IsEqual(n[3], fEps).AllSet());
  }


  static void TestNormalizeIfNotZero(const ezSimdVec4d& a, ezSimdVec4d n[4], const ezSimdDouble& fEps)
  {
    ezSimdVec4d a1 = a;
    a1.NormalizeIfNotZero<1>(fEps);
    ezSimdVec4d a2 = a;
    a2.NormalizeIfNotZero<2>(fEps);
    ezSimdVec4d a3 = a;
    a3.NormalizeIfNotZero<3>(fEps);
    ezSimdVec4d a4 = a;
    a4.NormalizeIfNotZero<4>(fEps);
    EZ_TEST_BOOL(a1.IsEqual(n[0], fEps).AllSet());
    EZ_TEST_BOOL(a2.IsEqual(n[1], fEps).AllSet());
    EZ_TEST_BOOL(a3.IsEqual(n[2], fEps).AllSet());
    EZ_TEST_BOOL(a4.IsEqual(n[3], fEps).AllSet());

    EZ_TEST_BOOL(a1.IsNormalized<1>(fEps));
    EZ_TEST_BOOL(a2.IsNormalized<2>(fEps));
    EZ_TEST_BOOL(a3.IsNormalized<3>(fEps));
    EZ_TEST_BOOL(a4.IsNormalized<4>(fEps));
    EZ_TEST_BOOL(!a1.IsNormalized<2>(fEps));
    EZ_TEST_BOOL(!a2.IsNormalized<3>(fEps));
    EZ_TEST_BOOL(!a3.IsNormalized<4>(fEps));

    ezSimdVec4d b(fEps);
    b.NormalizeIfNotZero<4>(fEps);
    EZ_TEST_BOOL(b.IsZero<4>());
  }

  static void TestNormalizeIfNotZeroWithFallback(const ezSimdVec4d& a, const ezSimdDouble& fEps)
  {

    ezSimdVec4d vNorm = a;
    vNorm.Normalize<3>();


    ezSimdVec4d vNormCond = vNorm * (fEps * ezSimdDouble(0.1));
    vNormCond.NormalizeIfNotZero<3>(a, fEps);
    EZ_TEST_BOOL((vNormCond == a).AllSet());

    vNormCond = vNorm * fEps;
    vNormCond.NormalizeIfNotZero<3>(a, (fEps * ezSimdDouble(0.1)));
    EZ_TEST_BOOL(vNormCond.IsEqual(vNorm, fEps).AllSet());
  }

} // namespace

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdVec4d)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    ezSimdVec4d vDefCtor;
    EZ_TEST_BOOL(vDefCtor.IsNaN<4>());
#else
// GCC assumes that the contents of the memory prior to the placement constructor doesn't matter
// So it optimizes away the initialization.
#  if EZ_DISABLED(EZ_COMPILER_GCC)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(32) double testBlock[4] = {1, 2, 3, 4};
    ezSimdVec4d* pDefCtor = ::new ((void*)&testBlock[0]) ezSimdVec4d;
    EZ_TEST_BOOL(pDefCtor->x() == 1.0 && pDefCtor->y() == 2.0 && pDefCtor->z() == 3.0 && pDefCtor->w() == 4.0);
#  endif
#endif

    // Make sure the class didn't accidentally change in size.
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
    static_assert(sizeof(ezSimdVec4d) == 32);
    static_assert(alignof(ezSimdVec4d) == 32);
#else
    static_assert(sizeof(ezSimdVec4d) == 32);
    static_assert(alignof(ezSimdVec4d) == 16);
#endif
#endif

    ezSimdVec4d vInit1F(2.0);
    EZ_TEST_BOOL(vInit1F.x() == 2.0 && vInit1F.y() == 2.0 && vInit1F.z() == 2.0 && vInit1F.w() == 2.0);

    ezSimdDouble a(3.0);
    ezSimdVec4d vInit1SF(a);
    EZ_TEST_BOOL(vInit1SF.x() == 3.0 && vInit1SF.y() == 3.0 && vInit1SF.z() == 3.0 && vInit1SF.w() == 3.0);

    ezSimdVec4d vInit4D(1.0, 2.0, 3.0, 4.0);
    EZ_TEST_BOOL(vInit4D.x() == 1.0 && vInit4D.y() == 2.0 && vInit4D.z() == 3.0 && vInit4D.w() == 4.0);

    // Make sure all components have the correct values
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE && EZ_ENABLED(EZ_COMPILER_MSVC)
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
    EZ_TEST_BOOL(
      vInit4D.m_v.m256d_f64[0] == 1.0 && vInit4D.m_v.m256d_f64[1] == 2.0 && vInit4D.m_v.m256d_f64[2] == 3.0 && vInit4D.m_v.m256d_f64[3] == 4.0);

#else
    EZ_TEST_BOOL(
      vInit4D.m_v.xy.m128d_f64[0] == 1.0 && vInit4D.m_v.xy.m128d_f64[1] == 2.0 && vInit4D.m_v.zw.m128d_f64[0] == 3.0 && vInit4D.m_v.zw.m128d_f64[1] == 4.0);

#endif
#endif

    ezSimdVec4d vCopy(vInit4D);
    EZ_TEST_BOOL(vCopy.x() == 1.0 && vCopy.y() == 2.0 && vCopy.z() == 3.0 && vCopy.w() == 4.0);

    ezSimdVec4d vZero = ezSimdVec4d::MakeZero();
    EZ_TEST_BOOL(vZero.x() == 0.0 && vZero.y() == 0.0 && vZero.z() == 0.0 && vZero.w() == 0.0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Setter")
  {
    ezSimdVec4d a;
    a.Set(2.0);
    EZ_TEST_BOOL(a.x() == 2.0 && a.y() == 2.0 && a.z() == 2.0 && a.w() == 2.0);

    ezSimdVec4d b;
    b.Set(1.0, 2.0, 3.0, 4.0);
    EZ_TEST_BOOL(b.x() == 1.0 && b.y() == 2.0 && b.z() == 3.0 && b.w() == 4.0);

    b.SetX(5.0);
    EZ_TEST_BOOL(b.x() == 5.0 && b.y() == 2.0 && b.z() == 3.0 && b.w() == 4.0);

    b.SetY(6.0);
    EZ_TEST_BOOL(b.x() == 5.0 && b.y() == 6.0 && b.z() == 3.0 && b.w() == 4.0);

    b.SetZ(7.0);
    EZ_TEST_BOOL(b.x() == 5.0 && b.y() == 6.0 && b.z() == 7.0 && b.w() == 4.0);

    b.SetW(8.0);
    EZ_TEST_BOOL(b.x() == 5.0 && b.y() == 6.0 && b.z() == 7.0 && b.w() == 8.0);

    ezSimdVec4d c;
    c.SetZero();
    EZ_TEST_BOOL(c.x() == 0.0 && c.y() == 0.0 && c.z() == 0.0 && c.w() == 0.0);

    {
      ezSimdVec4d z = ezSimdVec4d::MakeZero();
      EZ_TEST_BOOL(z.x() == 0.0 && z.y() == 0.0 && z.z() == 0.0 && z.w() == 0.0);
    }

    {
      ezSimdVec4d z = ezSimdVec4d::MakeNaN();
      EZ_TEST_BOOL(ezMath::IsNaN((double)z.x()));
      EZ_TEST_BOOL(ezMath::IsNaN((double)z.y()));
      EZ_TEST_BOOL(ezMath::IsNaN((double)z.z()));
      EZ_TEST_BOOL(ezMath::IsNaN((double)z.w()));
    }

    {
      double testBlock[4] = {1, 2, 3, 4};
      ezSimdVec4d x;
      x.Load<1>(testBlock);
      EZ_TEST_BOOL(x.x() == 1.0 && x.y() == 0.0 && x.z() == 0.0 && x.w() == 0.0);

      ezSimdVec4d xy;
      xy.Load<2>(testBlock);
      EZ_TEST_BOOL(xy.x() == 1.0 && xy.y() == 2.0 && xy.z() == 0.0 && xy.w() == 0.0);

      ezSimdVec4d xyz;
      xyz.Load<3>(testBlock);
      EZ_TEST_BOOL(xyz.x() == 1.0 && xyz.y() == 2.0 && xyz.z() == 3.0 && xyz.w() == 0.0);

      ezSimdVec4d xyzw;
      xyzw.Load<4>(testBlock);
      EZ_TEST_BOOL(xyzw.x() == 1.0 && xyzw.y() == 2.0 && xyzw.z() == 3.0 && xyzw.w() == 4.0);

      EZ_TEST_BOOL(xyzw.GetComponent(0) == 1.0);
      EZ_TEST_BOOL(xyzw.GetComponent(1) == 2.0);
      EZ_TEST_BOOL(xyzw.GetComponent(2) == 3.0);
      EZ_TEST_BOOL(xyzw.GetComponent(3) == 4.0);
      EZ_TEST_BOOL(xyzw.GetComponent(4) == 4.0);

      // Make sure all components have the correct values
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE && EZ_ENABLED(EZ_COMPILER_MSVC)
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
      EZ_TEST_BOOL(xyzw.m_v.m256d_f64[0] == 1.0 && xyzw.m_v.m256d_f64[1] == 2.0 && xyzw.m_v.m256d_f64[2] == 3.0 && xyzw.m_v.m256d_f64[3] == 4.0);
#else
      EZ_TEST_BOOL(xyzw.m_v.xy.m128d_f64[0] == 1.0 && xyzw.m_v.xy.m128d_f64[1] == 2.0 && xyzw.m_v.zw.m128d_f64[0] == 3.0 && xyzw.m_v.zw.m128d_f64[1] == 4.0);
#endif
#endif
    }

    {
      double testBlock[4] = {7, 7, 7, 7};
      double mem[4] = {};

      ezSimdVec4d b2(1.0, 2.0, 3.0, 4.0);

      memcpy(mem, testBlock, 32);
      b2.Store<1>(mem);
      EZ_TEST_BOOL(mem[0] == 1.0 && mem[1] == 7.0 && mem[2] == 7.0 && mem[3] == 7.0);

      memcpy(mem, testBlock, 32);
      b2.Store<2>(mem);
      EZ_TEST_BOOL(mem[0] == 1.0 && mem[1] == 2.0 && mem[2] == 7.0 && mem[3] == 7.0);

      memcpy(mem, testBlock, 32);
      b2.Store<3>(mem);
      EZ_TEST_BOOL(mem[0] == 1.0 && mem[1] == 2.0 && mem[2] == 3.0 && mem[3] == 7.0);

      memcpy(mem, testBlock, 32);
      b2.Store<4>(mem);
      EZ_TEST_BOOL(mem[0] == 1.0 && mem[1] == 2.0 && mem[2] == 3.0 && mem[3] == 4.0);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Functions")
  {
    {
      ezSimdVec4d a(1.0, 2.0, 4.0, 8.0);
      ezSimdVec4d b(1.0, 0.5, 0.25, 0.125);

      EZ_TEST_BOOL(a.GetReciprocal().IsEqual(b, ezMath::SmallEpsilon<double>()).AllSet());
      EZ_TEST_BOOL(a.GetReciprocal().IsEqual(b, ezMath::SmallEpsilon<double>()).AllSet());
    }

    {
      ezSimdVec4d a(1.0, 2.0, 4.0, 8.0);
      ezSimdVec4d b(1.0, ezMath::Sqrt(2.0), ezMath::Sqrt(4.0), ezMath::Sqrt(8.0));

      EZ_TEST_BOOL(a.GetSqrt().IsEqual(b, ezMath::SmallEpsilon<double>()).AllSet());
      EZ_TEST_BOOL(a.GetSqrt().IsEqual(b, ezMath::SmallEpsilon<double>()).AllSet());
    }

    {
      ezSimdVec4d a(1.0, 2.0, 4.0, 8.0);
      ezSimdVec4d b(1.0, 1.0 / ezMath::Sqrt(2.0), 1.0 / ezMath::Sqrt(4.0), 1.0 / ezMath::Sqrt(8.0));

      EZ_TEST_BOOL(a.GetInvSqrt().IsEqual(b, ezMath::SmallEpsilon<double>()).AllSet());
      EZ_TEST_BOOL(a.GetInvSqrt().IsEqual(b, ezMath::SmallEpsilon<double>()).AllSet());

    }

    {
      ezSimdVec4d a(2.0, -2.0, 4.0, -8.0);
      double r[4];
      r[0] = 2.0;
      r[1] = ezVec2d(a.x(), a.y()).GetLength();
      r[2] = ezVec3d(a.x(), a.y(), a.z()).GetLength();
      r[3] = ezVec4d(a.x(), a.y(), a.z(), a.w()).GetLength();


      EZ_TEST_DOUBLE(a.GetLength<1>(), r[0], ezMath::SmallEpsilon<double>());
      EZ_TEST_DOUBLE(a.GetLength<2>(), r[1], ezMath::SmallEpsilon<double>());
      EZ_TEST_DOUBLE(a.GetLength<3>(), r[2], ezMath::SmallEpsilon<double>());
      EZ_TEST_DOUBLE(a.GetLength<4>(), r[3], ezMath::SmallEpsilon<double>());


      TestLength(a, r, ezMath::SmallEpsilon<double>());
    }

    {
      ezSimdVec4d a(2.0, -2.0, 4.0, -8.0);
      double r[4];
      r[0] = 0.5;
      r[1] = 1.0 / ezVec2d(a.x(), a.y()).GetLength();
      r[2] = 1.0 / ezVec3d(a.x(), a.y(), a.z()).GetLength();
      r[3] = 1.0 / ezVec4d(a.x(), a.y(), a.z(), a.w()).GetLength();

      EZ_TEST_DOUBLE(a.GetInvLength<1>(), r[0], ezMath::SmallEpsilon<double>());
      EZ_TEST_DOUBLE(a.GetInvLength<2>(), r[1], ezMath::SmallEpsilon<double>());
      EZ_TEST_DOUBLE(a.GetInvLength<3>(), r[2], ezMath::SmallEpsilon<double>());
      EZ_TEST_DOUBLE(a.GetInvLength<4>(), r[3], ezMath::SmallEpsilon<double>());

      TestInvLength(a, r, ezMath::SmallEpsilon<double>());
      // TestInvLength<ezMathAcc::BITS_23>(a, r, ezMath::DefaultEpsilon<double>());
      // TestInvLength<ezMathAcc::BITS_12>(a, r, ezMath::HugeEpsilon<double>());
    }

    {
      ezSimdVec4d a(2.0, -2.0, 4.0, -8.0);
      double r[4];
      r[0] = 2.0 * 2.0;
      r[1] = ezVec2d(a.x(), a.y()).GetLengthSquared();
      r[2] = ezVec3d(a.x(), a.y(), a.z()).GetLengthSquared();
      r[3] = ezVec4d(a.x(), a.y(), a.z(), a.w()).GetLengthSquared();

      EZ_TEST_DOUBLE(a.GetLengthSquared<1>(), r[0], ezMath::SmallEpsilon<double>());
      EZ_TEST_DOUBLE(a.GetLengthSquared<2>(), r[1], ezMath::SmallEpsilon<double>());
      EZ_TEST_DOUBLE(a.GetLengthSquared<3>(), r[2], ezMath::SmallEpsilon<double>());
      EZ_TEST_DOUBLE(a.GetLengthSquared<4>(), r[3], ezMath::SmallEpsilon<double>());
    }

    {
      ezSimdVec4d a(2.0, -2.0, 4.0, -8.0);
      ezSimdDouble r[4];
      r[0] = 2.0;
      r[1] = ezVec2d(a.x(), a.y()).GetLength();
      r[2] = ezVec3d(a.x(), a.y(), a.z()).GetLength();
      r[3] = ezVec4d(a.x(), a.y(), a.z(), a.w()).GetLength();

      ezSimdVec4d n[4];
      n[0] = a / r[0];
      n[1] = a / r[1];
      n[2] = a / r[2];
      n[3] = a / r[3];

      TestNormalize(a, n, r, ezMath::SmallEpsilon<double>());

    }

    {
      ezSimdVec4d a(2.0, -2.0, 4.0, -8.0);
      ezSimdVec4d n[4];
      n[0] = a / 2.0;
      n[1] = a / ezVec2d(a.x(), a.y()).GetLength();
      n[2] = a / ezVec3d(a.x(), a.y(), a.z()).GetLength();
      n[3] = a / ezVec4d(a.x(), a.y(), a.z(), a.w()).GetLength();

      TestNormalizeIfNotZero(a, n, ezMath::SmallEpsilon<double>());

    }

    {
      ezSimdVec4d a(2.0, -2.0, 4.0, -8.0);

      TestNormalizeIfNotZeroWithFallback(a, ezMath::SmallEpsilon<double>());
    }

    {
      ezSimdVec4d a;

      a.Set(0.0, 2.0, 0.0, 0.0);
      EZ_TEST_BOOL(a.IsZero<1>());
      EZ_TEST_BOOL(!a.IsZero<2>());

      a.Set(0.0, 0.0, 3.0, 0.0);
      EZ_TEST_BOOL(a.IsZero<2>());
      EZ_TEST_BOOL(!a.IsZero<3>());

      a.Set(0.0, 0.0, 0.0, 4.0);
      EZ_TEST_BOOL(a.IsZero<3>());
      EZ_TEST_BOOL(!a.IsZero<4>());

      double smallEps = ezMath::SmallEpsilon<double>();
      a.Set(smallEps, 2.0, smallEps, smallEps);
      EZ_TEST_BOOL(a.IsZero<1>(ezMath::DefaultEpsilon<double>()));
      EZ_TEST_BOOL(!a.IsZero<2>(ezMath::DefaultEpsilon<double>()));

      a.Set(smallEps, smallEps, 3.0, smallEps);
      EZ_TEST_BOOL(a.IsZero<2>(ezMath::DefaultEpsilon<double>()));
      EZ_TEST_BOOL(!a.IsZero<3>(ezMath::DefaultEpsilon<double>()));

      a.Set(smallEps, smallEps, smallEps, 4.0);
      EZ_TEST_BOOL(a.IsZero<3>(ezMath::DefaultEpsilon<double>()));
      EZ_TEST_BOOL(!a.IsZero<4>(ezMath::DefaultEpsilon<double>()));
    }

    {
      ezSimdVec4d a;

      double NaN = ezMath::NaN<double>();
      double Inf = ezMath::Infinity<double>();

      a.Set(NaN, 1.0, NaN, NaN);
      EZ_TEST_BOOL(a.IsNaN<1>());
      EZ_TEST_BOOL(a.IsNaN<2>());
      EZ_TEST_BOOL(!a.IsValid<2>());

      a.Set(Inf, 1.0, NaN, NaN);
      EZ_TEST_BOOL(!a.IsNaN<1>());
      EZ_TEST_BOOL(!a.IsNaN<2>());
      EZ_TEST_BOOL(!a.IsValid<2>());

      a.Set(1.0, 2.0, Inf, NaN);
      EZ_TEST_BOOL(a.IsNaN<4>());
      EZ_TEST_BOOL(!a.IsNaN<3>());
      EZ_TEST_BOOL(a.IsValid<2>());
      EZ_TEST_BOOL(!a.IsValid<3>());

      a.Set(-1.0, -2.0, -3.0, -4.0);
      EZ_TEST_BOOL(a.IsValid<1>());
      EZ_TEST_BOOL(a.IsValid<2>());
      EZ_TEST_BOOL(a.IsValid<3>());
      EZ_TEST_BOOL(a.IsValid<4>());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Swizzle")
  {
    ezSimdVec4d a(3.0, 5.0, 7.0, 9.0);

    ezSimdVec4d b = a.Get<ezSwizzle::XXXX>();
    EZ_TEST_BOOL(b.x() == 3.0 && b.y() == 3.0 && b.z() == 3.0 && b.w() == 3.0);

    b = a.Get<ezSwizzle::YYYX>();
    EZ_TEST_BOOL(b.x() == 5.0 && b.y() == 5.0 && b.z() == 5.0 && b.w() == 3.0);

    b = a.Get<ezSwizzle::ZZZX>();
    EZ_TEST_BOOL(b.x() == 7.0 && b.y() == 7.0 && b.z() == 7.0 && b.w() == 3.0);

    b = a.Get<ezSwizzle::WWWX>();
    EZ_TEST_BOOL(b.x() == 9.0 && b.y() == 9.0 && b.z() == 9.0 && b.w() == 3.0);

    b = a.Get<ezSwizzle::WZYX>();
    EZ_TEST_BOOL(b.x() == 9.0 && b.y() == 7.0 && b.z() == 5.0 && b.w() == 3.0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetCombined")
  {
    ezSimdVec4d a(2.0, 4.0, 6.0, 8.0);
    ezSimdVec4d b(3.0, 5.0, 7.0, 9.0);

    ezSimdVec4d c = a.GetCombined<ezSwizzle::XXXX>(b);
    EZ_TEST_BOOL(c.x() == a.x() && c.y() == a.x() && c.z() == b.x() && c.w() == b.x());

    c = a.GetCombined<ezSwizzle::YYYX>(b);
    EZ_TEST_BOOL(c.x() == a.y() && c.y() == a.y() && c.z() == b.y() && c.w() == b.x());

    c = a.GetCombined<ezSwizzle::ZZZX>(b);
    EZ_TEST_BOOL(c.x() == a.z() && c.y() == a.z() && c.z() == b.z() && c.w() == b.x());

    c = a.GetCombined<ezSwizzle::WWWX>(b);
    EZ_TEST_BOOL(c.x() == a.w() && c.y() == a.w() && c.z() == b.w() && c.w() == b.x());

    c = a.GetCombined<ezSwizzle::WZYX>(b);
    EZ_TEST_BOOL(c.x() == a.w() && c.y() == a.z() && c.z() == b.y() && c.w() == b.x());

    c = a.GetCombined<ezSwizzle::XYZW>(b);
    EZ_TEST_BOOL(c.x() == a.x() && c.y() == a.y() && c.z() == b.z() && c.w() == b.w());

    c = a.GetCombined<ezSwizzle::WZYX>(b);
    EZ_TEST_BOOL(c.x() == a.w() && c.y() == a.z() && c.z() == b.y() && c.w() == b.x());

    c = a.GetCombined<ezSwizzle::YYYY>(b);
    EZ_TEST_BOOL(c.x() == a.y() && c.y() == a.y() && c.z() == b.y() && c.w() == b.y());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operators")
  {
    {
      ezSimdVec4d a(-3.0, 5.0, -7.0, 9.0);

      ezSimdVec4d b = -a;
      EZ_TEST_BOOL(b.x() == 3.0 && b.y() == -5.0 && b.z() == 7.0 && b.w() == -9.0);

      b.Set(8.0, 6.0, 4.0, 2.0);
      ezSimdVec4d c;
      c = a + b;
      EZ_TEST_BOOL(c.x() == 5.0 && c.y() == 11.0 && c.z() == -3.0 && c.w() == 11.0);

      c = a - b;
      EZ_TEST_BOOL(c.x() == -11.0 && c.y() == -1.0 && c.z() == -11.0 && c.w() == 7.0);

      c = a * ezSimdDouble(3.0);
      EZ_TEST_BOOL(c.x() == -9.0 && c.y() == 15.0 && c.z() == -21.0 && c.w() == 27.0);

      c = a / ezSimdDouble(2.0);
      EZ_TEST_BOOL(c.x() == -1.5 && c.y() == 2.5 && c.z() == -3.5 && c.w() == 4.5);

      c = a.CompMul(b);
      EZ_TEST_BOOL(c.x() == -24.0 && c.y() == 30.0 && c.z() == -28.0 && c.w() == 18.0);

      ezSimdVec4d divRes(-0.375, 5.0 / 6.0, -1.75, 4.5);
      ezSimdVec4d d1 = a.CompDiv(b);


      EZ_TEST_BOOL(d1.IsEqual(divRes, ezMath::SmallEpsilon<double>()).AllSet());

    }

    {
      ezSimdVec4d a(-3.4, 5.4, -7.6, 9.6);
      ezSimdVec4d b(8.0, 6.0, 4.0, 2.0);
      ezSimdVec4d c;

      c = a.CompMin(b);
      EZ_TEST_BOOL(c.x() == -3.4 && c.y() == 5.4 && c.z() == -7.6 && c.w() == 2.0);

      c = a.CompMax(b);
      EZ_TEST_BOOL(c.x() == 8.0 && c.y() == 6.0 && c.z() == 4.0 && c.w() == 9.6);

      c = a.Abs();
      EZ_TEST_BOOL(c.x() == 3.4 && c.y() == 5.4 && c.z() == 7.6 && c.w() == 9.6);

      c = a.Round();
      EZ_TEST_BOOL(c.x() == -3.0 && c.y() == 5.0 && c.z() == -8.0 && c.w() == 10.0);

      c = a.Floor();
      EZ_TEST_BOOL(c.x() == -4.0 && c.y() == 5.0 && c.z() == -8.0 && c.w() == 9.0);

      c = a.Ceil();
      EZ_TEST_BOOL(c.x() == -3.0 && c.y() == 6.0 && c.z() == -7.0 && c.w() == 10.0);

      c = a.Trunc();
      EZ_TEST_BOOL(c.x() == -3.0 && c.y() == 5.0 && c.z() == -7.0 && c.w() == 9.0);

      c = a.Fraction();
      EZ_TEST_BOOL(c.IsEqual(ezSimdVec4d(-0.4, 0.4, -0.6, 0.6), ezMath::SmallEpsilon<double>()).AllSet());
    }

    {
      ezSimdVec4d a(-3.0, 5.0, -7.0, 9.0);
      ezSimdVec4d b(8.0, 6.0, 4.0, 2.0);

      ezSimdVec4bWide cmp(true, false, false, true);
      ezSimdVec4d c;

      c = a.FlipSign(cmp);
      EZ_TEST_BOOL(c.x() == 3.0 && c.y() == 5.0 && c.z() == -7.0 && c.w() == -9.0);

      c = ezSimdVec4d::Select(cmp, b, a);
      EZ_TEST_BOOL(c.x() == 8.0 && c.y() == 5.0 && c.z() == -7.0 && c.w() == 2.0);

      c = ezSimdVec4d::Select(cmp, a, b);
      EZ_TEST_BOOL(c.x() == -3.0 && c.y() == 6.0 && c.z() == 4.0 && c.w() == 9.0);
    }

    {
      ezSimdVec4d a(-3.0, 5.0, -7.0, 9.0);
      ezSimdVec4d b(8.0, 6.0, 4.0, 2.0);

      ezSimdVec4d c = a;
      c += b;
      EZ_TEST_BOOL(c.x() == 5.0 && c.y() == 11.0 && c.z() == -3.0 && c.w() == 11.0);

      c = a;
      c -= b;
      EZ_TEST_BOOL(c.x() == -11.0 && c.y() == -1.0 && c.z() == -11.0 && c.w() == 7.0);

      c = a;
      c *= ezSimdDouble(3.0);
      EZ_TEST_BOOL(c.x() == -9.0 && c.y() == 15.0 && c.z() == -21.0 && c.w() == 27.0);

      c = a;
      c /= ezSimdDouble(2.0);
      EZ_TEST_BOOL(c.x() == -1.5 && c.y() == 2.5 && c.z() == -3.5 && c.w() == 4.5);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Comparison")
  {
    ezSimdVec4d a(7.0, 5.0, 4.0, 3.0);
    ezSimdVec4d b(8.0, 6.0, 4.0, 2.0);
    ezSimdVec4bWide cmp;

    cmp = a == b;
    EZ_TEST_BOOL(!cmp.x() && !cmp.y() && cmp.z() && !cmp.w());

    cmp = a != b;
    EZ_TEST_BOOL(cmp.x() && cmp.y() && !cmp.z() && cmp.w());

    cmp = a <= b;
    EZ_TEST_BOOL(cmp.x() && cmp.y() && cmp.z() && !cmp.w());

    cmp = a < b;
    EZ_TEST_BOOL(cmp.x() && cmp.y() && !cmp.z() && !cmp.w());

    cmp = a >= b;
    EZ_TEST_BOOL(!cmp.x() && !cmp.y() && cmp.z() && cmp.w());

    cmp = a > b;
    EZ_TEST_BOOL(!cmp.x() && !cmp.y() && !cmp.z() && cmp.w());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Advanced Operators")
  {
    {
      ezSimdVec4d a(-3.0, 5.0, -7.0, 9.0);

      EZ_TEST_DOUBLE(a.HorizontalSum<1>(), -3.0, 0.0);
      EZ_TEST_DOUBLE(a.HorizontalSum<2>(), 2.0, 0.0);
      EZ_TEST_DOUBLE(a.HorizontalSum<3>(), -5.0, 0.0);
      EZ_TEST_DOUBLE(a.HorizontalSum<4>(), 4.0, 0.0);
      EZ_TEST_BOOL(AllCompSame(a.HorizontalSum<1>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalSum<2>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalSum<3>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalSum<4>()));

      EZ_TEST_DOUBLE(a.HorizontalMin<1>(), -3.0, 0.0);
      EZ_TEST_DOUBLE(a.HorizontalMin<2>(), -3.0, 0.0);
      EZ_TEST_DOUBLE(a.HorizontalMin<3>(), -7.0, 0.0);
      EZ_TEST_DOUBLE(a.HorizontalMin<4>(), -7.0, 0.0);
      EZ_TEST_BOOL(AllCompSame(a.HorizontalMin<1>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalMin<2>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalMin<3>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalMin<4>()));

      EZ_TEST_DOUBLE(a.HorizontalMax<1>(), -3.0, 0.0);
      EZ_TEST_DOUBLE(a.HorizontalMax<2>(), 5.0, 0.0);
      EZ_TEST_DOUBLE(a.HorizontalMax<3>(), 5.0, 0.0);
      EZ_TEST_DOUBLE(a.HorizontalMax<4>(), 9.0, 0.0);
      EZ_TEST_BOOL(AllCompSame(a.HorizontalMax<1>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalMax<2>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalMax<3>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalMax<4>()));
    }

    {
      ezSimdVec4d a(-3.0, 5.0, -7.0, 9.0);
      ezSimdVec4d b(8.0, 6.0, 4.0, 2.0);

      EZ_TEST_DOUBLE(a.Dot<1>(b), -24.0, 0.0);
      EZ_TEST_DOUBLE(a.Dot<2>(b), 6.0, 0.0);
      EZ_TEST_DOUBLE(a.Dot<3>(b), -22.0, 0.0);
      EZ_TEST_DOUBLE(a.Dot<4>(b), -4.0, 0.0);
      EZ_TEST_BOOL(AllCompSame(a.Dot<1>(b)));
      EZ_TEST_BOOL(AllCompSame(a.Dot<2>(b)));
      EZ_TEST_BOOL(AllCompSame(a.Dot<3>(b)));
      EZ_TEST_BOOL(AllCompSame(a.Dot<4>(b)));
    }

    {
      ezSimdVec4d a(1.0, 2.0, 3.0, 0.0);
      ezSimdVec4d b(2.0, -4.0, 6.0, 8.0);

      ezVec3d res = ezVec3d(a.x(), a.y(), a.z()).CrossRH(ezVec3d(b.x(), b.y(), b.z()));

      ezSimdVec4d c = a.CrossRH(b);
      EZ_TEST_BOOL(c.x() == res.x);
      EZ_TEST_BOOL(c.y() == res.y);
      EZ_TEST_BOOL(c.z() == res.z);
    }

    {
      ezSimdVec4d a(1.0, 2.0, 3.0, 0.0);
      ezSimdVec4d b(2.0, -4.0, 6.0, 0.0);

      ezVec3d res = ezVec3d(a.x(), a.y(), a.z()).CrossRH(ezVec3d(b.x(), b.y(), b.z()));

      ezSimdVec4d c = a.CrossRH(b);
      EZ_TEST_BOOL(c.x() == res.x);
      EZ_TEST_BOOL(c.y() == res.y);
      EZ_TEST_BOOL(c.z() == res.z);
    }

    {
      ezSimdVec4d a(-3.0, 5.0, -7.0, 0.0);
      ezSimdVec4d b = a.GetOrthogonalVector();

      EZ_TEST_BOOL(!b.IsZero<3>());
      EZ_TEST_DOUBLE(a.Dot<3>(b), 0.0, 0.0);

      a = ezSimdVec4d(0.0, 1.0, 0.0, 0.0);
      b = a.GetOrthogonalVector();

      EZ_TEST_BOOL(!b.IsZero<3>());
      EZ_TEST_DOUBLE(a.Dot<3>(b), 0.0, 0.0);

      a = ezSimdVec4d(0.0, 0.0, 1.0, 0.0);
      b = a.GetOrthogonalVector();

      EZ_TEST_BOOL(!b.IsZero<3>());
      EZ_TEST_DOUBLE(a.Dot<3>(b), 0.0, 0.0);
    }

    {
      ezSimdVec4d a(-3.0, 5.0, -7.0, 9.0);
      ezSimdVec4d b(8.0, 6.0, 4.0, 2.0);
      ezSimdVec4d c(1.0, 2.0, 3.0, 4.0);
      ezSimdVec4d d;

      d = ezSimdVec4d::MulAdd(a, b, c);
      EZ_TEST_BOOL(d.x() == -23.0 && d.y() == 32.0 && d.z() == -25.0 && d.w() == 22.0);

      d = ezSimdVec4d::MulAdd(a, ezSimdDouble(3.0), c);
      EZ_TEST_BOOL(d.x() == -8.0 && d.y() == 17.0 && d.z() == -18.0 && d.w() == 31.0);

      d = ezSimdVec4d::MulSub(a, b, c);
      EZ_TEST_BOOL(d.x() == -25.0 && d.y() == 28.0 && d.z() == -31.0 && d.w() == 14.0);

      d = ezSimdVec4d::MulSub(a, ezSimdDouble(3.0), c);
      EZ_TEST_BOOL(d.x() == -10.0 && d.y() == 13.0 && d.z() == -24.0 && d.w() == 23.0);

      d = ezSimdVec4d::CopySign(b, a);
      EZ_TEST_BOOL(d.x() == -8.0 && d.y() == 6.0 && d.z() == -4.0 && d.w() == 2.0);
    }
  }
}
