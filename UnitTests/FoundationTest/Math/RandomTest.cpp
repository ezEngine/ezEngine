#include <PCH.h>
#include <Foundation/Math/Random.h>

EZ_CREATE_SIMPLE_TEST(Math, Random)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "UIntInRange")
  {
    ezRandom r;
    r.Initialize(0xAABBCCDDEEFF0011ULL);

    for (ezUInt32 i = 2; i < 10000; ++i)
    {
      const ezUInt32 val = r.UIntInRange(i);
      EZ_TEST_BOOL(val < i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IntInRange")
  {
    ezRandom r;
    r.Initialize(0xBBCCDDEEFF0011AAULL);

    EZ_TEST_INT(r.IntInRange(5, 0), 5);
    EZ_TEST_INT(r.IntInRange(5, 1), 5);
    EZ_TEST_INT(r.IntInRange(-5, 0), -5);
    EZ_TEST_INT(r.IntInRange(-5, 1), -5);

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const ezInt32 val = r.IntInRange(i, i);
      EZ_TEST_BOOL(val >= i);
      EZ_TEST_BOOL(val < i + i);
    }

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const ezInt32 val = r.IntInRange(-i, 2 * i);
      EZ_TEST_BOOL(val >= -i);
      EZ_TEST_BOOL(val < -i + 2 * i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IntMinMax")
  {
    ezRandom r;
    r.Initialize(0xCCDDEEFF0011AABBULL);

    EZ_TEST_INT(r.IntMinMax(5, 5), 5);
    EZ_TEST_INT(r.IntMinMax(-5, -5), -5);

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const ezInt32 val = r.IntMinMax(i, 2 * i);
      EZ_TEST_BOOL(val >= i);
      EZ_TEST_BOOL(val <= i + i);
    }

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const ezInt32 val = r.IntMinMax(-i, i);
      EZ_TEST_BOOL(val >= -i);
      EZ_TEST_BOOL(val <=  i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "DoubleZeroToOneExclusive")
  {
    ezRandom r;
    r.Initialize(0xDDEEFF0011AABBCCULL);

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleZeroToOneExclusive();
      EZ_TEST_BOOL(val >= 0.0);
      EZ_TEST_BOOL(val < 1.0);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "DoubleZeroToOneInclusive")
  {
    ezRandom r;
    r.Initialize(0xEEFF0011AABBCCDDULL);

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleZeroToOneInclusive();
      EZ_TEST_BOOL(val >= 0.0);
      EZ_TEST_BOOL(val <= 1.0);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "DoubleInRange")
  {
    ezRandom r;
    r.Initialize(0xFF0011AABBCCDDEEULL);

    EZ_TEST_DOUBLE(r.DoubleInRange(5, 0), 5, 0.0);
    EZ_TEST_DOUBLE(r.DoubleInRange(-5, 0), -5, 0.0);

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleInRange(i, i);
      EZ_TEST_BOOL(val >= i);
      EZ_TEST_BOOL(val < i + i);
    }

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleInRange(-i, 2 * i);
      EZ_TEST_BOOL(val >= -i);
      EZ_TEST_BOOL(val < -i + 2 * i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "DoubleMinMax")
  {
    ezRandom r;
    r.Initialize(0x0011AABBCCDDEEFFULL);

    EZ_TEST_DOUBLE(r.DoubleMinMax(5, 5), 5, 0.0);
    EZ_TEST_DOUBLE(r.DoubleMinMax(-5, -5), -5, 0.0);

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleMinMax(i, 2 * i);
      EZ_TEST_BOOL(val >= i);
      EZ_TEST_BOOL(val <= i + i);
    }

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleMinMax(-i, i);
      EZ_TEST_BOOL(val >= -i);
      EZ_TEST_BOOL(val <=  i);
    }
  }

}
