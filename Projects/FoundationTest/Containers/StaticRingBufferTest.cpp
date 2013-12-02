#include <PCH.h>
#include <Foundation/Containers/StaticRingBuffer.h>

typedef ezConstructionCounter cc;

EZ_CREATE_SIMPLE_TEST(Containers, StaticRingBuffer)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    EZ_TEST(ezConstructionCounter::HasAllDestructed());

    {
      ezStaticRingBuffer<ezInt32, 32> r1;
      ezStaticRingBuffer<ezInt32, 16> r2;
      ezStaticRingBuffer<cc, 2> r3;
    }

    EZ_TEST(ezConstructionCounter::HasAllDestructed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy Constructor / Operator=")
  {
    EZ_TEST(ezConstructionCounter::HasAllDestructed());

    {
      ezStaticRingBuffer<cc, 16> r1;

      for (ezUInt32 i = 0; i < 16; ++i)
        r1.PushBack(cc(i));

      ezStaticRingBuffer<cc, 16> r2 (r1);

      for (ezUInt32 i = 0; i < 16; ++i)
        EZ_TEST(r2[i] == cc(i));

      ezStaticRingBuffer<cc, 16> r3;
      r3 = r1;

      for (ezUInt32 i = 0; i < 16; ++i)
        EZ_TEST(r3[i] == cc(i));
    }

    EZ_TEST(ezConstructionCounter::HasAllDestructed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operator==")
  {
    EZ_TEST(ezConstructionCounter::HasAllDestructed());

    {
      ezStaticRingBuffer<cc, 16> r1;

      for (ezUInt32 i = 0; i < 16; ++i)
        r1.PushBack(cc(i));

      ezStaticRingBuffer<cc, 16> r2 (r1);
      ezStaticRingBuffer<cc, 16> r3 (r1);
      r3.PeekFront() = cc(3);

      EZ_TEST(r1 == r1);
      EZ_TEST(r2 == r2);
      EZ_TEST(r3 == r3);

      EZ_TEST(r1 == r2);
      EZ_TEST(r1 != r3);
      EZ_TEST(r2 != r3);
    }

    EZ_TEST(ezConstructionCounter::HasAllDestructed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PushBack / operator[] / CanAppend")
  {
    ezStaticRingBuffer<ezInt32, 16> r;

    for (ezUInt32 i = 0; i < 16; ++i)
    {
      EZ_TEST(r.CanAppend());
      r.PushBack(i);
    }

    EZ_TEST(!r.CanAppend());

    for (ezUInt32 i = 0; i < 16; ++i)
      EZ_TEST_INT(r[i], i);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetCount / IsEmpty")
  {
    ezStaticRingBuffer<ezInt32, 16> r;

    EZ_TEST(r.IsEmpty());

    for (ezUInt32 i = 0; i < 16; ++i)
    {
      EZ_TEST_INT(r.GetCount(), i);
      r.PushBack(i);
      EZ_TEST_INT(r.GetCount(), i + 1);

      EZ_TEST(!r.IsEmpty());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Clear / IsEmpty")
  {
    ezStaticRingBuffer<ezInt32, 16> r;

    EZ_TEST(r.IsEmpty());

    for (ezUInt32 i = 0; i < 16; ++i)
      r.PushBack(i);

    EZ_TEST(!r.IsEmpty());

    r.Clear();

    EZ_TEST(r.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Cycle Items / PeekFront")
  {
    EZ_TEST(ezConstructionCounter::HasAllDestructed());

    {
      ezStaticRingBuffer<ezConstructionCounter, 16> r;

      for (ezUInt32 i = 0; i < 16; ++i)
      {
        r.PushBack(ezConstructionCounter(i));
        EZ_TEST(ezConstructionCounter::HasDone(2, 1)); // one temporary
      }

      for (ezUInt32 i = 16; i < 1000; ++i)
      {
        EZ_TEST(r.PeekFront() == ezConstructionCounter(i - 16));
        EZ_TEST(ezConstructionCounter::HasDone(1, 1)); // one temporary

        EZ_TEST(!r.CanAppend());

        r.PopFront();
        EZ_TEST(ezConstructionCounter::HasDone(0, 1));

        EZ_TEST(r.CanAppend());

        r.PushBack(ezConstructionCounter(i));
        EZ_TEST(ezConstructionCounter::HasDone(2, 1)); // one temporary
      }

      for (ezUInt32 i = 1000; i < 1016; ++i)
      {
        EZ_TEST(r.PeekFront() == ezConstructionCounter(i - 16));
        EZ_TEST(ezConstructionCounter::HasDone(1, 1)); // one temporary

        r.PopFront();
        EZ_TEST(ezConstructionCounter::HasDone(0, 1)); // one temporary
      }

      EZ_TEST(r.IsEmpty());
    }

    EZ_TEST(ezConstructionCounter::HasAllDestructed());
  }
}