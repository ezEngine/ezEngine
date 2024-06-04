#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdVec4u.h>

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdVec4u)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
    // In debug the default constructor initializes everything with 0xCDCDCDCD.
    ezSimdVec4u vDefCtor;
    EZ_TEST_BOOL(vDefCtor.x() == 0xCDCDCDCD && vDefCtor.y() == 0xCDCDCDCD && vDefCtor.z() == 0xCDCDCDCD && vDefCtor.w() == 0xCDCDCDCD);
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) float testBlock[4] = {1, 2, 3, 4};
    ezSimdVec4u* pDefCtor = ::new ((void*)&testBlock[0]) ezSimdVec4u;
    EZ_TEST_BOOL(testBlock[0] == 1 && testBlock[1] == 2 && testBlock[2] == 3 && testBlock[3] == 4);
#endif

    // Make sure the class didn't accidentally change in size.
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
    static_assert(sizeof(ezSimdVec4u) == 16);
    static_assert(EZ_ALIGNMENT_OF(ezSimdVec4u) == 16);
#endif

    ezSimdVec4u a(2);
    EZ_TEST_BOOL(a.x() == 2 && a.y() == 2 && a.z() == 2 && a.w() == 2);

    ezSimdVec4u b(1, 2, 3, 0xFFFFFFFFu);
    EZ_TEST_BOOL(b.x() == 1 && b.y() == 2 && b.z() == 3 && b.w() == 0xFFFFFFFFu);

    // Make sure all components have the correct values
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE && EZ_ENABLED(EZ_COMPILER_MSVC)
    EZ_TEST_BOOL(b.m_v.m128i_u32[0] == 1 && b.m_v.m128i_u32[1] == 2 && b.m_v.m128i_u32[2] == 3 && b.m_v.m128i_u32[3] == 0xFFFFFFFFu);
#endif

    ezSimdVec4u copy(b);
    EZ_TEST_BOOL(copy.x() == 1 && copy.y() == 2 && copy.z() == 3 && copy.w() == 0xFFFFFFFFu);

    EZ_TEST_BOOL(copy.GetComponent<0>() == 1 && copy.GetComponent<1>() == 2 && copy.GetComponent<2>() == 3 && copy.GetComponent<3>() == 0xFFFFFFFFu);

    {
      ezSimdVec4u vZero = ezSimdVec4u::MakeZero();
      EZ_TEST_BOOL(vZero.x() == 0 && vZero.y() == 0 && vZero.z() == 0 && vZero.w() == 0);
    }

    {
      ezSimdVec4u vZero = ezSimdVec4u::MakeZero();
      EZ_TEST_BOOL(vZero.x() == 0 && vZero.y() == 0 && vZero.z() == 0 && vZero.w() == 0);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Setter")
  {
    ezSimdVec4u a;
    a.Set(2);
    EZ_TEST_BOOL(a.x() == 2 && a.y() == 2 && a.z() == 2 && a.w() == 2);

    ezSimdVec4u b;
    b.Set(1, 2, 3, 4);
    EZ_TEST_BOOL(b.x() == 1 && b.y() == 2 && b.z() == 3 && b.w() == 4);

    ezSimdVec4u vSetZero;
    vSetZero.SetZero();
    EZ_TEST_BOOL(vSetZero.x() == 0 && vSetZero.y() == 0 && vSetZero.z() == 0 && vSetZero.w() == 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion")
  {
    ezSimdVec4u ua(-10000, 5, -7, 11);

    ezSimdVec4i ia(ua);
    EZ_TEST_BOOL(ia.x() == -10000 && ia.y() == 5 && ia.z() == -7 && ia.w() == 11);

    ezSimdVec4f fa = ua.ToFloat();
    EZ_TEST_BOOL(fa.x() == 4294957296.0f && fa.y() == 5.0f && fa.z() == 4294967289.0f && fa.w() == 11.0f);

    fa = ezSimdVec4f(-2.3f, 5.7f, -4294967040.0f, 4294967040.0f);
    ezSimdVec4u b = ezSimdVec4u::Truncate(fa);
    EZ_TEST_INT(b.x(), 0);
    EZ_TEST_INT(b.y(), 5);
    EZ_TEST_INT(b.z(), 0);
    EZ_TEST_INT(b.w(), 4294967040);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Swizzle")
  {
    ezSimdVec4u a(3, 5, 7, 9);

    ezSimdVec4u b = a.Get<ezSwizzle::XXXX>();
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
      ezSimdVec4u a(-3, 5, -7, 9);
      ezSimdVec4u b(8, 6, 4, 2);
      ezSimdVec4u c;
      c = a + b;
      EZ_TEST_BOOL(c.x() == 5 && c.y() == 11 && c.z() == -3 && c.w() == 11);

      c = a - b;
      EZ_TEST_BOOL(c.x() == -11 && c.y() == -1 && c.z() == -11 && c.w() == 7);

      a.Set(0xFFFFFFFF);
      c = a.CompMul(b);
      EZ_TEST_BOOL(c.x() == 4294967288u && c.y() == 4294967290u && c.z() == 4294967292u && c.w() == 4294967294u);
    }

    {
      ezSimdVec4u a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
      ezSimdVec4u b(EZ_BIT(4), EZ_BIT(3), EZ_BIT(3), EZ_BIT(5) - 1);
      ezSimdVec4u c;

      c = a | b;
      EZ_TEST_BOOL(c.x() == (EZ_BIT(1) | EZ_BIT(4)) && c.y() == (EZ_BIT(2) | EZ_BIT(3)) && c.z() == EZ_BIT(3) && c.w() == EZ_BIT(5) - 1);

      c = a & b;
      EZ_TEST_BOOL(c.x() == 0 && c.y() == 0 && c.z() == EZ_BIT(3) && c.w() == EZ_BIT(4));

      c = a ^ b;
      EZ_TEST_BOOL(c.x() == (EZ_BIT(1) | EZ_BIT(4)) && c.y() == (EZ_BIT(2) | EZ_BIT(3)) && c.z() == 0 && c.w() == EZ_BIT(4) - 1);

      c = ~a;
      EZ_TEST_BOOL(c.x() == 0xFFFFFFFD && c.y() == 0xFFFFFFFB && c.z() == 0xFFFFFFF7 && c.w() == 0xFFFFFFEF);

      c = a << 3;
      EZ_TEST_BOOL(c.x() == EZ_BIT(4) && c.y() == EZ_BIT(5) && c.z() == EZ_BIT(6) && c.w() == EZ_BIT(7));

      c = a >> 1;
      EZ_TEST_BOOL(c.x() == EZ_BIT(0) && c.y() == EZ_BIT(1) && c.z() == EZ_BIT(2) && c.w() == EZ_BIT(3));
    }

    {
      ezSimdVec4u a(-3, 5, -7, 9);
      ezSimdVec4u b(8, 6, 4, 2);

      ezSimdVec4u c = a;
      c += b;
      EZ_TEST_BOOL(c.x() == 5 && c.y() == 11 && c.z() == -3 && c.w() == 11);

      c = a;
      c -= b;
      EZ_TEST_BOOL(c.x() == -11 && c.y() == -1 && c.z() == -11 && c.w() == 7);
    }

    {
      ezSimdVec4u a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
      ezSimdVec4u b(EZ_BIT(4), EZ_BIT(3), EZ_BIT(3), EZ_BIT(5) - 1);

      ezSimdVec4u c = a;
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

      c = ezSimdVec4u(-2, -4, -7, -8);
      EZ_TEST_BOOL(c.x() == 0xFFFFFFFE && c.y() == 0xFFFFFFFC && c.z() == 0xFFFFFFF9 && c.w() == 0xFFFFFFF8);
      c >>= 1;
      EZ_TEST_BOOL(c.x() == 0x7FFFFFFF && c.y() == 0x7FFFFFFE && c.z() == 0x7FFFFFFC && c.w() == 0x7FFFFFFC);
    }

    {
      ezSimdVec4u a(-3, 5, -7, 9);
      ezSimdVec4u b(8, 6, 4, 2);
      ezSimdVec4u c;

      c = a.CompMin(b);
      EZ_TEST_BOOL(c.x() == 8 && c.y() == 5 && c.z() == 4 && c.w() == 2);

      c = a.CompMax(b);
      EZ_TEST_BOOL(c.x() == -3 && c.y() == 6 && c.z() == -7 && c.w() == 9);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Comparison")
  {
    ezSimdVec4u a(-7, 5, 4, 3);
    ezSimdVec4u b(8, 6, 4, -2);
    ezSimdVec4b cmp;

    cmp = a == b;
    EZ_TEST_BOOL(!cmp.x() && !cmp.y() && cmp.z() && !cmp.w());

    cmp = a != b;
    EZ_TEST_BOOL(cmp.x() && cmp.y() && !cmp.z() && cmp.w());

    cmp = a <= b;
    EZ_TEST_BOOL(!cmp.x() && cmp.y() && cmp.z() && cmp.w());

    cmp = a < b;
    EZ_TEST_BOOL(!cmp.x() && cmp.y() && !cmp.z() && cmp.w());

    cmp = a >= b;
    EZ_TEST_BOOL(cmp.x() && !cmp.y() && cmp.z() && !cmp.w());

    cmp = a > b;
    EZ_TEST_BOOL(cmp.x() && !cmp.y() && !cmp.z() && !cmp.w());
  }
}
