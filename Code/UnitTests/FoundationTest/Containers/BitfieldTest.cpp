#include <FoundationTestPCH.h>

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>

EZ_CREATE_SIMPLE_TEST(Containers, Bitfield)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetCount / IsEmpty / Clear")
  {
    ezDynamicBitfield bf; // using a dynamic array

    EZ_TEST_INT(bf.GetCount(), 0);
    EZ_TEST_BOOL(bf.IsEmpty());

    bf.SetCount(15, false);

    EZ_TEST_INT(bf.GetCount(), 15);
    EZ_TEST_BOOL(!bf.IsEmpty());

    bf.Clear();

    EZ_TEST_INT(bf.GetCount(), 0);
    EZ_TEST_BOOL(bf.IsEmpty());

    bf.SetCount(37, false);

    EZ_TEST_INT(bf.GetCount(), 37);
    EZ_TEST_BOOL(!bf.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetCount / SetAllBits / ClearAllBits")
  {
    ezHybridBitfield<16> bf; // using a hybrid array

    bf.SetCount(249, false);
    EZ_TEST_INT(bf.GetCount(), 249);

    for (ezUInt32 i = 0; i < bf.GetCount(); ++i)
      EZ_TEST_BOOL(!bf.IsBitSet(i));

    bf.SetAllBits();
    EZ_TEST_INT(bf.GetCount(), 249);

    for (ezUInt32 i = 0; i < bf.GetCount(); ++i)
      EZ_TEST_BOOL(bf.IsBitSet(i));

    bf.ClearAllBits();
    EZ_TEST_INT(bf.GetCount(), 249);

    for (ezUInt32 i = 0; i < bf.GetCount(); ++i)
      EZ_TEST_BOOL(!bf.IsBitSet(i));


    bf.SetCount(349, true);
    EZ_TEST_INT(bf.GetCount(), 349);

    for (ezUInt32 i = 0; i < 249; ++i)
      EZ_TEST_BOOL(!bf.IsBitSet(i));

    for (ezUInt32 i = 249; i < bf.GetCount(); ++i)
      EZ_TEST_BOOL(bf.IsBitSet(i));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetCount / SetBit / ClearBit / SetCountUninitialized")
  {
    ezHybridBitfield<16> bf; // using a hybrid array

    bf.SetCount(100, false);
    EZ_TEST_INT(bf.GetCount(), 100);

    for (ezUInt32 i = 0; i < bf.GetCount(); ++i)
      EZ_TEST_BOOL(!bf.IsBitSet(i));

    bf.SetCount(200, true);
    EZ_TEST_INT(bf.GetCount(), 200);

    for (ezUInt32 i = 100; i < bf.GetCount(); ++i)
      EZ_TEST_BOOL(bf.IsBitSet(i));

    bf.SetCountUninitialized(250);
    EZ_TEST_INT(bf.GetCount(), 250);

    bf.ClearAllBits();

    for (ezUInt32 i = 0; i < bf.GetCount(); i += 2)
      bf.SetBit(i);

    for (ezUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      EZ_TEST_BOOL(bf.IsBitSet(i));
      EZ_TEST_BOOL(!bf.IsBitSet(i + 1));
    }

    for (ezUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      bf.ClearBit(i);
      bf.SetBit(i + 1);
    }

    for (ezUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      EZ_TEST_BOOL(!bf.IsBitSet(i));
      EZ_TEST_BOOL(bf.IsBitSet(i + 1));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetBitRange")
  {
    for (ezUInt32 size = 1; size < 1024; ++size)
    {
      ezBitfield<ezDeque<ezUInt32>> bf; // using a deque
      bf.SetCount(size, false);

      EZ_TEST_INT(bf.GetCount(), size);

      for (ezUInt32 count = 0; count < bf.GetCount(); ++count)
        EZ_TEST_BOOL(!bf.IsBitSet(count));

      ezUInt32 uiStart = size / 2;
      ezUInt32 uiEnd = ezMath::Min(uiStart + (size / 3 * 2), size - 1);

      bf.SetBitRange(uiStart, uiEnd - uiStart + 1);

      for (ezUInt32 count = 0; count < uiStart; ++count)
        EZ_TEST_BOOL(!bf.IsBitSet(count));
      for (ezUInt32 count = uiStart; count <= uiEnd; ++count)
        EZ_TEST_BOOL(bf.IsBitSet(count));
      for (ezUInt32 count = uiEnd + 1; count < bf.GetCount(); ++count)
        EZ_TEST_BOOL(!bf.IsBitSet(count));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ClearBitRange")
  {
    for (ezUInt32 size = 1; size < 1024; ++size)
    {
      ezBitfield<ezDeque<ezUInt32>> bf; // using a deque
      bf.SetCount(size, true);

      EZ_TEST_INT(bf.GetCount(), size);

      for (ezUInt32 count = 0; count < bf.GetCount(); ++count)
        EZ_TEST_BOOL(bf.IsBitSet(count));

      ezUInt32 uiStart = size / 2;
      ezUInt32 uiEnd = ezMath::Min(uiStart + (size / 3 * 2), size - 1);

      bf.ClearBitRange(uiStart, uiEnd - uiStart + 1);

      for (ezUInt32 count = 0; count < uiStart; ++count)
        EZ_TEST_BOOL(bf.IsBitSet(count));
      for (ezUInt32 count = uiStart; count <= uiEnd; ++count)
        EZ_TEST_BOOL(!bf.IsBitSet(count));
      for (ezUInt32 count = uiEnd + 1; count < bf.GetCount(); ++count)
        EZ_TEST_BOOL(bf.IsBitSet(count));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsAnyBitSet / IsNoBitSet")
  {
    ezHybridBitfield<16> bf; // using a hybrid array

    EZ_TEST_BOOL(bf.IsEmpty() == true);
    EZ_TEST_BOOL(bf.IsAnyBitSet() == false);
    EZ_TEST_BOOL(bf.IsNoBitSet() == true);

    bf.SetCount(250, false);

    EZ_TEST_BOOL(bf.IsEmpty() == false);
    EZ_TEST_BOOL(bf.IsAnyBitSet() == false);
    EZ_TEST_BOOL(bf.IsNoBitSet() == true);

    for (ezUInt32 i = 0; i < bf.GetCount(); i += 2)
      bf.SetBit(i);

    EZ_TEST_BOOL(bf.IsEmpty() == false);
    EZ_TEST_BOOL(bf.IsAnyBitSet() == true);
    EZ_TEST_BOOL(bf.IsNoBitSet() == false);
  }
}


EZ_CREATE_SIMPLE_TEST(Containers, StaticBitfield)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetAllBits / ClearAllBits")
  {
    ezStaticBitfield64 bf;

    for (ezUInt32 i = 0; i < bf.GetNumBits(); ++i)
      EZ_TEST_BOOL(!bf.IsBitSet(i));

    bf.SetAllBits();

    for (ezUInt32 i = 0; i < bf.GetNumBits(); ++i)
      EZ_TEST_BOOL(bf.IsBitSet(i));

    bf.ClearAllBits();

    for (ezUInt32 i = 0; i < bf.GetNumBits(); ++i)
      EZ_TEST_BOOL(!bf.IsBitSet(i));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetBit / ClearBit")
  {
    ezStaticBitfield32 bf;

    for (ezUInt32 i = 0; i < bf.GetNumBits(); ++i)
      EZ_TEST_BOOL(!bf.IsBitSet(i));

    for (ezUInt32 i = 0; i < bf.GetNumBits(); i += 2)
      bf.SetBit(i);

    for (ezUInt32 i = 0; i < bf.GetNumBits(); i += 2)
    {
      EZ_TEST_BOOL(bf.IsBitSet(i));
      EZ_TEST_BOOL(!bf.IsBitSet(i + 1));
    }

    for (ezUInt32 i = 0; i < bf.GetNumBits(); i += 2)
    {
      bf.ClearBit(i);
      bf.SetBit(i + 1);
    }

    for (ezUInt32 i = 0; i < bf.GetNumBits(); i += 2)
    {
      EZ_TEST_BOOL(!bf.IsBitSet(i));
      EZ_TEST_BOOL(bf.IsBitSet(i + 1));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetBitRange")
  {
    for (ezUInt32 uiStart = 0; uiStart < 61; ++uiStart)
    {
      ezStaticBitfield64 bf;

      for (ezUInt32 count = 0; count < bf.GetNumBits(); ++count)
        EZ_TEST_BOOL(!bf.IsBitSet(count));

      ezUInt32 uiEnd = uiStart + 3;

      bf.SetBitRange(uiStart, uiEnd - uiStart + 1);

      for (ezUInt32 count = 0; count < uiStart; ++count)
        EZ_TEST_BOOL(!bf.IsBitSet(count));
      for (ezUInt32 count = uiStart; count <= uiEnd; ++count)
        EZ_TEST_BOOL(bf.IsBitSet(count));
      for (ezUInt32 count = uiEnd + 1; count < bf.GetNumBits(); ++count)
        EZ_TEST_BOOL(!bf.IsBitSet(count));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ClearBitRange")
  {
    for (ezUInt32 uiStart = 0; uiStart < 61; ++uiStart)
    {
      ezStaticBitfield64 bf;
      bf.SetAllBits();

      for (ezUInt32 count = 0; count < bf.GetNumBits(); ++count)
        EZ_TEST_BOOL(bf.IsBitSet(count));

      ezUInt32 uiEnd = uiStart + 3;

      bf.ClearBitRange(uiStart, uiEnd - uiStart + 1);

      for (ezUInt32 count = 0; count < uiStart; ++count)
        EZ_TEST_BOOL(bf.IsBitSet(count));
      for (ezUInt32 count = uiStart; count <= uiEnd; ++count)
        EZ_TEST_BOOL(!bf.IsBitSet(count));
      for (ezUInt32 count = uiEnd + 1; count < bf.GetNumBits(); ++count)
        EZ_TEST_BOOL(bf.IsBitSet(count));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsAnyBitSet / IsNoBitSet")
  {
    ezStaticBitfield8 bf;

    EZ_TEST_BOOL(bf.IsAnyBitSet() == false);
    EZ_TEST_BOOL(bf.IsNoBitSet() == true);

    for (ezUInt32 i = 0; i < bf.GetNumBits(); i += 2)
      bf.SetBit(i);

    EZ_TEST_BOOL(bf.IsAnyBitSet() == true);
    EZ_TEST_BOOL(bf.IsNoBitSet() == false);
  }
}
