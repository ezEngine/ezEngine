#include <PCH.h>
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
    ezBitfield<ezHybridArray<ezUInt32, 16> > bf; // using a hybrid array

    bf.SetCount(249, false);
    EZ_TEST_INT(bf.GetCount(), 249);

    for (ezUInt32 i = 0; i < bf.GetCount(); ++i)
      EZ_TEST_BOOL(!bf.IsSet(i));
    
    bf.SetAllBits();
    EZ_TEST_INT(bf.GetCount(), 249);

    for (ezUInt32 i = 0; i < bf.GetCount(); ++i)
      EZ_TEST_BOOL(bf.IsSet(i));

    bf.ClearAllBits();
    EZ_TEST_INT(bf.GetCount(), 249);

    for (ezUInt32 i = 0; i < bf.GetCount(); ++i)
      EZ_TEST_BOOL(!bf.IsSet(i));
      

    bf.SetCount(349, true);
    EZ_TEST_INT(bf.GetCount(), 349);

    for (ezUInt32 i = 0; i < 249; ++i)
      EZ_TEST_BOOL(!bf.IsSet(i));

    for (ezUInt32 i = 249; i < bf.GetCount(); ++i)
      EZ_TEST_BOOL(bf.IsSet(i));
  }
  
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetCount / SetBit / ClearBit")
  {
    ezBitfield<ezHybridArray<ezUInt32, 16> > bf; // using a hybrid array

    bf.SetCount(250, false);
    EZ_TEST_INT(bf.GetCount(), 250);

    for (ezUInt32 i = 0; i < bf.GetCount(); ++i)
      EZ_TEST_BOOL(!bf.IsSet(i));
    
    for (ezUInt32 i = 0; i < bf.GetCount(); i += 2)
      bf.SetBit(i);
    
    for (ezUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      EZ_TEST_BOOL(bf.IsSet(i));
      EZ_TEST_BOOL(!bf.IsSet(i + 1));
    }
    
    for (ezUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      bf.ClearBit(i);
      bf.SetBit(i + 1);
    }

    for (ezUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      EZ_TEST_BOOL(!bf.IsSet(i));
      EZ_TEST_BOOL(bf.IsSet(i + 1));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRange")
  {
    for (ezUInt32 size = 1; size < 1024; ++size)
    {
      ezBitfield<ezDeque<ezUInt32> > bf; // using a deque
      bf.SetCount(size, false);

      EZ_TEST_INT(bf.GetCount(), size);

      for (ezUInt32 count = 0; count < bf.GetCount(); ++count)
        EZ_TEST_BOOL(!bf.IsSet(count));

      ezUInt32 uiStart = size / 2;
      ezUInt32 uiEnd = ezMath::Min(uiStart + (size / 3 * 2), size - 1);

      bf.SetRange(uiStart, uiEnd);

      for (ezUInt32 count = 0; count < uiStart; ++count)
        EZ_TEST_BOOL(!bf.IsSet(count));
      for (ezUInt32 count = uiStart; count <= uiEnd; ++count)
        EZ_TEST_BOOL(bf.IsSet(count));
      for (ezUInt32 count = uiEnd + 1; count < bf.GetCount(); ++count)
        EZ_TEST_BOOL(!bf.IsSet(count));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ClearRange")
  {
    for (ezUInt32 size = 1; size < 1024; ++size)
    {
      ezBitfield<ezDeque<ezUInt32> > bf; // using a deque
      bf.SetCount(size, true);

      EZ_TEST_INT(bf.GetCount(), size);

      for (ezUInt32 count = 0; count < bf.GetCount(); ++count)
        EZ_TEST_BOOL(bf.IsSet(count));

      ezUInt32 uiStart = size / 2;
      ezUInt32 uiEnd = ezMath::Min(uiStart + (size / 3 * 2), size - 1);

      bf.ClearRange(uiStart, uiEnd);

      for (ezUInt32 count = 0; count < uiStart; ++count)
        EZ_TEST_BOOL(bf.IsSet(count));
      for (ezUInt32 count = uiStart; count <= uiEnd; ++count)
        EZ_TEST_BOOL(!bf.IsSet(count));
      for (ezUInt32 count = uiEnd + 1; count < bf.GetCount(); ++count)
        EZ_TEST_BOOL(bf.IsSet(count));
    }
  }

}


