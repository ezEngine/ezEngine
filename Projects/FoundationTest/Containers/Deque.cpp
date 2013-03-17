#include <PCH.h>

typedef ezConstructionCounter st;

EZ_CREATE_SIMPLE_TEST(Containers, Deque)
{
  ezIAllocator::Stats s;
  ezFoundation::GetDefaultAllocator()->GetStats(s);

  EZ_TEST_BLOCK(true, "Fill / Empty (Sawtooth)")
  {
    ezDeque<ezInt32> d;

    ezFoundation::GetDefaultAllocator()->GetStats(s);

    ezUInt32 uiVal = 0;

    for (ezInt32 i = 0; i < 10000; ++i)
      d.PushBack(uiVal++);

    // this is kind of the worst case scenario, as it will deallocate and reallocate chunks in every loop
    // the smaller the chunk size, the more allocations will happen
    for (ezUInt32 s2 = 0; s2 < 10; ++s2)
    {
      for (ezInt32 i = 0; i < 1000; ++i)
        d.PopBack();
      for (ezInt32 i = 0; i < 1000; ++i)
        d.PushBack(uiVal++);
    }
  }

  EZ_TEST_BLOCK(true, "Fill / Empty")
  {
    ezDeque<ezInt32> d;

    ezFoundation::GetDefaultAllocator()->GetStats(s);

    for (ezInt32 i = 0; i < 10000; ++i)
      d.PushBack(i);

    for (ezInt32 i = 0; i < 10000; ++i)
      d.PopFront();
  }

  EZ_TEST_BLOCK(true, "Queue Back")
  {
    ezDeque<ezInt32> d;

    d.PushBack(0);

    ezFoundation::GetDefaultAllocator()->GetStats(s);

    for (ezInt32 i = 0; i < 10000; ++i)
    {
      d.PushBack(i);
      d.PopFront();
    }

    d.Compact();

    for (ezInt32 i = 0; i < 10000; ++i)
    {
      d.PushBack(i);
      d.PopFront();
    }
  }

  ezFoundation::GetDefaultAllocator()->GetStats(s);

  EZ_TEST_BLOCK(true, "Queue Front")
  {
    ezDeque<ezInt32> d;

    d.PushBack(0);

    for (ezInt32 i = 0; i < 10000; ++i)
    {
      d.PushFront(i);
      d.PopBack();

      ezFoundation::GetDefaultAllocator()->GetStats(s);
    }

    d.Compact();

    for (ezInt32 i = 0; i < 10000; ++i)
    {
      d.PushFront(i);
      d.PopBack();

      ezFoundation::GetDefaultAllocator()->GetStats(s);
    }

    ezFoundation::GetDefaultAllocator()->GetStats(s);
  }

  EZ_TEST_BLOCK(true, "POD Types")
  {
    ezDeque<ezInt32> d1;
    d1.SetCount(5120);

    d1.Compact();

    EZ_TEST(d1.GetCount() == 5120);


    d1.SetCount(1);
    d1.Compact();

    d1.Clear();

    d1.Compact();
  }

  ezStartup::ShutdownCore();

  ezStartup::StartupCore();

  EZ_TEST_BLOCK(true, "POD Types")
  {
    ezDeque<ezInt32> d1;
    d1.SetCount(1000);

    EZ_TEST(d1.GetCount() == 1000);
    d1.Clear();
    EZ_TEST(d1.IsEmpty());

    for (ezInt32 i = 1; i < 1000; ++i)
    {
      if (i == 99)
      {
        int urg = 0;
      }

      d1.PushBack(i);

      EZ_TEST(d1.PeekBack() == i);
      EZ_TEST(d1.GetCount() == i);
      EZ_TEST(!d1.IsEmpty());
    }

    d1.Clear();
    EZ_TEST(d1.IsEmpty());

    for (ezInt32 i = 1; i < 1000; ++i)
    {
      d1.PushFront(i);

      EZ_TEST(d1.PeekFront() == i);
      EZ_TEST(d1.GetCount() == i);
      EZ_TEST(!d1.IsEmpty());
    }

    d1.Clear();
    EZ_TEST(d1.IsEmpty());

    for (ezInt32 i = 1; i <= 1000; ++i)
    {
      d1.PushFront(i);
      d1.PushBack(i);

      EZ_TEST(d1.PeekFront() == i);
      EZ_TEST(d1.PeekBack() == i);
      EZ_TEST(d1.GetCount() == i * 2);
      EZ_TEST(!d1.IsEmpty());
    }

    ezDeque<ezInt32> d2;
    d2 = d1;

    for (ezInt32 i = 1000; i >= 1; --i)
    {
      EZ_TEST(d1.PeekFront() == i);
      EZ_TEST(d1.PeekBack() == i);
      EZ_TEST(d1.GetCount() == i * 2);
      EZ_TEST(!d1.IsEmpty());

      d1.PopFront();
      d1.PopBack ();


      EZ_TEST(d2.PeekFront() == i);
      EZ_TEST(d2.PeekBack() == i);
      EZ_TEST(d2.GetCount() == i * 2);
      EZ_TEST(!d2.IsEmpty());

      d2.PopFront();
      d2.PopBack ();
    }
    
  }

  ezStartup::ShutdownCore();

  ezStartup::StartupCore();

  EZ_TEST_BLOCK(true, "Non-POD Types")
  {
    EZ_TEST(st::HasAllDestructed());

    {
      ezDeque<st> v1;
      EZ_TEST(st::HasDone(0, 0));

      {
        v1.PushBack(st (3));
        EZ_TEST(st::HasDone(2, 1));

        v1.PushBack();
        EZ_TEST(st::HasDone(1, 0));

        v1.PopBack ();
        EZ_TEST(st::HasDone(0, 1));
      }
      {
        v1.PushFront(st (3));
        EZ_TEST(st::HasDone(2, 1));

        v1.PushFront();
        EZ_TEST(st::HasDone(1, 0));

        v1.PopFront();
        EZ_TEST(st::HasDone(0, 1));
      }

      EZ_TEST(v1.GetCount() == 2);

      v1.SetCount(12);
      EZ_TEST(st::HasDone(10, 0));

      {
        ezDeque<st> v2;
        EZ_TEST(st::HasDone(0, 0));

        v2 = v1;
        EZ_TEST(st::HasDone(12, 0));

        v2.Clear();
        EZ_TEST(st::HasDone(0, 12));

        ezDeque<st> v3 (v1);
        EZ_TEST(st::HasDone(12, 0));

        ezDeque<st> v4 (v1);
        EZ_TEST(st::HasDone(12, 0));

        v4.SetCount(0);
        EZ_TEST(st::HasDone(0, 12));
      }

      EZ_TEST(st::HasDone(0, 12));
    }

    EZ_TEST(st::HasAllDestructed());
  }

   EZ_TEST_BLOCK(true, "SortingPrimitives")
  {
    ezDeque<ezUInt32> list;

    list.Sort();

    for (ezUInt32 i = 0; i < 245; i++)
    {
      list.PushBack(std::rand());
    }
    list.Sort();

    ezUInt32 last = 0;
    for (ezUInt32 i = 0; i < list.GetCount(); i++)
    {
      EZ_TEST(last <= list[i]);
      last = list[i];
    }
  }
}

