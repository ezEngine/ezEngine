#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/IterateBits.h>

namespace
{
  template <typename T>
  void TestEmptyIntegerBitValues()
  {
    ezUInt32 uiNextBit = 1;
    for (auto bit : ezIterateBitValues(static_cast<T>(0)))
    {
      EZ_TEST_BOOL_MSG(false, "No bit should be present");
    }
  }

  template <typename T>
  void TestFullIntegerBitValues()
  {
    constexpr ezUInt64 uiBitCount = sizeof(T) * 8;
    ezUInt64 uiNextBit = 1;
    ezUInt64 uiCount = 0;
    for (auto bit : ezIterateBitValues(ezMath::MaxValue<T>()))
    {
      EZ_TEST_INT(bit, uiNextBit);
      uiNextBit *= 2;
      uiCount++;
    }
    EZ_TEST_INT(uiBitCount, uiCount);
  }

  template <typename T>
  void TestEmptyIntegerBitIndices()
  {
    ezUInt32 uiNextBit = 1;
    for (auto bit : ezIterateBitIndices(static_cast<T>(0)))
    {
      EZ_TEST_BOOL_MSG(false, "No bit should be present");
    }
  }

  template <typename T>
  void TestFullIntegerBitIndices()
  {
    constexpr ezUInt64 uiBitCount = sizeof(T) * 8;
    ezUInt64 uiNextBitIndex = 0;
    for (auto bit : ezIterateBitIndices(ezMath::MaxValue<T>()))
    {
      EZ_TEST_INT(bit, uiNextBitIndex);
      ++uiNextBitIndex;
    }
    EZ_TEST_INT(uiBitCount, uiNextBitIndex);
  }
} // namespace

EZ_CREATE_SIMPLE_TEST(Containers, IterateBits)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezIterateBitValues")
  {
    {
      // Empty set
      TestEmptyIntegerBitValues<ezUInt8>();
      TestEmptyIntegerBitValues<ezUInt16>();
      TestEmptyIntegerBitValues<ezUInt32>();
      TestEmptyIntegerBitValues<ezUInt64>();
    }

    {
      // Full sets
      TestFullIntegerBitValues<ezUInt8>();
      TestFullIntegerBitValues<ezUInt16>();
      TestFullIntegerBitValues<ezUInt32>();
      TestFullIntegerBitValues<ezUInt64>();
    }

    {
      // Some bits set
      ezUInt64 uiBitMask = 0b1101;
      ezHybridArray<ezUInt64, 3> bits;
      bits.PushBack(0b0001);
      bits.PushBack(0b0100);
      bits.PushBack(0b1000);

      for (ezUInt64 bit : ezIterateBitValues(uiBitMask))
      {
        EZ_TEST_INT(bit, bits[0]);
        bits.RemoveAtAndCopy(0);
      }
      EZ_TEST_BOOL(bits.IsEmpty());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezIterateBitIndices")
  {
    {
      // Empty set
      TestEmptyIntegerBitIndices<ezUInt8>();
      TestEmptyIntegerBitIndices<ezUInt16>();
      TestEmptyIntegerBitIndices<ezUInt32>();
      TestEmptyIntegerBitIndices<ezUInt64>();
    }

    {
      // Full sets
      TestFullIntegerBitIndices<ezUInt8>();
      TestFullIntegerBitIndices<ezUInt16>();
      TestFullIntegerBitIndices<ezUInt32>();
      TestFullIntegerBitIndices<ezUInt64>();
    }

    {
      // Some bits set
      ezUInt64 uiBitMask = 0b1101;
      ezHybridArray<ezUInt64, 3> bits;
      bits.PushBack(0);
      bits.PushBack(2);
      bits.PushBack(3);

      for (ezUInt64 bit : ezIterateBitIndices(uiBitMask))
      {
        EZ_TEST_INT(bit, bits[0]);
        bits.RemoveAtAndCopy(0);
      }
      EZ_TEST_BOOL(bits.IsEmpty());
    }
  }
}
