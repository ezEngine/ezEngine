#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdVec4u.h>

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdVec4i)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
    // In debug the default constructor initializes everything with 0xCDCDCDCD.
    ezSimdVec4i vDefCtor;
    EZ_TEST_BOOL(vDefCtor.x() == 0xCDCDCDCD && vDefCtor.y() == 0xCDCDCDCD && vDefCtor.z() == 0xCDCDCDCD && vDefCtor.w() == 0xCDCDCDCD);
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) float testBlock[4] = {1, 2, 3, 4};
    ezSimdVec4i* pDefCtor = ::new ((void*)&testBlock[0]) ezSimdVec4i;
    EZ_TEST_BOOL(testBlock[0] == 1 && testBlock[1] == 2 && testBlock[2] == 3 && testBlock[3] == 4);
#endif

    // Make sure the class didn't accidentally change in size.
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
    static_assert(sizeof(ezSimdVec4i) == 16);
    static_assert(EZ_ALIGNMENT_OF(ezSimdVec4i) == 16);
#endif

    ezSimdVec4i a(2);
    EZ_TEST_BOOL(a.x() == 2 && a.y() == 2 && a.z() == 2 && a.w() == 2);

    ezSimdVec4i b(1, 2, 3, 4);
    EZ_TEST_BOOL(b.x() == 1 && b.y() == 2 && b.z() == 3 && b.w() == 4);

    // Make sure all components have the correct values
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE && EZ_ENABLED(EZ_COMPILER_MSVC)
    EZ_TEST_BOOL(b.m_v.m128i_i32[0] == 1 && b.m_v.m128i_i32[1] == 2 && b.m_v.m128i_i32[2] == 3 && b.m_v.m128i_i32[3] == 4);
#endif

    ezSimdVec4i copy(b);
    EZ_TEST_BOOL(copy.x() == 1 && copy.y() == 2 && copy.z() == 3 && copy.w() == 4);

    EZ_TEST_BOOL(copy.GetComponent<0>() == 1 && copy.GetComponent<1>() == 2 && copy.GetComponent<2>() == 3 && copy.GetComponent<3>() == 4);

    ezSimdVec4i vZero = ezSimdVec4i::MakeZero();
    EZ_TEST_BOOL(vZero.x() == 0 && vZero.y() == 0 && vZero.z() == 0 && vZero.w() == 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Setter")
  {
    ezSimdVec4i a;
    a.Set(2);
    EZ_TEST_BOOL(a.x() == 2 && a.y() == 2 && a.z() == 2 && a.w() == 2);

    ezSimdVec4i b;
    b.Set(1, 2, 3, 4);
    EZ_TEST_BOOL(b.x() == 1 && b.y() == 2 && b.z() == 3 && b.w() == 4);

    ezSimdVec4i vSetZero;
    vSetZero.SetZero();
    EZ_TEST_BOOL(vSetZero.x() == 0 && vSetZero.y() == 0 && vSetZero.z() == 0 && vSetZero.w() == 0);

    {
      ezSimdVec4i z = ezSimdVec4i::MakeZero();
      EZ_TEST_BOOL(z.x() == 0 && z.y() == 0 && z.z() == 0 && z.w() == 0);
    }

    {
      int testBlock[4] = {1, 2, 3, 4};
      ezSimdVec4i x;
      x.Load<1>(testBlock);
      EZ_TEST_BOOL(x.x() == 1 && x.y() == 0 && x.z() == 0 && x.w() == 0);

      ezSimdVec4i xy;
      xy.Load<2>(testBlock);
      EZ_TEST_BOOL(xy.x() == 1 && xy.y() == 2 && xy.z() == 0 && xy.w() == 0);

      ezSimdVec4i xyz;
      xyz.Load<3>(testBlock);
      EZ_TEST_BOOL(xyz.x() == 1 && xyz.y() == 2 && xyz.z() == 3 && xyz.w() == 0);

      ezSimdVec4i xyzw;
      xyzw.Load<4>(testBlock);
      EZ_TEST_BOOL(xyzw.x() == 1 && xyzw.y() == 2 && xyzw.z() == 3 && xyzw.w() == 4);

      EZ_TEST_INT(xyzw.GetComponent<0>(), 1);
      EZ_TEST_INT(xyzw.GetComponent<1>(), 2);
      EZ_TEST_INT(xyzw.GetComponent<2>(), 3);
      EZ_TEST_INT(xyzw.GetComponent<3>(), 4);

      // Make sure all components have the correct values
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE && EZ_ENABLED(EZ_COMPILER_MSVC)
      EZ_TEST_BOOL(xyzw.m_v.m128i_i32[0] == 1 && xyzw.m_v.m128i_i32[1] == 2 && xyzw.m_v.m128i_i32[2] == 3 && xyzw.m_v.m128i_i32[3] == 4);
#endif
    }

    {
      int testBlock[4] = {7, 7, 7, 7};
      int mem[4] = {};

      ezSimdVec4i b2(1, 2, 3, 4);

      memcpy(mem, testBlock, 16);
      b2.Store<1>(mem);
      EZ_TEST_BOOL(mem[0] == 1 && mem[1] == 7 && mem[2] == 7 && mem[3] == 7);

      memcpy(mem, testBlock, 16);
      b2.Store<2>(mem);
      EZ_TEST_BOOL(mem[0] == 1 && mem[1] == 2 && mem[2] == 7 && mem[3] == 7);

      memcpy(mem, testBlock, 16);
      b2.Store<3>(mem);
      EZ_TEST_BOOL(mem[0] == 1 && mem[1] == 2 && mem[2] == 3 && mem[3] == 7);

      memcpy(mem, testBlock, 16);
      b2.Store<4>(mem);
      EZ_TEST_BOOL(mem[0] == 1 && mem[1] == 2 && mem[2] == 3 && mem[3] == 4);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion")
  {
    ezSimdVec4i ia(-3, 5, -7, 11);

    ezSimdVec4u ua(ia);
    EZ_TEST_BOOL(ua.x() == -3 && ua.y() == 5 && ua.z() == -7 && ua.w() == 11);

    ezSimdVec4f fa = ia.ToFloat();
    EZ_TEST_BOOL(fa.x() == -3.0f && fa.y() == 5.0f && fa.z() == -7.0f && fa.w() == 11.0f);

    fa = ezSimdVec4f(-2.3f, 5.7f, -2147483520.0f, 2147483520.0f);
    ezSimdVec4i b = ezSimdVec4i::Truncate(fa);
    EZ_TEST_INT(b.x(), -2);
    EZ_TEST_INT(b.y(), 5);
    EZ_TEST_INT(b.z(), -2147483520);
    EZ_TEST_INT(b.w(), 2147483520);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Swizzle")
  {
    ezSimdVec4i a(3, 5, 7, 9);

    ezSimdVec4i b = a.Get<ezSwizzle::XXXX>();
    EZ_TEST_BOOL(b.x() == 3 && b.y() == 3 && b.z() == 3 && b.w() == 3);

    b = a.Get<ezSwizzle::YYYX>();
    EZ_TEST_BOOL(b.x() == 5 && b.y() == 5 && b.z() == 5 && b.w() == 3);

    b = a.Get<ezSwizzle::ZZZX>();
    EZ_TEST_BOOL(b.x() == 7 && b.y() == 7 && b.z() == 7 && b.w() == 3);

    b = a.Get<ezSwizzle::WWWX>();
    EZ_TEST_BOOL(b.x() == 9 && b.y() == 9 && b.z() == 9 && b.w() == 3);

    b = a.Get<ezSwizzle::WZYX>();
    EZ_TEST_BOOL(b.x() == 9 && b.y() == 7 && b.z() == 5 && b.w() == 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetCombined")
  {
    ezSimdVec4i a(2, 4, 6, 8);
    ezSimdVec4i b(3, 5, 7, 9);

    ezSimdVec4i c = a.GetCombined<ezSwizzle::XXXX>(b);
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
      ezSimdVec4i a(-3, 5, -7, 9);

      ezSimdVec4i b = -a;
      EZ_TEST_BOOL(b.x() == 3 && b.y() == -5 && b.z() == 7 && b.w() == -9);

      b.Set(8, 6, 4, 2);
      ezSimdVec4i c;
      c = a + b;
      EZ_TEST_BOOL(c.x() == 5 && c.y() == 11 && c.z() == -3 && c.w() == 11);

      c = a - b;
      EZ_TEST_BOOL(c.x() == -11 && c.y() == -1 && c.z() == -11 && c.w() == 7);

      c = a.CompMul(b);
      EZ_TEST_BOOL(c.x() == -24 && c.y() == 30 && c.z() == -28 && c.w() == 18);

      c = a.CompDiv(b);
      EZ_TEST_BOOL(c.x() == 0 && c.y() == 0 && c.z() == -1 && c.w() == 4);
    }

    {
      ezSimdVec4i a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
      ezSimdVec4i b(EZ_BIT(4), EZ_BIT(3), EZ_BIT(3), EZ_BIT(5) - 1);
      ezSimdVec4i c;

      c = a | b;
      EZ_TEST_BOOL(c.x() == (EZ_BIT(1) | EZ_BIT(4)) && c.y() == (EZ_BIT(2) | EZ_BIT(3)) && c.z() == EZ_BIT(3) && c.w() == EZ_BIT(5) - 1);

      c = a & b;
      EZ_TEST_BOOL(c.x() == 0 && c.y() == 0 && c.z() == EZ_BIT(3) && c.w() == EZ_BIT(4));

      c = a ^ b;
      EZ_TEST_BOOL(c.x() == (EZ_BIT(1) | EZ_BIT(4)) && c.y() == (EZ_BIT(2) | EZ_BIT(3)) && c.z() == 0 && c.w() == EZ_BIT(4) - 1);

      c = ~a;
      EZ_TEST_BOOL(c.x() == ~EZ_BIT(1) && c.y() == ~EZ_BIT(2) && c.z() == ~EZ_BIT(3) && c.w() == ~EZ_BIT(4));

      c = a << 3;
      EZ_TEST_BOOL(c.x() == EZ_BIT(4) && c.y() == EZ_BIT(5) && c.z() == EZ_BIT(6) && c.w() == EZ_BIT(7));

      c = a >> 1;
      EZ_TEST_BOOL(c.x() == EZ_BIT(0) && c.y() == EZ_BIT(1) && c.z() == EZ_BIT(2) && c.w() == EZ_BIT(3));

      ezSimdVec4i s(1, 2, 3, 4);
      c = a << s;
      EZ_TEST_BOOL(c.x() == EZ_BIT(2) && c.y() == EZ_BIT(4) && c.z() == EZ_BIT(6) && c.w() == EZ_BIT(8));

      c = b >> s;
      EZ_TEST_BOOL(c.x() == EZ_BIT(3) && c.y() == EZ_BIT(1) && c.z() == EZ_BIT(0) && c.w() == EZ_BIT(0));
    }

    {
      ezSimdVec4i a(-3, 5, -7, 9);
      ezSimdVec4i b(8, 6, 4, 2);

      ezSimdVec4i c = a;
      c += b;
      EZ_TEST_BOOL(c.x() == 5 && c.y() == 11 && c.z() == -3 && c.w() == 11);

      c = a;
      c -= b;
      EZ_TEST_BOOL(c.x() == -11 && c.y() == -1 && c.z() == -11 && c.w() == 7);
    }

    {
      ezSimdVec4i a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
      ezSimdVec4i b(EZ_BIT(4), EZ_BIT(3), EZ_BIT(3), EZ_BIT(5) - 1);

      ezSimdVec4i c = a;
      c |= b;
      EZ_TEST_BOOL(c.x() == (EZ_BIT(1) | EZ_BIT(4)) && c.y() == (EZ_BIT(2) | EZ_BIT(3)) && c.z() == EZ_BIT(3) && c.w() == EZ_BIT(5) - 1);

      c = a;
      c &= b;
      EZ_TEST_BOOL(c.x() == 0 && c.y() == 0 && c.z() == EZ_BIT(3) && c.w() == EZ_BIT(4));

      c = a;
      c ^= b;
      EZ_TEST_BOOL(c.x() == (EZ_BIT(1) | EZ_BIT(4)) && c.y() == (EZ_BIT(2) | EZ_BIT(3)) && c.z() == 0 && c.w() == EZ_BIT(4) - 1);

      c = a;
      c <<= 3;
      EZ_TEST_BOOL(c.x() == EZ_BIT(4) && c.y() == EZ_BIT(5) && c.z() == EZ_BIT(6) && c.w() == EZ_BIT(7));

      c = a;
      c >>= 1;
      EZ_TEST_BOOL(c.x() == EZ_BIT(0) && c.y() == EZ_BIT(1) && c.z() == EZ_BIT(2) && c.w() == EZ_BIT(3));

      c = ezSimdVec4i(-2, -4, -7, -8);
      c >>= 1;
      EZ_TEST_BOOL(c.x() == -1 && c.y() == -2 && c.z() == -4 && c.w() == -4);
    }

    {
      ezSimdVec4i a(-3, 5, -7, 9);
      ezSimdVec4i b(8, 6, 4, 2);
      ezSimdVec4i c;

      c = a.CompMin(b);
      EZ_TEST_BOOL(c.x() == -3 && c.y() == 5 && c.z() == -7 && c.w() == 2);

      c = a.CompMax(b);
      EZ_TEST_BOOL(c.x() == 8 && c.y() == 6 && c.z() == 4 && c.w() == 9);

      c = a.Abs();
      EZ_TEST_BOOL(c.x() == 3 && c.y() == 5 && c.z() == 7 && c.w() == 9);

      ezSimdVec4b cmp(false, true, false, true);
      c = ezSimdVec4i::Select(cmp, a, b);
      EZ_TEST_BOOL(c.x() == 8 && c.y() == 5 && c.z() == 4 && c.w() == 9);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Comparison")
  {
    ezSimdVec4i a(-7, 5, 4, 3);
    ezSimdVec4i b(8, 6, 4, -2);
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
}
