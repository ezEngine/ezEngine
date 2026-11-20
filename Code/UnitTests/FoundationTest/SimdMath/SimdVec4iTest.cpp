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
    static_assert(alignof(ezSimdVec4i) == 16);
#endif

    ezSimdVec4i a(2);
    EZ_TEST_BOOL(a.x() == 2 && a.y() == 2 && a.z() == 2 && a.w() == 2);

    ezSimdVec4i b(1, 2, 3, 4);
    EZ_TEST_BOOL(b.x() == 1 && b.y() == 2 && b.z() == 3 && b.w() == 4);

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
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Load")
  {
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
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Store")
  {
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReinterpretAsFloat")
  {
    constexpr ezUInt32 patterns[][4] = {
      {0x7F800000u, 0xFF800000u, 0x7FC12345u, 0xFFC54321u}, // inf, -inf, nan, -nan
      {0x00000001u, 0x80000001u, 0x007FFFFFu, 0x807FFFFFu}, // Subnormals.
      {0x00800000u, 0x80800000u, 0x7F7FFFFFu, 0xFF7FFFFFu}  // Normal range boundaries.
    };

    for (const auto& pattern : patterns)
    {
      ezSimdVec4i vBits;
      vBits.Load<4>(reinterpret_cast<const ezInt32*>(&pattern[0]));
      float result[4];
      vBits.ReinterpretAsFloat().Store<4>(result);
      for (ezUInt32 uiLane = 0; uiLane < 4; ++uiLane)
      {
        EZ_TEST_INT(reinterpret_cast<const ezUInt32&>(result[uiLane]), pattern[uiLane]);
      }
      EZ_TEST_BOOL((vBits.ReinterpretAsFloat().ReinterpretAsInt() == vBits).AllSet<4>());
    }
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransposeBytes")
  {
    EZ_TEST_BOOL((ezSimdVec4i::MakeZero().TransposeBytes() == ezSimdVec4i::MakeZero()).AllSet<4>());
    EZ_TEST_BOOL((ezSimdVec4i(-1).TransposeBytes() == ezSimdVec4i(-1)).AllSet<4>());

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

    ezSimdVec4i vMatrix;
    ezSimdVec4i vExpected;
    vMatrix.Load<4>(reinterpret_cast<const ezInt32*>(matrix));
    vExpected.Load<4>(reinterpret_cast<const ezInt32*>(transposed));
    EZ_TEST_BOOL((vMatrix.TransposeBytes() == vExpected).AllSet<4>());
    EZ_TEST_BOOL((vExpected.TransposeBytes() == vMatrix).AllSet<4>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Unary operators")
  {
    const ezSimdVec4i a(-3, 5, -7, 9);
    const ezSimdVec4i c = -a;
    EZ_TEST_BOOL(c.x() == 3 && c.y() == -5 && c.z() == 7 && c.w() == -9);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Arithmetic operators")
  {
    {
      const ezSimdVec4i a(-3, 5, -7, 9);
      const ezSimdVec4i b(8, 6, 4, 2);
      const ezSimdVec4i cSum = a + b;
      EZ_TEST_BOOL(cSum.x() == 5 && cSum.y() == 11 && cSum.z() == -3 && cSum.w() == 11);
    }

    {
      const ezSimdVec4i a(-3, 5, -7, 9);
      const ezSimdVec4i b(8, 6, 4, 2);
      const ezSimdVec4i cDifference = a - b;
      EZ_TEST_BOOL(cDifference.x() == -11 && cDifference.y() == -1 && cDifference.z() == -11 && cDifference.w() == 7);
    }

    {
      const ezSimdVec4i a(-3, 5, -7, 9);
      const ezSimdVec4i b(8, 6, 4, 2);
      const ezSimdVec4i cMul = a.CompMul(b);
      EZ_TEST_BOOL(cMul.x() == -24 && cMul.y() == 30 && cMul.z() == -28 && cMul.w() == 18);
    }

    {
      const ezSimdVec4i a(-3, 5, -7, 9);
      const ezSimdVec4i b(8, 6, 4, 2);
      const ezSimdVec4i cDiv = a.CompDiv(b);
      EZ_TEST_BOOL(cDiv.x() == 0 && cDiv.y() == 0 && cDiv.z() == -1 && cDiv.w() == 4);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Bitwise operators")
  {
    const ezSimdVec4i a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
    const ezSimdVec4i b(EZ_BIT(4), EZ_BIT(3), EZ_BIT(3), EZ_BIT(5) - 1);

    {
      const ezSimdVec4i cOr = a | b;
      EZ_TEST_BOOL(cOr.x() == (EZ_BIT(1) | EZ_BIT(4)) && cOr.y() == (EZ_BIT(2) | EZ_BIT(3)) && cOr.z() == EZ_BIT(3) && cOr.w() == EZ_BIT(5) - 1);
    }

    {
      const ezSimdVec4i cAnd = a & b;
      EZ_TEST_BOOL(cAnd.x() == 0 && cAnd.y() == 0 && cAnd.z() == EZ_BIT(3) && cAnd.w() == EZ_BIT(4));
    }

    {
      const ezSimdVec4i cXor = a ^ b;
      EZ_TEST_BOOL(cXor.x() == (EZ_BIT(1) | EZ_BIT(4)) && cXor.y() == (EZ_BIT(2) | EZ_BIT(3)) && cXor.z() == 0 && cXor.w() == EZ_BIT(4) - 1);
    }

    {
      const ezSimdVec4i cNot = ~a;
      EZ_TEST_BOOL(cNot.x() == ~EZ_BIT(1) && cNot.y() == ~EZ_BIT(2) && cNot.z() == ~EZ_BIT(3) && cNot.w() == ~EZ_BIT(4));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Shift operators")
  {
    const ezSimdVec4i a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
    {
      const ezSimdVec4i cLeft = a << 3;
      EZ_TEST_BOOL(cLeft.x() == EZ_BIT(4) && cLeft.y() == EZ_BIT(5) && cLeft.z() == EZ_BIT(6) && cLeft.w() == EZ_BIT(7));
    }
    {
      const ezSimdVec4i cRight = a >> 1;
      EZ_TEST_BOOL(cRight.x() == EZ_BIT(0) && cRight.y() == EZ_BIT(1) && cRight.z() == EZ_BIT(2) && cRight.w() == EZ_BIT(3));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Component Shift operators")
  {
    {
      const ezSimdVec4i a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
      const ezSimdVec4i s(1, 2, 3, 4);
      const ezSimdVec4i cLeft = a << s;
      EZ_TEST_BOOL(cLeft.x() == EZ_BIT(2) && cLeft.y() == EZ_BIT(4) && cLeft.z() == EZ_BIT(6) && cLeft.w() == EZ_BIT(8));
    }
    {
      const ezSimdVec4i b(EZ_BIT(4), EZ_BIT(3), EZ_BIT(3), EZ_BIT(5) - 1);
      const ezSimdVec4i s(1, 2, 3, 4);
      const ezSimdVec4i cRight = b >> s;
      EZ_TEST_BOOL(cRight.x() == EZ_BIT(3) && cRight.y() == EZ_BIT(1) && cRight.z() == EZ_BIT(0) && cRight.w() == EZ_BIT(0));
    }
    {
      const ezSimdVec4i values(1, 2, 4, 8);
      const ezSimdVec4i counts(1, 2, 3, 4);
      const ezSimdVec4i cLeft = values << counts;
      EZ_TEST_BOOL(cLeft.x() == 2 && cLeft.y() == 8 && cLeft.z() == 32 && cLeft.w() == 128);
    }
    {
      const ezSimdVec4i values(-1, -2, -4, -8);
      const ezSimdVec4i counts(1, 2, 3, 4);
      const ezSimdVec4i cLeft = values << counts;
      EZ_TEST_BOOL(cLeft.x() == -2 && cLeft.y() == -8 && cLeft.z() == -32 && cLeft.w() == -128);
    }
    {
      const ezSimdVec4i values(16, -32, 64, -128);
      const ezSimdVec4i counts(1, 2, 3, 4);
      const ezSimdVec4i cRight = values >> counts;
      EZ_TEST_BOOL(cRight.x() == 8 && cRight.y() == -8 && cRight.z() == 8 && cRight.w() == -8);
    }
    {
      const ezSimdVec4i values(0x7FFFFFFF, static_cast<ezInt32>(0x80000000u), 0x40000000, 1);
      const ezSimdVec4i counts(31, 31, 1, 31);
      const ezSimdVec4i cLeft = values << counts;
      EZ_TEST_BOOL(cLeft.x() == static_cast<ezInt32>(0x80000000u) && cLeft.y() == 0 && cLeft.z() == static_cast<ezInt32>(0x80000000u) && cLeft.w() == static_cast<ezInt32>(0x80000000u));
      const ezSimdVec4i cRight = values >> counts;
      EZ_TEST_BOOL(cRight.x() == 0 && cRight.y() == -1 && cRight.z() == 0x20000000 && cRight.w() == 0);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compound arithmetic operators")
  {
    {
      const ezSimdVec4i a(-3, 5, -7, 9);
      const ezSimdVec4i b(8, 6, 4, 2);
      ezSimdVec4i cAdd = a;
      cAdd += b;
      EZ_TEST_BOOL(cAdd.x() == 5 && cAdd.y() == 11 && cAdd.z() == -3 && cAdd.w() == 11);
    }
    {
      const ezSimdVec4i a(-3, 5, -7, 9);
      const ezSimdVec4i b(8, 6, 4, 2);
      ezSimdVec4i cSubtract = a;
      cSubtract -= b;
      EZ_TEST_BOOL(cSubtract.x() == -11 && cSubtract.y() == -1 && cSubtract.z() == -11 && cSubtract.w() == 7);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compound bitwise operators")
  {
    {
      const ezSimdVec4i a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
      const ezSimdVec4i b(EZ_BIT(4), EZ_BIT(3), EZ_BIT(3), EZ_BIT(5) - 1);
      ezSimdVec4i cOr = a;
      cOr |= b;
      EZ_TEST_BOOL(cOr.x() == (EZ_BIT(1) | EZ_BIT(4)) && cOr.y() == (EZ_BIT(2) | EZ_BIT(3)) && cOr.z() == EZ_BIT(3) && cOr.w() == EZ_BIT(5) - 1);
    }
    {
      const ezSimdVec4i a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
      const ezSimdVec4i b(EZ_BIT(4), EZ_BIT(3), EZ_BIT(3), EZ_BIT(5) - 1);
      ezSimdVec4i cAnd = a;
      cAnd &= b;
      EZ_TEST_BOOL(cAnd.x() == 0 && cAnd.y() == 0 && cAnd.z() == EZ_BIT(3) && cAnd.w() == EZ_BIT(4));
    }
    {
      const ezSimdVec4i a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
      const ezSimdVec4i b(EZ_BIT(4), EZ_BIT(3), EZ_BIT(3), EZ_BIT(5) - 1);
      ezSimdVec4i cXor = a;
      cXor ^= b;
      EZ_TEST_BOOL(cXor.x() == (EZ_BIT(1) | EZ_BIT(4)) && cXor.y() == (EZ_BIT(2) | EZ_BIT(3)) && cXor.z() == 0 && cXor.w() == EZ_BIT(4) - 1);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compound shift operators")
  {
    {
      const ezSimdVec4i a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
      ezSimdVec4i cLeftShift = a;
      cLeftShift <<= 3;
      EZ_TEST_BOOL(cLeftShift.x() == EZ_BIT(4) && cLeftShift.y() == EZ_BIT(5) && cLeftShift.z() == EZ_BIT(6) && cLeftShift.w() == EZ_BIT(7));
    }
    {
      const ezSimdVec4i a(EZ_BIT(1), EZ_BIT(2), EZ_BIT(3), EZ_BIT(4));
      ezSimdVec4i cRightShift = a;
      cRightShift >>= 1;
      EZ_TEST_BOOL(cRightShift.x() == EZ_BIT(0) && cRightShift.y() == EZ_BIT(1) && cRightShift.z() == EZ_BIT(2) && cRightShift.w() == EZ_BIT(3));
    }
    {
      ezSimdVec4i cNegativeRightShift(-2, -4, -7, -8);
      cNegativeRightShift >>= 1;
      EZ_TEST_BOOL(cNegativeRightShift.x() == -1 && cNegativeRightShift.y() == -2 && cNegativeRightShift.z() == -4 && cNegativeRightShift.w() == -4);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Min/Max/Abs/Select")
  {
    {
      const ezSimdVec4i a(-3, 5, -7, 9);
      const ezSimdVec4i b(8, 6, 4, 2);
      const ezSimdVec4i cMin = a.CompMin(b);
      EZ_TEST_BOOL(cMin.x() == -3 && cMin.y() == 5 && cMin.z() == -7 && cMin.w() == 2);
    }
    {
      const ezSimdVec4i a(-3, 5, -7, 9);
      const ezSimdVec4i b(8, 6, 4, 2);
      const ezSimdVec4i cMax = a.CompMax(b);
      EZ_TEST_BOOL(cMax.x() == 8 && cMax.y() == 6 && cMax.z() == 4 && cMax.w() == 9);
    }
    {
      const ezSimdVec4i a(-3, 5, -7, 9);
      const ezSimdVec4i cAbs = a.Abs();
      EZ_TEST_BOOL(cAbs.x() == 3 && cAbs.y() == 5 && cAbs.z() == 7 && cAbs.w() == 9);
    }
    {
      const ezSimdVec4i a(-3, 5, -7, 9);
      const ezSimdVec4i b(8, 6, 4, 2);
      const ezSimdVec4b cmp(false, true, false, true);
      const ezSimdVec4i cSelected = ezSimdVec4i::Select(cmp, a, b);
      EZ_TEST_BOOL(cSelected.x() == 8 && cSelected.y() == 5 && cSelected.z() == 4 && cSelected.w() == 9);
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
