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
    static_assert(alignof(ezSimdVec4u) == 16);
#endif

    ezSimdVec4u a(2);
    EZ_TEST_BOOL(a.x() == 2 && a.y() == 2 && a.z() == 2 && a.w() == 2);

    ezSimdVec4u b(1, 2, 3, 0xFFFFFFFFu);
    EZ_TEST_BOOL(b.x() == 1 && b.y() == 2 && b.z() == 3 && b.w() == 0xFFFFFFFFu);

    ezSimdVec4u copy(b);
    EZ_TEST_BOOL(copy.x() == 1 && copy.y() == 2 && copy.z() == 3 && copy.w() == 0xFFFFFFFFu);

    EZ_TEST_BOOL(copy.GetComponent<0>() == 1 && copy.GetComponent<1>() == 2 && copy.GetComponent<2>() == 3 && copy.GetComponent<3>() == 0xFFFFFFFFu);

    ezSimdVec4u vZero = ezSimdVec4u::MakeZero();
    EZ_TEST_BOOL(vZero.x() == 0 && vZero.y() == 0 && vZero.z() == 0 && vZero.w() == 0);
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Load")
  {
    {
      ezUInt32 testBlock[4] = {1, 2, 3, 4};
      ezSimdVec4u x;
      x.Load<1>(testBlock);
      EZ_TEST_BOOL(x.x() == 1 && x.y() == 0 && x.z() == 0 && x.w() == 0);

      ezSimdVec4u xy;
      xy.Load<2>(testBlock);
      EZ_TEST_BOOL(xy.x() == 1 && xy.y() == 2 && xy.z() == 0 && xy.w() == 0);

      ezSimdVec4u xyz;
      xyz.Load<3>(testBlock);
      EZ_TEST_BOOL(xyz.x() == 1 && xyz.y() == 2 && xyz.z() == 3 && xyz.w() == 0);

      ezSimdVec4u xyzw;
      xyzw.Load<4>(testBlock);
      EZ_TEST_BOOL(xyzw.x() == 1 && xyzw.y() == 2 && xyzw.z() == 3 && xyzw.w() == 4);

      EZ_TEST_INT(xyzw.GetComponent<0>(), 1);
      EZ_TEST_INT(xyzw.GetComponent<1>(), 2);
      EZ_TEST_INT(xyzw.GetComponent<2>(), 3);
      EZ_TEST_INT(xyzw.GetComponent<3>(), 4);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Store")
  {
    {
      ezUInt32 testBlock[4] = {7, 7, 7, 7};
      ezUInt32 mem[4] = {};

      ezSimdVec4u b2(1, 2, 3, 4);

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReinterpretAsFloat")
  {
    constexpr ezUInt32 patterns[][4] = {
      {0x7F800000u, 0xFF800000u, 0x7FC12345u, 0xFFC54321u}, // inf, -inf, nan, -nan
      {0x00000001u, 0x80000001u, 0x007FFFFFu, 0x807FFFFFu}, // Subnormals.
      {0x00800000u, 0x80800000u, 0x7F7FFFFFu, 0xFF7FFFFFu}  // Normal range boundaries.
    };

    for (const auto& pattern : patterns)
    {
      ezSimdVec4u vBits;
      vBits.Load<4>(pattern);
      float result[4];
      vBits.ReinterpretAsFloat().Store<4>(result);
      for (ezUInt32 uiLane = 0; uiLane < 4; ++uiLane)
      {
        EZ_TEST_INT(reinterpret_cast<const ezUInt32&>(result[uiLane]), pattern[uiLane]);
      }
    }
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetCombined")
  {
    ezSimdVec4u a(2, 4, 6, 8);
    ezSimdVec4u b(3, 5, 7, 9);

    ezSimdVec4u c = a.GetCombined<ezSwizzle::XXXX>(b);
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

    c = a.GetCombined<ezSwizzle::YYYY>(b);
    EZ_TEST_BOOL(c.x() == a.y() && c.y() == a.y() && c.z() == b.y() && c.w() == b.y());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Arithmetic operators")
  {
    {
      const ezSimdVec4u a(13, 5, 17, 9);
      const ezSimdVec4u b(8, 6, 4, 2);
      const ezSimdVec4u cSum = a + b;
      EZ_TEST_BOOL(cSum.x() == 21 && cSum.y() == 11 && cSum.z() == 21 && cSum.w() == 11);
    }
    {
      const ezSimdVec4u a(13, 5, 17, 9);
      const ezSimdVec4u b(8, 6, 4, 2);
      const ezSimdVec4u cDifference = a - b;
      EZ_TEST_BOOL(cDifference.x() == 5 && cDifference.y() == 0xFFFFFFFFu && cDifference.z() == 13 && cDifference.w() == 7);
    }
    {
      const ezSimdVec4u b(8, 6, 4, 2);
      const ezSimdVec4u cMul = ezSimdVec4u(0xFFFFFFFFu).CompMul(b);
      EZ_TEST_BOOL(cMul.x() == 4294967288u && cMul.y() == 4294967290u && cMul.z() == 4294967292u && cMul.w() == 4294967294u);
    }
    {
      const ezSimdVec4u b(8, 6, 4, 2);
      const ezSimdVec4u cMaxDiv = ezSimdVec4u(0xFFFFFFFFu).CompDiv(b);
      EZ_TEST_BOOL(cMaxDiv.x() == 0xFFFFFFFFu / 8u && cMaxDiv.y() == 0xFFFFFFFFu / 6u && cMaxDiv.z() == 0xFFFFFFFFu / 4u && cMaxDiv.w() == 0xFFFFFFFFu / 2u);
    }
    {
      const ezSimdVec4u dividends(24, 25, 100, 17);
      const ezSimdVec4u divisors(3, 5, 10, 4);
      const ezSimdVec4u cExact = dividends.CompDiv(divisors);
      EZ_TEST_BOOL(cExact.x() == 8 && cExact.y() == 5 && cExact.z() == 10 && cExact.w() == 4);
    }
    {
      const ezSimdVec4u dividends(13, 5, 17, 9);
      const ezSimdVec4u divisors(8, 6, 4, 2);
      const ezSimdVec4u cTruncated = dividends.CompDiv(divisors);
      EZ_TEST_BOOL(cTruncated.x() == 1 && cTruncated.y() == 0 && cTruncated.z() == 4 && cTruncated.w() == 4);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Bitwise operators")
  {
    const ezSimdVec4u a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
    const ezSimdVec4u b(EZ_BIT(4), EZ_BIT(3), EZ_BIT(3), EZ_BIT(5) - 1);
    {
      const ezSimdVec4u cOr = a | b;
      EZ_TEST_BOOL(cOr.x() == (EZ_BIT(1) | EZ_BIT(4)) && cOr.y() == (EZ_BIT(2) | EZ_BIT(3)) && cOr.z() == EZ_BIT(3) && cOr.w() == EZ_BIT(5) - 1);
    }
    {
      const ezSimdVec4u cAnd = a & b;
      EZ_TEST_BOOL(cAnd.x() == 0 && cAnd.y() == 0 && cAnd.z() == EZ_BIT(3) && cAnd.w() == EZ_BIT(4));
    }
    {
      const ezSimdVec4u cXor = a ^ b;
      EZ_TEST_BOOL(cXor.x() == (EZ_BIT(1) | EZ_BIT(4)) && cXor.y() == (EZ_BIT(2) | EZ_BIT(3)) && cXor.z() == 0 && cXor.w() == EZ_BIT(4) - 1);
    }
    {
      const ezSimdVec4u cNot = ~a;
      EZ_TEST_BOOL(cNot.x() == 0xFFFFFFFD && cNot.y() == 0xFFFFFFFB && cNot.z() == 0xFFFFFFF7 && cNot.w() == 0xFFFFFFEF);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Shift operators")
  {
    const ezSimdVec4u a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
    {
      const ezSimdVec4u cLeft = a << 3;
      EZ_TEST_BOOL(cLeft.x() == EZ_BIT(4) && cLeft.y() == EZ_BIT(5) && cLeft.z() == EZ_BIT(6) && cLeft.w() == EZ_BIT(7));
    }
    {
      const ezSimdVec4u cRight = a >> 1;
      EZ_TEST_BOOL(cRight.x() == EZ_BIT(0) && cRight.y() == EZ_BIT(1) && cRight.z() == EZ_BIT(2) && cRight.w() == EZ_BIT(3));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Component Shift operators")
  {
    {
      const ezSimdVec4u values(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
      const ezSimdVec4u counts(1, 2, 3, 4);
      const ezSimdVec4u cLeft = values << counts;
      EZ_TEST_BOOL(cLeft.x() == EZ_BIT(2) && cLeft.y() == EZ_BIT(4) && cLeft.z() == EZ_BIT(6) && cLeft.w() == EZ_BIT(8));
    }
    {
      const ezSimdVec4u values(EZ_BIT(4), EZ_BIT(3), EZ_BIT(3), EZ_BIT(5) - 1);
      const ezSimdVec4u counts(1, 2, 3, 4);
      const ezSimdVec4u cRight = values >> counts;
      EZ_TEST_BOOL(cRight.x() == EZ_BIT(3) && cRight.y() == EZ_BIT(1) && cRight.z() == EZ_BIT(0) && cRight.w() == EZ_BIT(0));
    }
    {
      const ezSimdVec4u values(1, 2, 4, 8);
      const ezSimdVec4u counts(1, 2, 3, 4);
      const ezSimdVec4u cLeft = values << counts;
      EZ_TEST_BOOL(cLeft.x() == 2 && cLeft.y() == 8 && cLeft.z() == 32 && cLeft.w() == 128);
    }
    {
      const ezSimdVec4u values(16, 32, 64, 128);
      const ezSimdVec4u counts(1, 2, 3, 4);
      const ezSimdVec4u cRight = values >> counts;
      EZ_TEST_BOOL(cRight.x() == 8 && cRight.y() == 8 && cRight.z() == 8 && cRight.w() == 8);
    }
    {
      const ezSimdVec4u values(1, 2, 0xFFFFFFFFu, 0x80000000u);
      const ezSimdVec4u counts(32, 33, 255, 0xFFFFFFFFu);
      const ezSimdVec4u cLeft = values << counts;
      EZ_TEST_BOOL(cLeft.x() == 0 && cLeft.y() == 0 && cLeft.z() == 0 && cLeft.w() == 0);
      const ezSimdVec4u cRight = values >> counts;
      EZ_TEST_BOOL(cRight.x() == 0 && cRight.y() == 0 && cRight.z() == 0 && cRight.w() == 0);
    }
    {
      const ezSimdVec4u values(0xFFFFFFFFu, 0x80000000u, 0x89ABCDEFu, 1);
      const ezSimdVec4u counts(1, 1, 32, 31);
      const ezSimdVec4u cLeft = values << counts;
      EZ_TEST_BOOL(cLeft.x() == 0xFFFFFFFEu && cLeft.y() == 0 && cLeft.z() == 0 && cLeft.w() == 0x80000000u);
      const ezSimdVec4u cRight = values >> counts;
      EZ_TEST_BOOL(cRight.x() == 0x7FFFFFFFu && cRight.y() == 0x40000000u && cRight.z() == 0 && cRight.w() == 0);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compound arithmetic operators")
  {
    {
      const ezSimdVec4u a(13, 5, 17, 9);
      const ezSimdVec4u b(8, 6, 4, 2);
      ezSimdVec4u cAdd = a;
      cAdd += b;
      EZ_TEST_BOOL(cAdd.x() == 21 && cAdd.y() == 11 && cAdd.z() == 21 && cAdd.w() == 11);
    }
    {
      const ezSimdVec4u a(13, 5, 17, 9);
      const ezSimdVec4u b(8, 6, 4, 2);
      ezSimdVec4u cSubtract = a;
      cSubtract -= b;
      EZ_TEST_BOOL(cSubtract.x() == 5 && cSubtract.y() == 0xFFFFFFFFu && cSubtract.z() == 13 && cSubtract.w() == 7);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compound bitwise operators")
  {
    {
      const ezSimdVec4u a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
      const ezSimdVec4u b(EZ_BIT(4), EZ_BIT(3), EZ_BIT(3), EZ_BIT(5) - 1);
      ezSimdVec4u cOr = a;
      cOr |= b;
      EZ_TEST_BOOL(cOr.x() == (EZ_BIT(1) | EZ_BIT(4)) && cOr.y() == (EZ_BIT(2) | EZ_BIT(3)) && cOr.z() == EZ_BIT(3) && cOr.w() == EZ_BIT(5) - 1);
    }
    {
      const ezSimdVec4u a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
      const ezSimdVec4u b(EZ_BIT(4), EZ_BIT(3), EZ_BIT(3), EZ_BIT(5) - 1);
      ezSimdVec4u cAnd = a;
      cAnd &= b;
      EZ_TEST_BOOL(cAnd.x() == 0 && cAnd.y() == 0 && cAnd.z() == EZ_BIT(3) && cAnd.w() == EZ_BIT(4));
    }
    {
      const ezSimdVec4u a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
      const ezSimdVec4u b(EZ_BIT(4), EZ_BIT(3), EZ_BIT(3), EZ_BIT(5) - 1);
      ezSimdVec4u cXor = a;
      cXor ^= b;
      EZ_TEST_BOOL(cXor.x() == (EZ_BIT(1) | EZ_BIT(4)) && cXor.y() == (EZ_BIT(2) | EZ_BIT(3)) && cXor.z() == 0 && cXor.w() == EZ_BIT(4) - 1);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compound shift operators")
  {
    {
      const ezSimdVec4u a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
      ezSimdVec4u cLeftShift = a;
      cLeftShift <<= 3;
      EZ_TEST_BOOL(cLeftShift.x() == EZ_BIT(4) && cLeftShift.y() == EZ_BIT(5) && cLeftShift.z() == EZ_BIT(6) && cLeftShift.w() == EZ_BIT(7));
    }
    {
      const ezSimdVec4u a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
      ezSimdVec4u cRightShift = a;
      cRightShift >>= 1;
      EZ_TEST_BOOL(cRightShift.x() == EZ_BIT(0) && cRightShift.y() == EZ_BIT(1) && cRightShift.z() == EZ_BIT(2) && cRightShift.w() == EZ_BIT(3));
    }
    {
      ezSimdVec4u cMaxRightShift(0xFFFFFFFEu, 0xFFFFFFFCu, 0xFFFFFFF9u, 0xFFFFFFF8u);
      cMaxRightShift >>= 1;
      EZ_TEST_BOOL(cMaxRightShift.x() == 0x7FFFFFFF && cMaxRightShift.y() == 0x7FFFFFFE && cMaxRightShift.z() == 0x7FFFFFFC && cMaxRightShift.w() == 0x7FFFFFFC);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Min/Max/Select")
  {
    {
      const ezSimdVec4u a(13, 5, 17, 9);
      const ezSimdVec4u b(8, 6, 4, 2);
      const ezSimdVec4u cMin = a.CompMin(b);
      EZ_TEST_BOOL(cMin.x() == 8 && cMin.y() == 5 && cMin.z() == 4 && cMin.w() == 2);
    }
    {
      const ezSimdVec4u a(13, 5, 17, 9);
      const ezSimdVec4u b(8, 6, 4, 2);
      const ezSimdVec4u cMax = a.CompMax(b);
      EZ_TEST_BOOL(cMax.x() == 13 && cMax.y() == 6 && cMax.z() == 17 && cMax.w() == 9);
    }
    {
      const ezSimdVec4u a(13, 5, 17, 9);
      const ezSimdVec4u b(8, 6, 4, 2);
      const ezSimdVec4b cmp(false, true, false, true);
      const ezSimdVec4u cSelected = ezSimdVec4u::Select(cmp, a, b);
      EZ_TEST_BOOL(cSelected.x() == 8 && cSelected.y() == 5 && cSelected.z() == 4 && cSelected.w() == 9);
    }
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransposeBytes")
  {
    EZ_TEST_BOOL((ezSimdVec4u::MakeZero().TransposeBytes() == ezSimdVec4u::MakeZero()).AllSet<4>());
    EZ_TEST_BOOL((ezSimdVec4u(0xFFFFFFFFu).TransposeBytes() == ezSimdVec4u(0xFFFFFFFFu)).AllSet<4>());

    alignas(16) const ezUInt8 matrix[16] = {
      0x00, 0x01, 0x02, 0x03,
      0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0A, 0x0B,
      0x0C, 0x0D, 0x0E, 0x0F};
    alignas(16) const ezUInt8 transposed[16] = {
      0x00, 0x04, 0x08, 0x0C,
      0x01, 0x05, 0x09, 0x0D,
      0x02, 0x06, 0x0A, 0x0E,
      0x03, 0x07, 0x0B, 0x0F};

    ezSimdVec4u vMatrix;
    ezSimdVec4u vExpected;
    vMatrix.Load<4>(reinterpret_cast<const ezUInt32*>(matrix));
    vExpected.Load<4>(reinterpret_cast<const ezUInt32*>(transposed));
    EZ_TEST_BOOL((vMatrix.TransposeBytes() == vExpected).AllSet<4>());
    EZ_TEST_BOOL((vExpected.TransposeBytes() == vMatrix).AllSet<4>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SubtractSaturated16")
  {
    // Subtract independent low and high 16-bit lanes.
    {
      const ezSimdVec4u vA(0x0005000Au, 0x00340012u, 0x12345678u, 0xFFFF8000u);
      const ezSimdVec4u vB(0x00020003u, 0x00120010u, 0x12345678u, 0x7FFF8000u);
      const ezSimdVec4u vExpected(0x00030007u, 0x00220002u, 0, 0x80000000u);
      EZ_TEST_BOOL((vA.SubtractSaturated16(vB) == vExpected).AllSet<4>());
    }
    // Clamp low and high lane underflows independently to zero.
    {
      const ezSimdVec4u vA(0x00010000u, 0x80007FFFu, 0xFFFF8000u, 0x12345678u);
      const ezSimdVec4u vB(0x00020001u, 0x7FFF8000u, 0x8000FFFFu, 0x12345679u);
      const ezSimdVec4u vExpected(0, 0x00010000u, 0x7FFF0000u, 0);
      EZ_TEST_BOOL((vA.SubtractSaturated16(vB) == vExpected).AllSet<4>());
    }
    // Preserve values when subtracting zero and produce zero when subtracting a value from itself.
    {
      const ezSimdVec4u vA(0xFFFF8000u, 0x8000FFFFu, 0x00000000u, 0x12345678u);
      EZ_TEST_BOOL((vA.SubtractSaturated16(ezSimdVec4u::MakeZero()) == vA).AllSet<4>());
      EZ_TEST_BOOL((ezSimdVec4u::MakeZero().SubtractSaturated16(vA) == ezSimdVec4u::MakeZero()).AllSet<4>());
      EZ_TEST_BOOL((vA.SubtractSaturated16(vA) == ezSimdVec4u::MakeZero()).AllSet<4>());
    }
    // Exercise zero, 0x7FFF, 0x8000, and 0xFFFF lane boundaries.
    {
      const ezSimdVec4u vA(0xFFFF0000u, 0x0000FFFFu, 0x80007FFFu, 0x12345678u);
      const ezSimdVec4u vB(0x0000FFFFu, 0xFFFF0000u, 0x7FFF8000u, 0x12345678u);
      EZ_TEST_BOOL((vA.SubtractSaturated16(vB) == ezSimdVec4u(0xFFFF0000u, 0x0000FFFFu, 0x00010000u, 0)).AllSet<4>());
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
