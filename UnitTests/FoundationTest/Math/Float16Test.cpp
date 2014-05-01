#include <PCH.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Strings/String.h>

EZ_CREATE_SIMPLE_TEST(Math, Float16)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "From float and back")
  {
    // default constructor
    EZ_TEST_BOOL(static_cast<float>(ezFloat16()) == 0.0f);

    // Border cases - exact matching needed.
    EZ_TEST_FLOAT(static_cast<float>(ezFloat16(1.0f)), 1.0f, 0);
    EZ_TEST_FLOAT(static_cast<float>(ezFloat16(-1.0f)), -1.0f, 0);
    EZ_TEST_FLOAT(static_cast<float>(ezFloat16(0.0f)), 0.0f, 0);
    EZ_TEST_FLOAT(static_cast<float>(ezFloat16(-0.0f)), -0.0f, 0);
    EZ_TEST_BOOL(static_cast<float>(ezFloat16(ezMath::BasicType<float>::GetInfinity())) == ezMath::BasicType<float>::GetInfinity());
    EZ_TEST_BOOL(static_cast<float>(ezFloat16(-ezMath::BasicType<float>::GetInfinity())) == -ezMath::BasicType<float>::GetInfinity());
    EZ_TEST_BOOL(ezMath::IsNaN(static_cast<float>(ezFloat16(ezMath::BasicType<float>::GetNaN()))));

    // Some random values.
    EZ_TEST_FLOAT(static_cast<float>(ezFloat16(42.0f)), 42.0f, ezMath::BasicType<float>::LargeEpsilon());
    EZ_TEST_FLOAT(static_cast<float>(ezFloat16(1.e3f)), 1.e3f, ezMath::BasicType<float>::LargeEpsilon());
    EZ_TEST_FLOAT(static_cast<float>(ezFloat16(-1230.0f)), -1230.0f, ezMath::BasicType<float>::LargeEpsilon());
    EZ_TEST_FLOAT(static_cast<float>(ezFloat16(ezMath::BasicType<float>::Pi())), ezMath::BasicType<float>::Pi(), ezMath::BasicType<float>::HugeEpsilon());

    // Denormalized float.
    EZ_TEST_FLOAT(static_cast<float>(ezFloat16(1.e-40f)), 0.0f, 0);
    EZ_TEST_FLOAT(static_cast<float>(ezFloat16(1.e-44f)), 0.0f, 0);

    // Clamping of too large/small values
    // Half only supports 2^-14 to 2^14 (in 10^x this is roughly 4.51) (see Wikipedia)
    EZ_TEST_FLOAT(static_cast<float>(ezFloat16(1.e-10f)), 0.0f, 0);
    EZ_TEST_BOOL(static_cast<float>(ezFloat16(1.e5f)) == ezMath::BasicType<float>::GetInfinity());
    EZ_TEST_BOOL(static_cast<float>(ezFloat16(-1.e5f)) == -ezMath::BasicType<float>::GetInfinity());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator ==")
  {
    EZ_TEST_BOOL(ezFloat16(1.0f) == ezFloat16(1.0f));
    EZ_TEST_BOOL(ezFloat16(10000000.0f) == ezFloat16(10000000.0f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator !=")
  {
    EZ_TEST_BOOL(ezFloat16(1.0f) != ezFloat16(-1.0f));
    EZ_TEST_BOOL(ezFloat16(10000000.0f) != ezFloat16(10000.0f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRawData / SetRawData")
  {
    ezFloat16 f;
    f.SetRawData(23);

    EZ_TEST_INT(f.GetRawData(), 23);
  }
}