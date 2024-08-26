#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Random.h>
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
    ezHybridBitfield<512> bf; // using a hybrid array

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetCount / SetBit / FlipBit / ClearBit / SetBitValue / SetCountUninitialized")
  {
    ezHybridBitfield<512> bf; // using a hybrid array

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

    for (ezUInt32 i = 0; i < bf.GetCount(); ++i)
    {
      bf.SetBitValue(i, (i % 3) == 0);
    }

    for (ezUInt32 i = 0; i < bf.GetCount(); ++i)
    {
      EZ_TEST_BOOL(bf.IsBitSet(i) == ((i % 3) == 0));
    }

    for (ezUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      bf.FlipBit(i);
    }

    for (ezUInt32 i = 0; i < bf.GetCount(); ++i)
    {
      EZ_TEST_BOOL(bf.IsBitSet(i) == (((0b011100 >> (i % 6)) & 1) == 1));
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FlipBitRange")
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

      bf.FlipBitRange(uiStart, uiEnd - uiStart + 1);

      for (ezUInt32 count = 0; count < uiStart; ++count)
        EZ_TEST_BOOL(bf.IsBitSet(count));
      for (ezUInt32 count = uiStart; count <= uiEnd; ++count)
        EZ_TEST_BOOL(!bf.IsBitSet(count));
      for (ezUInt32 count = uiEnd + 1; count < bf.GetCount(); ++count)
        EZ_TEST_BOOL(bf.IsBitSet(count));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsAnyBitSet / IsNoBitSet / AreAllBitsSet")
  {
    ezHybridBitfield<512> bf;                  // using a hybrid array

    EZ_TEST_BOOL(bf.IsEmpty() == true);
    EZ_TEST_BOOL(bf.IsAnyBitSet() == false);   // empty
    EZ_TEST_BOOL(bf.IsNoBitSet() == true);
    EZ_TEST_BOOL(bf.AreAllBitsSet() == false); // empty

    bf.SetCount(250, false);

    EZ_TEST_BOOL(bf.IsEmpty() == false);
    EZ_TEST_BOOL(bf.IsAnyBitSet() == false);
    EZ_TEST_BOOL(bf.IsNoBitSet() == true);
    EZ_TEST_BOOL(bf.AreAllBitsSet() == false);

    for (ezUInt32 i = 0; i < bf.GetCount(); i += 2)
      bf.SetBit(i);

    EZ_TEST_BOOL(bf.IsEmpty() == false);
    EZ_TEST_BOOL(bf.IsAnyBitSet() == true);
    EZ_TEST_BOOL(bf.IsNoBitSet() == false);
    EZ_TEST_BOOL(bf.AreAllBitsSet() == false);

    for (ezUInt32 i = 0; i < bf.GetCount(); i++)
      bf.SetBit(i);

    EZ_TEST_BOOL(bf.IsAnyBitSet() == true);
    EZ_TEST_BOOL(bf.IsNoBitSet() == false);
    EZ_TEST_BOOL(bf.AreAllBitsSet() == true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Swap")
  {
    ezHybridBitfield<512> bf0; // using a hybrid array
    ezHybridBitfield<512> bf1; // using a hybrid array

    ezUInt32 bitFieldCount0 = 100;
    ezUInt32 bitFieldCount1 = 999;
    ezUInt32 bitIndexSet0 = 2;
    ezUInt32 bitIndexSet1 = 555;

    bf0.SetCount(bitFieldCount0, false);
    bf1.SetCount(bitFieldCount1, false);
    bf0.SetBit(bitIndexSet0);
    bf1.SetBit(bitIndexSet1);

    EZ_TEST_BOOL(bitFieldCount0 == bf0.GetCount());
    for (ezUInt32 i = 0; i < bf0.GetCount(); ++i)
    {
      EZ_TEST_BOOL(bf0.IsBitSet(i) == (bitIndexSet0 == i));
    }

    EZ_TEST_BOOL(bitFieldCount1 == bf1.GetCount());
    for (ezUInt32 i = 0; i < bf1.GetCount(); ++i)
    {
      EZ_TEST_BOOL(bf1.IsBitSet(i) == (bitIndexSet1 == i));
    }

    bf0.Swap(bf1);
    ezMath::Swap(bitIndexSet0, bitIndexSet1);
    ezMath::Swap(bitFieldCount0, bitFieldCount1);

    EZ_TEST_BOOL(bitFieldCount0 == bf0.GetCount());
    for (ezUInt32 i = 0; i < bf0.GetCount(); ++i)
    {
      EZ_TEST_BOOL(bf0.IsBitSet(i) == (bitIndexSet0 == i));
    }

    EZ_TEST_BOOL(bitFieldCount1 == bf1.GetCount());
    for (ezUInt32 i = 0; i < bf1.GetCount(); ++i)
    {
      EZ_TEST_BOOL(bf1.IsBitSet(i) == (bitIndexSet1 == i));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Iterator")
  {
    {
      // Check empty bitfields of varying sizes.
      for (ezUInt32 uiNumBits = 0; uiNumBits <= 65; ++uiNumBits)
      {
        ezHybridBitfield<128> bitfield;
        bitfield.SetCount(uiNumBits, true);
        for (ezUInt32 b = 0; b < uiNumBits; ++b)
        {
          bitfield.ClearBit(b);
        }
        for (ezUInt32 uiBit : bitfield)
        {
          EZ_TEST_BOOL_MSG(false, "No bit should be set");
        }

        for (auto it = bitfield.GetIterator(); it.IsValid(); it.Next())
        {
          EZ_TEST_BOOL_MSG(false, "No bit should be set");
        }
        EZ_TEST_BOOL(bitfield.GetIterator() == bitfield.GetEndIterator());
        EZ_TEST_BOOL(!bitfield.GetIterator().IsValid());
        EZ_TEST_BOOL(!bitfield.GetEndIterator().IsValid());
      }
    }

    {
      // Full bits.
      for (ezUInt32 uiNumBits = 0; uiNumBits <= 65; ++uiNumBits)
      {
        ezHybridBitfield<128> bitfield;
        bitfield.SetCount(uiNumBits, true);
        ezUInt32 uiNextBit = 0;
        for (ezUInt32 uiBit : bitfield)
        {
          EZ_TEST_INT(uiBit, uiNextBit);
          uiNextBit++;
        }
        EZ_TEST_INT(uiNumBits, uiNextBit);

        uiNextBit = 0;
        for (auto it = bitfield.GetIterator(); it.IsValid(); ++it)
        {
          EZ_TEST_INT(it.Value(), uiNextBit);
          EZ_TEST_INT(*it, uiNextBit);
          EZ_TEST_BOOL(it.IsValid());
          uiNextBit++;
        }
        EZ_TEST_INT(uiNumBits, uiNextBit);
      }
    }

    {
      // Partial bits set.
      ezRandom rnd;
      rnd.Initialize(42);

      for (ezUInt32 uiNumBits = 2; uiNumBits <= 65; ++uiNumBits)
      {
        ezHybridBitfield<128> bitfield;
        bitfield.SetCount(uiNumBits, false);

        // Add some random bits and ensure they appear in the iterator in order.
        ezHybridArray<ezUInt32, 3> bits;
        for (int i = 0; i < uiNumBits / 2; ++i)
        {
          ezUInt32 bit = (ezUInt32)rnd.IntMinMax(0, uiNumBits - 1);
          if (!bitfield.IsBitSet(bit))
          {
            bits.PushBack(bit);
            bitfield.SetBit(bit);
          }
        }
        bits.Sort();

        for (ezUInt32 uiBit : bitfield)
        {
          EZ_TEST_INT(uiBit, bits[0]);
          bits.RemoveAtAndCopy(0);
        }
        EZ_TEST_BOOL(bits.IsEmpty());
      }
    }
  }
}


EZ_CREATE_SIMPLE_TEST(Containers, StaticBitfield)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetAllBits / ClearAllBits")
  {
    ezStaticBitfield64 bf;

    for (ezUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
      EZ_TEST_BOOL(!bf.IsBitSet(i));

    bf.SetAllBits();

    for (ezUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
      EZ_TEST_BOOL(bf.IsBitSet(i));

    bf.ClearAllBits();

    for (ezUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
      EZ_TEST_BOOL(!bf.IsBitSet(i));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetBit / ClearBit / SetBitValue")
  {
    ezStaticBitfield32 bf;

    for (ezUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
      EZ_TEST_BOOL(!bf.IsBitSet(i));

    for (ezUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i += 2)
      bf.SetBit(i);

    for (ezUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i += 2)
    {
      EZ_TEST_BOOL(bf.IsBitSet(i));
      EZ_TEST_BOOL(!bf.IsBitSet(i + 1));
    }

    for (ezUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i += 2)
    {
      bf.ClearBit(i);
      bf.SetBit(i + 1);
    }

    for (ezUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i += 2)
    {
      EZ_TEST_BOOL(!bf.IsBitSet(i));
      EZ_TEST_BOOL(bf.IsBitSet(i + 1));
    }

    for (ezUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
    {
      bf.SetBitValue(i, (i % 3) == 0);
    }

    for (ezUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
    {
      EZ_TEST_BOOL(bf.IsBitSet(i) == ((i % 3) == 0));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetBitRange")
  {
    for (ezUInt32 uiStart = 0; uiStart < 61; ++uiStart)
    {
      ezStaticBitfield64 bf;

      for (ezUInt32 count = 0; count < bf.GetStorageTypeBitCount(); ++count)
        EZ_TEST_BOOL(!bf.IsBitSet(count));

      ezUInt32 uiEnd = uiStart + 3;

      bf.SetBitRange(uiStart, uiEnd - uiStart + 1);

      for (ezUInt32 count = 0; count < uiStart; ++count)
        EZ_TEST_BOOL(!bf.IsBitSet(count));
      for (ezUInt32 count = uiStart; count <= uiEnd; ++count)
        EZ_TEST_BOOL(bf.IsBitSet(count));
      for (ezUInt32 count = uiEnd + 1; count < bf.GetStorageTypeBitCount(); ++count)
        EZ_TEST_BOOL(!bf.IsBitSet(count));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ClearBitRange")
  {
    for (ezUInt32 uiStart = 0; uiStart < 61; ++uiStart)
    {
      ezStaticBitfield64 bf;
      bf.SetAllBits();

      for (ezUInt32 count = 0; count < bf.GetStorageTypeBitCount(); ++count)
        EZ_TEST_BOOL(bf.IsBitSet(count));

      ezUInt32 uiEnd = uiStart + 3;

      bf.ClearBitRange(uiStart, uiEnd - uiStart + 1);

      for (ezUInt32 count = 0; count < uiStart; ++count)
        EZ_TEST_BOOL(bf.IsBitSet(count));
      for (ezUInt32 count = uiStart; count <= uiEnd; ++count)
        EZ_TEST_BOOL(!bf.IsBitSet(count));
      for (ezUInt32 count = uiEnd + 1; count < bf.GetStorageTypeBitCount(); ++count)
        EZ_TEST_BOOL(bf.IsBitSet(count));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsAnyBitSet / IsNoBitSet / AreAllBitsSet")
  {
    ezStaticBitfield8 bf;

    EZ_TEST_BOOL(bf.IsAnyBitSet() == false);   // empty
    EZ_TEST_BOOL(bf.IsNoBitSet() == true);
    EZ_TEST_BOOL(bf.AreAllBitsSet() == false); // empty

    for (ezUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i += 2)
      bf.SetBit(i);

    EZ_TEST_BOOL(bf.IsAnyBitSet() == true);
    EZ_TEST_BOOL(bf.IsNoBitSet() == false);
    EZ_TEST_BOOL(bf.AreAllBitsSet() == false);

    for (ezUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i++)
      bf.SetBit(i);

    EZ_TEST_BOOL(bf.IsAnyBitSet() == true);
    EZ_TEST_BOOL(bf.IsNoBitSet() == false);
    EZ_TEST_BOOL(bf.AreAllBitsSet() == true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetNumBitsSet")
  {
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0).GetNumBitsSet(), 0);
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0xff).GetNumBitsSet(), 8);
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0xffff).GetNumBitsSet(), 16);
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0xffffffffu).GetNumBitsSet(), 32);
    EZ_TEST_INT(ezStaticBitfield64::MakeFromMask(0).GetNumBitsSet(), 0);
    EZ_TEST_INT(ezStaticBitfield64::MakeFromMask(0xff).GetNumBitsSet(), 8);
    EZ_TEST_INT(ezStaticBitfield64::MakeFromMask(0xffff).GetNumBitsSet(), 16);
    EZ_TEST_INT(ezStaticBitfield64::MakeFromMask(0xffffffffu).GetNumBitsSet(), 32);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLowestBitSet")
  {
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0u).GetLowestBitSet(), 32);
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(1u).GetLowestBitSet(), 0);
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0xffu).GetLowestBitSet(), 0);
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0xff00u).GetLowestBitSet(), 8);
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0xff0000u).GetLowestBitSet(), 16);
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0xff000000u).GetLowestBitSet(), 24);
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0x80000000u).GetLowestBitSet(), 31);
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0xffffffffu).GetLowestBitSet(), 0);
    EZ_TEST_INT(ezStaticBitfield64::MakeFromMask(0xffffffffffffffffull).GetLowestBitSet(), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetHighestBitSet")
  {
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0u).GetHighestBitSet(), 32);
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(1u).GetHighestBitSet(), 0);
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0xffu).GetHighestBitSet(), 7);
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0xff00u).GetHighestBitSet(), 15);
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0xff0000u).GetHighestBitSet(), 23);
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0xff000000u).GetHighestBitSet(), 31);
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0x80000000u).GetHighestBitSet(), 31);
    EZ_TEST_INT(ezStaticBitfield32::MakeFromMask(0xffffffffu).GetHighestBitSet(), 31);
    EZ_TEST_INT(ezStaticBitfield64::MakeFromMask(0xffffffffffffffffull).GetHighestBitSet(), 63);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Iterator")
  {
    {
      // Empty bitfield
      ezStaticBitfield32 bitfield = ezStaticBitfield32::MakeFromMask(0u);
      for (ezUInt32 uiBit : bitfield)
      {
        EZ_TEST_BOOL_MSG(false, "No bit should be set");
      }
      for (auto it = bitfield.GetIterator(); it.IsValid(); it.Next())
      {
        EZ_TEST_BOOL_MSG(false, "No bit should be set");
      }
      EZ_TEST_BOOL(bitfield.GetIterator() == bitfield.GetEndIterator());
      EZ_TEST_BOOL(!bitfield.GetIterator().IsValid());
      EZ_TEST_BOOL(!bitfield.GetEndIterator().IsValid());

      ezStaticBitfield64 bitfield64 = ezStaticBitfield64::MakeFromMask(0u);
      for (ezUInt32 uiBit : bitfield64)
      {
        EZ_TEST_BOOL_MSG(false, "No bit should be set");
      }
      for (auto it = bitfield64.GetIterator(); it.IsValid(); it.Next())
      {
        EZ_TEST_BOOL_MSG(false, "No bit should be set");
      }
      EZ_TEST_BOOL(bitfield64.GetIterator() == bitfield64.GetEndIterator());
      EZ_TEST_BOOL(!bitfield64.GetIterator().IsValid());
      EZ_TEST_BOOL(!bitfield64.GetEndIterator().IsValid());
    }

    {
      // Full 32 bits
      ezStaticBitfield32 bitfield = ezStaticBitfield32::MakeFromMask(0xffffffffu);
      ezUInt32 uiNextBit = 0;
      for (ezUInt32 uiBit : bitfield)
      {
        EZ_TEST_INT(uiBit, uiNextBit);
        uiNextBit++;
      }
      EZ_TEST_INT(32, uiNextBit);

      uiNextBit = 0;
      for (auto it = bitfield.GetIterator(); it.IsValid(); ++it)
      {
        EZ_TEST_INT(it.Value(), uiNextBit);
        EZ_TEST_INT(*it, uiNextBit);
        EZ_TEST_BOOL(it.IsValid());
        uiNextBit++;
      }
      EZ_TEST_INT(32, uiNextBit);
    }

    {
      // Full 64 bits
      ezStaticBitfield64 bitfield = ezStaticBitfield64::MakeFromMask(0xffffffffffffffffull);
      ezUInt32 uiNextBit = 0;
      for (ezUInt32 uiBit : bitfield)
      {
        EZ_TEST_INT(uiBit, uiNextBit);
        uiNextBit++;
      }
      EZ_TEST_INT(64, uiNextBit);

      uiNextBit = 0;
      for (auto it = bitfield.GetIterator(); it.IsValid(); ++it)
      {
        EZ_TEST_INT(it.Value(), uiNextBit);
        EZ_TEST_INT(*it, uiNextBit);
        EZ_TEST_BOOL(it.IsValid());
        uiNextBit++;
      }
      EZ_TEST_INT(64, uiNextBit);
    }

    {
      // Partial bits set 32 bit.
      ezRandom rnd;
      rnd.Initialize(42);

      for (ezUInt32 uiNumBits = 2; uiNumBits <= 32; ++uiNumBits)
      {
        // Add some random bits and ensure they appear in the iterator in order.
        ezHybridArray<ezUInt32, 3> bits;
        ezUInt32 uiBits = 0;
        for (int i = 0; i < uiNumBits; ++i)
        {
          const ezUInt32 bit = (ezUInt32)rnd.IntMinMax(0, 31);
          if (!bits.Contains(bit))
          {
            bits.PushBack(bit);
            uiBits |= EZ_BIT(bit);
          }
        }
        bits.Sort();

        ezStaticBitfield32 bitfield = ezStaticBitfield32::MakeFromMask(uiBits);

        for (ezUInt32 uiBit : bitfield)
        {
          EZ_TEST_INT(uiBit, bits[0]);
          bits.RemoveAtAndCopy(0);
        }
        EZ_TEST_BOOL(bits.IsEmpty());
      }
    }

    {
      // Partial bits set 64 bit.
      ezRandom rnd;
      rnd.Initialize(42);

      for (ezUInt32 uiNumBits = 2; uiNumBits <= 63; ++uiNumBits)
      {
        // Add some random bits and ensure they appear in the iterator in order.
        ezHybridArray<ezUInt32, 3> bits;
        ezUInt64 uiBits = 0;
        for (int i = 0; i < uiNumBits; ++i)
        {
          const ezUInt32 bit = (ezUInt32)rnd.IntMinMax(0, 63);
          if (!bits.Contains(bit))
          {
            bits.PushBack(bit);
            uiBits |= EZ_BIT(bit);
          }
        }
        bits.Sort();

        ezStaticBitfield64 bitfield = ezStaticBitfield64::MakeFromMask(uiBits);

        for (ezUInt32 uiBit : bitfield)
        {
          EZ_TEST_INT(uiBit, bits[0]);
          bits.RemoveAtAndCopy(0);
        }
        EZ_TEST_BOOL(bits.IsEmpty());
      }
    }
  }
}
