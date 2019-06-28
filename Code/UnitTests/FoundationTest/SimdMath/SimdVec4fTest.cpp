#include <FoundationTestPCH.h>

#include <Foundation/Math/Vec4.h>
#include <Foundation/SimdMath/SimdVec4f.h>

namespace
{
  static bool AllCompSame(const ezSimdFloat& a)
  {
    // Make sure all components are the same
    ezSimdVec4f test;
    test.m_v = a.m_v;
    return test.x() == test.y() && test.x() == test.z() && test.x() == test.w();
  }

  template <ezMathAcc::Enum acc>
  static void TestLength(const ezSimdVec4f& a, float r[4], const ezSimdFloat& eps)
  {
    ezSimdFloat l1 = a.GetLength<1, acc>();
    ezSimdFloat l2 = a.GetLength<2, acc>();
    ezSimdFloat l3 = a.GetLength<3, acc>();
    ezSimdFloat l4 = a.GetLength<4, acc>();
    EZ_TEST_FLOAT(l1, r[0], eps);
    EZ_TEST_FLOAT(l2, r[1], eps);
    EZ_TEST_FLOAT(l3, r[2], eps);
    EZ_TEST_FLOAT(l4, r[3], eps);
    EZ_TEST_BOOL(AllCompSame(l1));
    EZ_TEST_BOOL(AllCompSame(l2));
    EZ_TEST_BOOL(AllCompSame(l3));
    EZ_TEST_BOOL(AllCompSame(l4));
  }

  template <ezMathAcc::Enum acc>
  static void TestInvLength(const ezSimdVec4f& a, float r[4], const ezSimdFloat& eps)
  {
    ezSimdFloat l1 = a.GetInvLength<1, acc>();
    ezSimdFloat l2 = a.GetInvLength<2, acc>();
    ezSimdFloat l3 = a.GetInvLength<3, acc>();
    ezSimdFloat l4 = a.GetInvLength<4, acc>();
    EZ_TEST_FLOAT(l1, r[0], eps);
    EZ_TEST_FLOAT(l2, r[1], eps);
    EZ_TEST_FLOAT(l3, r[2], eps);
    EZ_TEST_FLOAT(l4, r[3], eps);
    EZ_TEST_BOOL(AllCompSame(l1));
    EZ_TEST_BOOL(AllCompSame(l2));
    EZ_TEST_BOOL(AllCompSame(l3));
    EZ_TEST_BOOL(AllCompSame(l4));
  }

  template <ezMathAcc::Enum acc>
  static void TestNormalize(const ezSimdVec4f& a, ezSimdVec4f n[4], ezSimdFloat r[4], const ezSimdFloat& eps)
  {
    ezSimdVec4f n1 = a.GetNormalized<1, acc>();
    ezSimdVec4f n2 = a.GetNormalized<2, acc>();
    ezSimdVec4f n3 = a.GetNormalized<3, acc>();
    ezSimdVec4f n4 = a.GetNormalized<4, acc>();
    EZ_TEST_BOOL(n1.IsEqual(n[0], eps).AllSet());
    EZ_TEST_BOOL(n2.IsEqual(n[1], eps).AllSet());
    EZ_TEST_BOOL(n3.IsEqual(n[2], eps).AllSet());
    EZ_TEST_BOOL(n4.IsEqual(n[3], eps).AllSet());

    ezSimdVec4f a1 = a;
    ezSimdVec4f a2 = a;
    ezSimdVec4f a3 = a;
    ezSimdVec4f a4 = a;

    ezSimdFloat l1 = a1.GetLengthAndNormalize<1, acc>();
    ezSimdFloat l2 = a2.GetLengthAndNormalize<2, acc>();
    ezSimdFloat l3 = a3.GetLengthAndNormalize<3, acc>();
    ezSimdFloat l4 = a4.GetLengthAndNormalize<4, acc>();
    EZ_TEST_FLOAT(l1, r[0], eps);
    EZ_TEST_FLOAT(l2, r[1], eps);
    EZ_TEST_FLOAT(l3, r[2], eps);
    EZ_TEST_FLOAT(l4, r[3], eps);
    EZ_TEST_BOOL(AllCompSame(l1));
    EZ_TEST_BOOL(AllCompSame(l2));
    EZ_TEST_BOOL(AllCompSame(l3));
    EZ_TEST_BOOL(AllCompSame(l4));

    EZ_TEST_BOOL(a1.IsEqual(n[0], eps).AllSet());
    EZ_TEST_BOOL(a2.IsEqual(n[1], eps).AllSet());
    EZ_TEST_BOOL(a3.IsEqual(n[2], eps).AllSet());
    EZ_TEST_BOOL(a4.IsEqual(n[3], eps).AllSet());

    EZ_TEST_BOOL(a1.IsNormalized<1>(eps));
    EZ_TEST_BOOL(a2.IsNormalized<2>(eps));
    EZ_TEST_BOOL(a3.IsNormalized<3>(eps));
    EZ_TEST_BOOL(a4.IsNormalized<4>(eps));
    EZ_TEST_BOOL(!a1.IsNormalized<2>(eps));
    EZ_TEST_BOOL(!a2.IsNormalized<3>(eps));
    EZ_TEST_BOOL(!a3.IsNormalized<4>(eps));

    a1 = a;
    a1.Normalize<1, acc>();
    a2 = a;
    a2.Normalize<2, acc>();
    a3 = a;
    a3.Normalize<3, acc>();
    a4 = a;
    a4.Normalize<4, acc>();
    EZ_TEST_BOOL(a1.IsEqual(n[0], eps).AllSet());
    EZ_TEST_BOOL(a2.IsEqual(n[1], eps).AllSet());
    EZ_TEST_BOOL(a3.IsEqual(n[2], eps).AllSet());
    EZ_TEST_BOOL(a4.IsEqual(n[3], eps).AllSet());
  }

  template <ezMathAcc::Enum acc>
  static void TestNormalizeIfNotZero(const ezSimdVec4f& a, ezSimdVec4f n[4], const ezSimdFloat& eps)
  {
    ezSimdVec4f a1 = a;
    a1.NormalizeIfNotZero<1>(eps);
    ezSimdVec4f a2 = a;
    a2.NormalizeIfNotZero<2>(eps);
    ezSimdVec4f a3 = a;
    a3.NormalizeIfNotZero<3>(eps);
    ezSimdVec4f a4 = a;
    a4.NormalizeIfNotZero<4>(eps);
    EZ_TEST_BOOL(a1.IsEqual(n[0], eps).AllSet());
    EZ_TEST_BOOL(a2.IsEqual(n[1], eps).AllSet());
    EZ_TEST_BOOL(a3.IsEqual(n[2], eps).AllSet());
    EZ_TEST_BOOL(a4.IsEqual(n[3], eps).AllSet());

    EZ_TEST_BOOL(a1.IsNormalized<1>(eps));
    EZ_TEST_BOOL(a2.IsNormalized<2>(eps));
    EZ_TEST_BOOL(a3.IsNormalized<3>(eps));
    EZ_TEST_BOOL(a4.IsNormalized<4>(eps));
    EZ_TEST_BOOL(!a1.IsNormalized<2>(eps));
    EZ_TEST_BOOL(!a2.IsNormalized<3>(eps));
    EZ_TEST_BOOL(!a3.IsNormalized<4>(eps));

    ezSimdVec4f b(eps);
    b.NormalizeIfNotZero<4>(eps);
    EZ_TEST_BOOL(b.IsZero<4>());
  }
}

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdVec4f)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    ezSimdVec4f vDefCtor;
    EZ_TEST_BOOL(vDefCtor.IsNaN<4>());
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    float EZ_ALIGN_16(testBlock[4]) = {1, 2, 3, 4};
    ezSimdVec4f* pDefCtor = ::new ((void*)&testBlock[0]) ezSimdVec4f;
    EZ_TEST_BOOL(pDefCtor->x() == 1.0f && pDefCtor->y() == 2.0f && pDefCtor->z() == 3.0f && pDefCtor->w() == 4.0f);
#endif

    // Make sure the class didn't accidentally change in size.
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
    EZ_CHECK_AT_COMPILETIME(sizeof(ezSimdVec4f) == 16);
    EZ_CHECK_AT_COMPILETIME(EZ_ALIGNMENT_OF(ezSimdVec4f) == 16);
#endif

    ezSimdVec4f vInit1F(2.0f);
    EZ_TEST_BOOL(vInit1F.x() == 2.0f && vInit1F.y() == 2.0f && vInit1F.z() == 2.0f && vInit1F.w() == 2.0f);

    ezSimdFloat a(3.0f);
    ezSimdVec4f vInit1SF(a);
    EZ_TEST_BOOL(vInit1SF.x() == 3.0f && vInit1SF.y() == 3.0f && vInit1SF.z() == 3.0f && vInit1SF.w() == 3.0f);

    ezSimdVec4f vInit4F(1.0f, 2.0f, 3.0f, 4.0f);
    EZ_TEST_BOOL(vInit4F.x() == 1.0f && vInit4F.y() == 2.0f && vInit4F.z() == 3.0f && vInit4F.w() == 4.0f);

    // Make sure all components have the correct values
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
    EZ_TEST_BOOL(vInit4F.m_v.m128_f32[0] == 1.0f && vInit4F.m_v.m128_f32[1] == 2.0f && vInit4F.m_v.m128_f32[2] == 3.0f &&
                 vInit4F.m_v.m128_f32[3] == 4.0f);
#endif

    ezSimdVec4f vCopy(vInit4F);
    EZ_TEST_BOOL(vCopy.x() == 1.0f && vCopy.y() == 2.0f && vCopy.z() == 3.0f && vCopy.w() == 4.0f);

    ezSimdVec4f vZero = ezSimdVec4f::ZeroVector();
    EZ_TEST_BOOL(vZero.x() == 0.0f && vZero.y() == 0.0f && vZero.z() == 0.0f && vZero.w() == 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Setter")
  {
    ezSimdVec4f a;
    a.Set(2.0f);
    EZ_TEST_BOOL(a.x() == 2.0f && a.y() == 2.0f && a.z() == 2.0f && a.w() == 2.0f);

    ezSimdVec4f b;
    b.Set(1.0f, 2.0f, 3.0f, 4.0f);
    EZ_TEST_BOOL(b.x() == 1.0f && b.y() == 2.0f && b.z() == 3.0f && b.w() == 4.0f);

    b.SetX(5.0f);
    EZ_TEST_BOOL(b.x() == 5.0f && b.y() == 2.0f && b.z() == 3.0f && b.w() == 4.0f);

    b.SetY(6.0f);
    EZ_TEST_BOOL(b.x() == 5.0f && b.y() == 6.0f && b.z() == 3.0f && b.w() == 4.0f);

    b.SetZ(7.0f);
    EZ_TEST_BOOL(b.x() == 5.0f && b.y() == 6.0f && b.z() == 7.0f && b.w() == 4.0f);

    b.SetW(8.0f);
    EZ_TEST_BOOL(b.x() == 5.0f && b.y() == 6.0f && b.z() == 7.0f && b.w() == 8.0f);

    ezSimdVec4f c;
    c.SetZero();
    EZ_TEST_BOOL(c.x() == 0.0f && c.y() == 0.0f && c.z() == 0.0f && c.w() == 0.0f);

    {
      float testBlock[4] = {1, 2, 3, 4};
      ezSimdVec4f x;
      x.Load<1>(testBlock);
      EZ_TEST_BOOL(x.x() == 1.0f && x.y() == 0.0f && x.z() == 0.0f && x.w() == 0.0f);

      ezSimdVec4f xy;
      xy.Load<2>(testBlock);
      EZ_TEST_BOOL(xy.x() == 1.0f && xy.y() == 2.0f && xy.z() == 0.0f && xy.w() == 0.0f);

      ezSimdVec4f xyz;
      xyz.Load<3>(testBlock);
      EZ_TEST_BOOL(xyz.x() == 1.0f && xyz.y() == 2.0f && xyz.z() == 3.0f && xyz.w() == 0.0f);

      ezSimdVec4f xyzw;
      xyzw.Load<4>(testBlock);
      EZ_TEST_BOOL(xyzw.x() == 1.0f && xyzw.y() == 2.0f && xyzw.z() == 3.0f && xyzw.w() == 4.0f);

      EZ_TEST_BOOL(xyzw.GetComponent(0) == 1.0f);
      EZ_TEST_BOOL(xyzw.GetComponent(1) == 2.0f);
      EZ_TEST_BOOL(xyzw.GetComponent(2) == 3.0f);
      EZ_TEST_BOOL(xyzw.GetComponent(3) == 4.0f);
      EZ_TEST_BOOL(xyzw.GetComponent(4) == 4.0f);

      // Make sure all components have the correct values
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
      EZ_TEST_BOOL(xyzw.m_v.m128_f32[0] == 1.0f && xyzw.m_v.m128_f32[1] == 2.0f && xyzw.m_v.m128_f32[2] == 3.0f &&
                   xyzw.m_v.m128_f32[3] == 4.0f);
#endif
    }

    {
      float testBlock[4] = {7, 7, 7, 7};
      float mem[4] = {};

      ezSimdVec4f a(1, 2, 3, 4);

      memcpy(mem, testBlock, 16);
      a.Store<1>(mem);
      EZ_TEST_BOOL(mem[0] == 1.0f && mem[1] == 7.0f && mem[2] == 7.0f && mem[3] == 7.0f);

      memcpy(mem, testBlock, 16);
      a.Store<2>(mem);
      EZ_TEST_BOOL(mem[0] == 1.0f && mem[1] == 2.0f && mem[2] == 7.0f && mem[3] == 7.0f);

      memcpy(mem, testBlock, 16);
      a.Store<3>(mem);
      EZ_TEST_BOOL(mem[0] == 1.0f && mem[1] == 2.0f && mem[2] == 3.0f && mem[3] == 7.0f);

      memcpy(mem, testBlock, 16);
      a.Store<4>(mem);
      EZ_TEST_BOOL(mem[0] == 1.0f && mem[1] == 2.0f && mem[2] == 3.0f && mem[3] == 4.0f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Functions")
  {
    {
      ezSimdVec4f a(1.0f, 2.0f, 4.0f, 8.0f);
      ezSimdVec4f b(1.0f, 0.5f, 0.25f, 0.125f);

      EZ_TEST_BOOL(a.GetReciprocal().IsEqual(b, ezMath::BasicType<float>::SmallEpsilon()).AllSet());
      EZ_TEST_BOOL(a.GetReciprocal<ezMathAcc::FULL>().IsEqual(b, ezMath::BasicType<float>::SmallEpsilon()).AllSet());
      EZ_TEST_BOOL(a.GetReciprocal<ezMathAcc::BITS_23>().IsEqual(b, ezMath::BasicType<float>::DefaultEpsilon()).AllSet());
      EZ_TEST_BOOL(a.GetReciprocal<ezMathAcc::BITS_12>().IsEqual(b, ezMath::BasicType<float>::HugeEpsilon()).AllSet());
    }

    {
      ezSimdVec4f a(1.0f, 2.0f, 4.0f, 8.0f);
      ezSimdVec4f b(1.0f, ezMath::Sqrt(2.0f), ezMath::Sqrt(4.0f), ezMath::Sqrt(8.0f));

      EZ_TEST_BOOL(a.GetSqrt().IsEqual(b, ezMath::BasicType<float>::SmallEpsilon()).AllSet());
      EZ_TEST_BOOL(a.GetSqrt<ezMathAcc::FULL>().IsEqual(b, ezMath::BasicType<float>::SmallEpsilon()).AllSet());
      EZ_TEST_BOOL(a.GetSqrt<ezMathAcc::BITS_23>().IsEqual(b, ezMath::BasicType<float>::DefaultEpsilon()).AllSet());
      EZ_TEST_BOOL(a.GetSqrt<ezMathAcc::BITS_12>().IsEqual(b, ezMath::BasicType<float>::HugeEpsilon()).AllSet());
    }

    {
      ezSimdVec4f a(2.0f, -2.0f, 4.0f, -8.0f);
      float r[4];
      r[0] = 2.0f;
      r[1] = ezVec2(a.x(), a.y()).GetLength();
      r[2] = ezVec3(a.x(), a.y(), a.z()).GetLength();
      r[3] = ezVec4(a.x(), a.y(), a.z(), a.w()).GetLength();

      EZ_TEST_FLOAT(a.GetLength<1>(), r[0], ezMath::BasicType<float>::SmallEpsilon());
      EZ_TEST_FLOAT(a.GetLength<2>(), r[1], ezMath::BasicType<float>::SmallEpsilon());
      EZ_TEST_FLOAT(a.GetLength<3>(), r[2], ezMath::BasicType<float>::SmallEpsilon());
      EZ_TEST_FLOAT(a.GetLength<4>(), r[3], ezMath::BasicType<float>::SmallEpsilon());

      TestLength<ezMathAcc::FULL>(a, r, ezMath::BasicType<float>::SmallEpsilon());
      TestLength<ezMathAcc::BITS_23>(a, r, ezMath::BasicType<float>::DefaultEpsilon());
      TestLength<ezMathAcc::BITS_12>(a, r, 0.01f);
    }

    {
      ezSimdVec4f a(2.0f, -2.0f, 4.0f, -8.0f);
      float r[4];
      r[0] = 0.5f;
      r[1] = 1.0f / ezVec2(a.x(), a.y()).GetLength();
      r[2] = 1.0f / ezVec3(a.x(), a.y(), a.z()).GetLength();
      r[3] = 1.0f / ezVec4(a.x(), a.y(), a.z(), a.w()).GetLength();

      EZ_TEST_FLOAT(a.GetInvLength<1>(), r[0], ezMath::BasicType<float>::SmallEpsilon());
      EZ_TEST_FLOAT(a.GetInvLength<2>(), r[1], ezMath::BasicType<float>::SmallEpsilon());
      EZ_TEST_FLOAT(a.GetInvLength<3>(), r[2], ezMath::BasicType<float>::SmallEpsilon());
      EZ_TEST_FLOAT(a.GetInvLength<4>(), r[3], ezMath::BasicType<float>::SmallEpsilon());

      TestInvLength<ezMathAcc::FULL>(a, r, ezMath::BasicType<float>::SmallEpsilon());
      TestInvLength<ezMathAcc::BITS_23>(a, r, ezMath::BasicType<float>::DefaultEpsilon());
      TestInvLength<ezMathAcc::BITS_12>(a, r, ezMath::BasicType<float>::HugeEpsilon());
    }

    {
      ezSimdVec4f a(2.0f, -2.0f, 4.0f, -8.0f);
      float r[4];
      r[0] = 2.0f * 2.0f;
      r[1] = ezVec2(a.x(), a.y()).GetLengthSquared();
      r[2] = ezVec3(a.x(), a.y(), a.z()).GetLengthSquared();
      r[3] = ezVec4(a.x(), a.y(), a.z(), a.w()).GetLengthSquared();

      EZ_TEST_FLOAT(a.GetLengthSquared<1>(), r[0], ezMath::BasicType<float>::SmallEpsilon());
      EZ_TEST_FLOAT(a.GetLengthSquared<2>(), r[1], ezMath::BasicType<float>::SmallEpsilon());
      EZ_TEST_FLOAT(a.GetLengthSquared<3>(), r[2], ezMath::BasicType<float>::SmallEpsilon());
      EZ_TEST_FLOAT(a.GetLengthSquared<4>(), r[3], ezMath::BasicType<float>::SmallEpsilon());
    }

    {
      ezSimdVec4f a(2.0f, -2.0f, 4.0f, -8.0f);
      ezSimdFloat r[4];
      r[0] = 2.0f;
      r[1] = ezVec2(a.x(), a.y()).GetLength();
      r[2] = ezVec3(a.x(), a.y(), a.z()).GetLength();
      r[3] = ezVec4(a.x(), a.y(), a.z(), a.w()).GetLength();

      ezSimdVec4f n[4];
      n[0] = a / r[0];
      n[1] = a / r[1];
      n[2] = a / r[2];
      n[3] = a / r[3];

      TestNormalize<ezMathAcc::FULL>(a, n, r, ezMath::BasicType<float>::SmallEpsilon());
      TestNormalize<ezMathAcc::BITS_23>(a, n, r, ezMath::BasicType<float>::DefaultEpsilon());
      TestNormalize<ezMathAcc::BITS_12>(a, n, r, 0.01f);
    }

    {
      ezSimdVec4f a(2.0f, -2.0f, 4.0f, -8.0f);
      ezSimdVec4f n[4];
      n[0] = a / 2.0f;
      n[1] = a / ezVec2(a.x(), a.y()).GetLength();
      n[2] = a / ezVec3(a.x(), a.y(), a.z()).GetLength();
      n[3] = a / ezVec4(a.x(), a.y(), a.z(), a.w()).GetLength();

      TestNormalizeIfNotZero<ezMathAcc::FULL>(a, n, ezMath::BasicType<float>::SmallEpsilon());
      TestNormalizeIfNotZero<ezMathAcc::BITS_23>(a, n, ezMath::BasicType<float>::DefaultEpsilon());
      TestNormalizeIfNotZero<ezMathAcc::BITS_12>(a, n, ezMath::BasicType<float>::HugeEpsilon());
    }

    {
      ezSimdVec4f a;

      a.Set(0.0f, 2.0f, 0.0f, 0.0f);
      EZ_TEST_BOOL(a.IsZero<1>());
      EZ_TEST_BOOL(!a.IsZero<2>());

      a.Set(0.0f, 0.0f, 3.0f, 0.0f);
      EZ_TEST_BOOL(a.IsZero<2>());
      EZ_TEST_BOOL(!a.IsZero<3>());

      a.Set(0.0f, 0.0f, 0.0f, 4.0f);
      EZ_TEST_BOOL(a.IsZero<3>());
      EZ_TEST_BOOL(!a.IsZero<4>());

      float smallEps = ezMath::BasicType<float>::SmallEpsilon();
      a.Set(smallEps, 2.0f, smallEps, smallEps);
      EZ_TEST_BOOL(a.IsZero<1>(ezMath::BasicType<float>::DefaultEpsilon()));
      EZ_TEST_BOOL(!a.IsZero<2>(ezMath::BasicType<float>::DefaultEpsilon()));

      a.Set(smallEps, smallEps, 3.0f, smallEps);
      EZ_TEST_BOOL(a.IsZero<2>(ezMath::BasicType<float>::DefaultEpsilon()));
      EZ_TEST_BOOL(!a.IsZero<3>(ezMath::BasicType<float>::DefaultEpsilon()));

      a.Set(smallEps, smallEps, smallEps, 4.0f);
      EZ_TEST_BOOL(a.IsZero<3>(ezMath::BasicType<float>::DefaultEpsilon()));
      EZ_TEST_BOOL(!a.IsZero<4>(ezMath::BasicType<float>::DefaultEpsilon()));
    }

    {
      ezSimdVec4f a;

      float NaN = ezMath::BasicType<float>::GetNaN();
      float Inf = ezMath::BasicType<float>::GetInfinity();

      a.Set(NaN, 1.0f, NaN, NaN);
      EZ_TEST_BOOL(a.IsNaN<1>());
      EZ_TEST_BOOL(a.IsNaN<2>());
      EZ_TEST_BOOL(!a.IsValid<2>());

      a.Set(Inf, 1.0f, NaN, NaN);
      EZ_TEST_BOOL(!a.IsNaN<1>());
      EZ_TEST_BOOL(!a.IsNaN<2>());
      EZ_TEST_BOOL(!a.IsValid<2>());

      a.Set(1.0f, 2.0f, Inf, NaN);
      EZ_TEST_BOOL(a.IsNaN<4>());
      EZ_TEST_BOOL(!a.IsNaN<3>());
      EZ_TEST_BOOL(a.IsValid<2>());
      EZ_TEST_BOOL(!a.IsValid<3>());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Swizzle")
  {
    ezSimdVec4f a(3.0f, 5.0f, 7.0f, 9.0f);

    ezSimdVec4f b = a.Get<ezSwizzle::XXXX>();
    EZ_TEST_BOOL(b.x() == 3.0f && b.y() == 3.0f && b.z() == 3.0f && b.w() == 3.0f);

    b = a.Get<ezSwizzle::YYYX>();
    EZ_TEST_BOOL(b.x() == 5.0f && b.y() == 5.0f && b.z() == 5.0f && b.w() == 3.0f);

    b = a.Get<ezSwizzle::ZZZX>();
    EZ_TEST_BOOL(b.x() == 7.0f && b.y() == 7.0f && b.z() == 7.0f && b.w() == 3.0f);

    b = a.Get<ezSwizzle::WWWX>();
    EZ_TEST_BOOL(b.x() == 9.0f && b.y() == 9.0f && b.z() == 9.0f && b.w() == 3.0f);

    b = a.Get<ezSwizzle::WZYX>();
    EZ_TEST_BOOL(b.x() == 9.0f && b.y() == 7.0f && b.z() == 5.0f && b.w() == 3.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operators")
  {
    {
      ezSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);

      ezSimdVec4f b = -a;
      EZ_TEST_BOOL(b.x() == 3.0f && b.y() == -5.0f && b.z() == 7.0f && b.w() == -9.0f);

      b.Set(8.0f, 6.0f, 4.0f, 2.0f);
      ezSimdVec4f c;
      c = a + b;
      EZ_TEST_BOOL(c.x() == 5.0f && c.y() == 11.0f && c.z() == -3.0f && c.w() == 11.0f);

      c = a - b;
      EZ_TEST_BOOL(c.x() == -11.0f && c.y() == -1.0f && c.z() == -11.0f && c.w() == 7.0f);

      c = a * ezSimdFloat(3.0f);
      EZ_TEST_BOOL(c.x() == -9.0f && c.y() == 15.0f && c.z() == -21.0f && c.w() == 27.0f);

      c = a / ezSimdFloat(2.0f);
      EZ_TEST_BOOL(c.x() == -1.5f && c.y() == 2.5f && c.z() == -3.5f && c.w() == 4.5f);

      c = a.CompMul(b);
      EZ_TEST_BOOL(c.x() == -24.0f && c.y() == 30.0f && c.z() == -28.0f && c.w() == 18.0f);

      ezSimdVec4f divRes(-0.375f, 5.0f / 6.0f, -1.75f, 4.5f);
      ezSimdVec4f d1 = a.CompDiv(b);
      ezSimdVec4f d2 = a.CompDiv<ezMathAcc::FULL>(b);
      ezSimdVec4f d3 = a.CompDiv<ezMathAcc::BITS_23>(b);
      ezSimdVec4f d4 = a.CompDiv<ezMathAcc::BITS_12>(b);

      EZ_TEST_BOOL(d1.IsEqual(divRes, ezMath::BasicType<float>::SmallEpsilon()).AllSet());
      EZ_TEST_BOOL(d2.IsEqual(divRes, ezMath::BasicType<float>::SmallEpsilon()).AllSet());
      EZ_TEST_BOOL(d3.IsEqual(divRes, ezMath::BasicType<float>::DefaultEpsilon()).AllSet());
      EZ_TEST_BOOL(d4.IsEqual(divRes, 0.01f).AllSet());
    }

    {
      ezSimdVec4f a(-3.4f, 5.4f, -7.6f, 9.6f);
      ezSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);
      ezSimdVec4f c;

      c = a.CompMin(b);
      EZ_TEST_BOOL(c.x() == -3.4f && c.y() == 5.4f && c.z() == -7.6f && c.w() == 2.0f);

      c = a.CompMax(b);
      EZ_TEST_BOOL(c.x() == 8.0f && c.y() == 6.0f && c.z() == 4.0f && c.w() == 9.6f);

      c = a.Abs();
      EZ_TEST_BOOL(c.x() == 3.4f && c.y() == 5.4f && c.z() == 7.6f && c.w() == 9.6f);

      c = a.Floor();
      EZ_TEST_BOOL(c.x() == -4.0f && c.y() == 5.0f && c.z() == -8.0f && c.w() == 9.0f);

      c = a.Ceil();
      EZ_TEST_BOOL(c.x() == -3.0f && c.y() == 6.0f && c.z() == -7.0f && c.w() == 10.0f);
    }

    {
      ezSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);
      ezSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);

      ezSimdVec4b cmp(true, false, false, true);
      ezSimdVec4f c;

      c = a.FlipSign(cmp);
      EZ_TEST_BOOL(c.x() == 3.0f && c.y() == 5.0f && c.z() == -7.0f && c.w() == -9.0f);

      c = ezSimdVec4f::Select(cmp, b, a);
      EZ_TEST_BOOL(c.x() == 8.0f && c.y() == 5.0f && c.z() == -7.0f && c.w() == 2.0f);

      c = ezSimdVec4f::Select(cmp, a, b);
      EZ_TEST_BOOL(c.x() == -3.0f && c.y() == 6.0f && c.z() == 4.0f && c.w() == 9.0f);
    }

    {
      ezSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);
      ezSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);

      ezSimdVec4f c = a;
      c += b;
      EZ_TEST_BOOL(c.x() == 5.0f && c.y() == 11.0f && c.z() == -3.0f && c.w() == 11.0f);

      c = a;
      c -= b;
      EZ_TEST_BOOL(c.x() == -11.0f && c.y() == -1.0f && c.z() == -11.0f && c.w() == 7.0f);

      c = a;
      c *= ezSimdFloat(3.0f);
      EZ_TEST_BOOL(c.x() == -9.0f && c.y() == 15.0f && c.z() == -21.0f && c.w() == 27.0f);

      c = a;
      c /= ezSimdFloat(2.0f);
      EZ_TEST_BOOL(c.x() == -1.5f && c.y() == 2.5f && c.z() == -3.5f && c.w() == 4.5f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Comparison")
  {
    ezSimdVec4f a(7.0f, 5.0f, 4.0f, 3.0f);
    ezSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);
    ezSimdVec4b cmp;

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
      ezSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);

      EZ_TEST_FLOAT(a.HorizontalSum<1>(), -3.0f, 0.0f);
      EZ_TEST_FLOAT(a.HorizontalSum<2>(), 2.0f, 0.0f);
      EZ_TEST_FLOAT(a.HorizontalSum<3>(), -5.0f, 0.0f);
      EZ_TEST_FLOAT(a.HorizontalSum<4>(), 4.0f, 0.0f);
      EZ_TEST_BOOL(AllCompSame(a.HorizontalSum<1>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalSum<2>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalSum<3>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalSum<4>()));

      EZ_TEST_FLOAT(a.HorizontalMin<1>(), -3.0f, 0.0f);
      EZ_TEST_FLOAT(a.HorizontalMin<2>(), -3.0f, 0.0f);
      EZ_TEST_FLOAT(a.HorizontalMin<3>(), -7.0f, 0.0f);
      EZ_TEST_FLOAT(a.HorizontalMin<4>(), -7.0f, 0.0f);
      EZ_TEST_BOOL(AllCompSame(a.HorizontalMin<1>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalMin<2>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalMin<3>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalMin<4>()));

      EZ_TEST_FLOAT(a.HorizontalMax<1>(), -3.0f, 0.0f);
      EZ_TEST_FLOAT(a.HorizontalMax<2>(), 5.0f, 0.0f);
      EZ_TEST_FLOAT(a.HorizontalMax<3>(), 5.0f, 0.0f);
      EZ_TEST_FLOAT(a.HorizontalMax<4>(), 9.0f, 0.0f);
      EZ_TEST_BOOL(AllCompSame(a.HorizontalMax<1>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalMax<2>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalMax<3>()));
      EZ_TEST_BOOL(AllCompSame(a.HorizontalMax<4>()));
    }

    {
      ezSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);
      ezSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);

      EZ_TEST_FLOAT(a.Dot<1>(b), -24.0f, 0.0f);
      EZ_TEST_FLOAT(a.Dot<2>(b), 6.0f, 0.0f);
      EZ_TEST_FLOAT(a.Dot<3>(b), -22.0f, 0.0f);
      EZ_TEST_FLOAT(a.Dot<4>(b), -4.0f, 0.0f);
      EZ_TEST_BOOL(AllCompSame(a.Dot<1>(b)));
      EZ_TEST_BOOL(AllCompSame(a.Dot<2>(b)));
      EZ_TEST_BOOL(AllCompSame(a.Dot<3>(b)));
      EZ_TEST_BOOL(AllCompSame(a.Dot<4>(b)));
    }

    {
      ezSimdVec4f a(1.0f, 2.0f, 3.0f, 0.0f);
      ezSimdVec4f b(2.0f, -4.0f, 6.0f, 8.0f);

      ezVec3 res = ezVec3(a.x(), a.y(), a.z()).CrossRH(ezVec3(b.x(), b.y(), b.z()));

      ezSimdVec4f c = a.CrossRH(b);
      EZ_TEST_BOOL(c.x() == res.x);
      EZ_TEST_BOOL(c.y() == res.y);
      EZ_TEST_BOOL(c.z() == res.z);
    }

    {
      ezSimdVec4f a(1.0f, 2.0f, 3.0f, 0.0f);
      ezSimdVec4f b(2.0f, -4.0f, 6.0f, 0.0f);

      ezVec3 res = ezVec3(a.x(), a.y(), a.z()).CrossRH(ezVec3(b.x(), b.y(), b.z()));

      ezSimdVec4f c = a.CrossRH(b);
      EZ_TEST_BOOL(c.x() == res.x);
      EZ_TEST_BOOL(c.y() == res.y);
      EZ_TEST_BOOL(c.z() == res.z);
    }

    {
      ezSimdVec4f a(-3.0f, 5.0f, -7.0f, 0.0f);
      ezSimdVec4f b = a.GetOrthogonalVector();

      EZ_TEST_BOOL(!b.IsZero<3>());
      EZ_TEST_FLOAT(a.Dot<3>(b), 0.0f, 0.0f);
    }

    {
      ezSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);
      ezSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);
      ezSimdVec4f c(1.0f, 2.0f, 3.0f, 4.0f);
      ezSimdVec4f d;

      d = ezSimdVec4f::MulAdd(a, b, c);
      EZ_TEST_BOOL(d.x() == -23.0f && d.y() == 32.0f && d.z() == -25.0f && d.w() == 22.0f);

      d = ezSimdVec4f::MulAdd(a, ezSimdFloat(3.0f), c);
      EZ_TEST_BOOL(d.x() == -8.0f && d.y() == 17.0f && d.z() == -18.0f && d.w() == 31.0f);

      d = ezSimdVec4f::MulSub(a, b, c);
      EZ_TEST_BOOL(d.x() == -25.0f && d.y() == 28.0f && d.z() == -31.0f && d.w() == 14.0f);

      d = ezSimdVec4f::MulSub(a, ezSimdFloat(3.0f), c);
      EZ_TEST_BOOL(d.x() == -10.0f && d.y() == 13.0f && d.z() == -24.0f && d.w() == 23.0f);

      d = ezSimdVec4f::CopySign(b, a);
      EZ_TEST_BOOL(d.x() == -8.0f && d.y() == 6.0f && d.z() == -4.0f && d.w() == 2.0f);
    }
  }
}
