#include <FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdVec4i.h>

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdVec4i)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with 0xCDCDCDCD.
    ezSimdVec4i vDefCtor;
    EZ_TEST_BOOL(vDefCtor.x() == 0xCDCDCDCD && vDefCtor.y() == 0xCDCDCDCD && vDefCtor.z() == 0xCDCDCDCD && vDefCtor.w() == 0xCDCDCDCD);
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    float EZ_ALIGN_16(testBlock[4]) = {1, 2, 3, 4};
    ezSimdVec4i* pDefCtor = ::new ((void*)&testBlock[0]) ezSimdVec4i;
    EZ_TEST_BOOL(testBlock[0] == 1 && testBlock[1] == 2 && testBlock[2] == 3 && testBlock[3] == 4);
#endif

    // Make sure the class didn't accidentally change in size.
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
    EZ_CHECK_AT_COMPILETIME(sizeof(ezSimdVec4i) == 16);
    EZ_CHECK_AT_COMPILETIME(EZ_ALIGNMENT_OF(ezSimdVec4i) == 16);
#endif

    ezSimdVec4i a(2);
    EZ_TEST_BOOL(a.x() == 2 && a.y() == 2 && a.z() == 2 && a.w() == 2);

    ezSimdVec4i b(1, 2, 3, 4);
    EZ_TEST_BOOL(b.x() == 1 && b.y() == 2 && b.z() == 3 && b.w() == 4);

    // Make sure all components have the correct values
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
    EZ_TEST_BOOL(b.m_v.m128i_i32[0] == 1 && b.m_v.m128i_i32[1] == 2 && b.m_v.m128i_i32[2] == 3 && b.m_v.m128i_i32[3] == 4);
#endif

    ezSimdVec4i copy(b);
    EZ_TEST_BOOL(copy.x() == 1 && copy.y() == 2 && copy.z() == 3 && copy.w() == 4);

    EZ_TEST_BOOL(copy.GetComponent<0>() == 1 && copy.GetComponent<1>() == 2 && copy.GetComponent<2>() == 3 && copy.GetComponent<3>() == 4);

    ezSimdVec4i vZero = ezSimdVec4i::ZeroVector();
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
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion")
  {
    ezSimdVec4i ia(-3, 5, -7, 11);

    ezSimdVec4f fa = ia.ToFloat();
    EZ_TEST_BOOL(fa.x() == -3.0f && fa.y() == 5.0f && fa.z() == -7.0f && fa.w() == 11.0f);

    fa += ezSimdVec4f(0.7f);
    ezSimdVec4i b = ezSimdVec4i::Truncate(fa);
    EZ_TEST_BOOL(b.x() == -2 && b.y() == 5 && b.z() == -6 && b.w() == 11);
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
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Comparison")
  {
    ezSimdVec4i a(7, 5, 4, 3);
    ezSimdVec4i b(8, 6, 4, 2);
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
