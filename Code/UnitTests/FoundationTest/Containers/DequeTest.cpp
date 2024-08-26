#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/UniquePtr.h>

namespace DequeTestDetail
{
  using st = ezConstructionCounter;

  static ezDeque<st> CreateArray(ezUInt32 uiSize, ezUInt32 uiOffset)
  {
    ezDeque<st> a;
    a.SetCount(uiSize);

    for (ezUInt32 i = 0; i < uiSize; ++i)
      a[i] = uiOffset + i;

    return a;
  }
} // namespace DequeTestDetail

EZ_CREATE_SIMPLE_TEST(Containers, Deque)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Fill / Empty (Sawtooth)")
  {
    ezDeque<ezInt32> d;

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Fill / Empty")
  {
    ezDeque<ezInt32> d;

    EZ_TEST_BOOL(d.GetHeapMemoryUsage() == 0);

    for (ezInt32 i = 0; i < 10000; ++i)
      d.PushBack(i);

    EZ_TEST_BOOL(d.GetHeapMemoryUsage() > 10000 * sizeof(ezInt32));

    for (ezInt32 i = 0; i < 10000; ++i)
      d.PopFront();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Queue Back")
  {
    ezDeque<ezInt32> d;

    d.PushBack(0);

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Queue Front")
  {
    ezDeque<ezInt32> d;

    d.PushBack(0);

    for (ezInt32 i = 0; i < 10000; ++i)
    {
      d.PushFront(i);
      d.PopBack();
    }

    d.Compact();

    for (ezInt32 i = 0; i < 10000; ++i)
    {
      d.PushFront(i);
      d.PopBack();
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "POD Types")
  {
    ezDeque<ezInt32> d1;
    d1.SetCount(5120);

    d1.Compact();

    EZ_TEST_BOOL(d1.GetCount() == 5120);


    d1.SetCount(1);
    d1.Compact();

    EZ_TEST_BOOL(d1.GetHeapMemoryUsage() > 0);

    d1.Clear();

    d1.Compact();

    EZ_TEST_BOOL(d1.GetHeapMemoryUsage() == 0);
  }

  ezStartup::ShutdownCoreSystems();

  ezStartup::StartupCoreSystems();

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "POD Types")
  {
    ezDeque<ezInt32> d1;
    d1.SetCount(1000);

    EZ_TEST_BOOL(d1.GetCount() == 1000);
    d1.Clear();
    EZ_TEST_BOOL(d1.IsEmpty());

    for (ezInt32 i = 1; i < 1000; ++i)
    {
      d1.PushBack(i);

      EZ_TEST_BOOL(d1.PeekBack() == i);
      EZ_TEST_BOOL(d1.GetCount() == i);
      EZ_TEST_BOOL(!d1.IsEmpty());
    }

    d1.Clear();
    EZ_TEST_BOOL(d1.IsEmpty());

    for (ezInt32 i = 1; i < 1000; ++i)
    {
      d1.PushFront(i);

      EZ_TEST_BOOL(d1.PeekFront() == i);
      EZ_TEST_BOOL(d1.GetCount() == i);
      EZ_TEST_BOOL(!d1.IsEmpty());
    }

    d1.Clear();
    EZ_TEST_BOOL(d1.IsEmpty());

    for (ezInt32 i = 1; i <= 1000; ++i)
    {
      d1.PushFront(i);
      d1.PushBack(i);

      EZ_TEST_BOOL(d1.PeekFront() == i);
      EZ_TEST_BOOL(d1.PeekBack() == i);
      EZ_TEST_BOOL(d1.GetCount() == i * 2);
      EZ_TEST_BOOL(!d1.IsEmpty());
    }

    ezDeque<ezInt32> d2;
    d2 = d1;

    for (ezInt32 i = 1000; i >= 1; --i)
    {
      EZ_TEST_BOOL(d1.PeekFront() == i);
      EZ_TEST_BOOL(d1.PeekBack() == i);
      EZ_TEST_BOOL(d1.GetCount() == i * 2);
      EZ_TEST_BOOL(!d1.IsEmpty());

      d1.PopFront();
      d1.PopBack();


      EZ_TEST_BOOL(d2.PeekFront() == i);
      EZ_TEST_BOOL(d2.PeekBack() == i);
      EZ_TEST_BOOL(d2.GetCount() == i * 2);
      EZ_TEST_BOOL(!d2.IsEmpty());

      d2.PopFront();
      d2.PopBack();
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "STL Iterator")
    {
      ezDeque<ezInt32> a1;
      for (ezInt32 i = 0; i < 1000; ++i)
        a1.PushBack(1000 - i - 1);

      // STL sort
      std::sort(begin(a1), end(a1));

      ezUInt32 prev = 0;
      for (ezUInt32 val : a1)
      {
        EZ_TEST_BOOL(prev <= val);
        prev = val;
      }

      // STL lower bound
      auto lb = std::lower_bound(begin(a1), end(a1), 400);
      EZ_TEST_BOOL(*lb == a1[400]);
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "STL Reverse Iterator")
    {
      ezDeque<ezInt32> a1;
      for (ezInt32 i = 0; i < 1000; ++i)
        a1.PushBack(1000 - i - 1);

      std::sort(rbegin(a1), rend(a1));

      // foreach
      ezUInt32 prev = 1000;
      for (ezUInt32 val : a1)
      {
        EZ_TEST_BOOL(prev >= val);
        prev = val;
      }

      // const array
      const ezDeque<ezInt32>& a2 = a1;

      // STL lower bound
      auto lb2 = std::lower_bound(rbegin(a2), rend(a2), 400);
      EZ_TEST_INT(*lb2, a2[1000 - 400 - 1]);
    }
  }

  ezStartup::ShutdownCoreSystems();

  ezStartup::StartupCoreSystems();

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Non-POD Types")
  {
    EZ_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

    {
      ezDeque<DequeTestDetail::st> v1;
      EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0));

      {
        v1.PushBack(DequeTestDetail::st(3));
        EZ_TEST_BOOL(DequeTestDetail::st::HasDone(2, 1));

        v1.PushBack();
        EZ_TEST_BOOL(DequeTestDetail::st::HasDone(1, 0));

        v1.PopBack();
        EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 1));
      }
      {
        v1.PushFront(DequeTestDetail::st(3));
        EZ_TEST_BOOL(DequeTestDetail::st::HasDone(2, 1));

        v1.PushFront();
        EZ_TEST_BOOL(DequeTestDetail::st::HasDone(1, 0));

        v1.PopFront();
        EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 1));
      }

      EZ_TEST_BOOL(v1.GetCount() == 2);

      v1.SetCount(12);
      EZ_TEST_BOOL(DequeTestDetail::st::HasDone(10, 0));

      {
        ezDeque<DequeTestDetail::st> v2;
        EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0));

        v2 = v1;
        EZ_TEST_BOOL(DequeTestDetail::st::HasDone(12, 0));

        v2.Clear();
        EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 12));

        ezDeque<DequeTestDetail::st> v3(v1);
        EZ_TEST_BOOL(DequeTestDetail::st::HasDone(12, 0));

        ezDeque<DequeTestDetail::st> v4(v1);
        EZ_TEST_BOOL(DequeTestDetail::st::HasDone(12, 0));

        v4.SetCount(0);
        EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 12));
      }

      EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 12));
    }

    EZ_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SortingPrimitives")
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
      EZ_TEST_BOOL(last <= list[i]);
      last = list[i];
    }
  }



  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezDeque<ezInt32> a1;
    ezDeque<DequeTestDetail::st> a2;

    EZ_TEST_BOOL(a1.GetCount() == 0);
    EZ_TEST_BOOL(a2.GetCount() == 0);
    EZ_TEST_BOOL(a1.IsEmpty());
    EZ_TEST_BOOL(a2.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy Constructor")
  {
    ezDeque<ezInt32> a1;

    for (ezInt32 i = 0; i < 32; ++i)
      a1.PushBack(rand() % 100000);

    ezDeque<ezInt32> a2 = a1;
    ezDeque<ezInt32> a3(a1);

    EZ_TEST_BOOL(a1.GetCount() == a2.GetCount());
    EZ_TEST_BOOL(a1.GetCount() == a3.GetCount());

    for (ezUInt32 i = 0; i < a1.GetCount(); ++i)
    {
      EZ_TEST_BOOL(a1[i] == a2[i]);
      EZ_TEST_BOOL(a1[i] == a3[i]);
      EZ_TEST_BOOL(a2[i] == a3[i]);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Move Constructor / Operator")
  {
    EZ_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

    {
      // move constructor
      ezDeque<DequeTestDetail::st> a1(DequeTestDetail::CreateArray(100, 20));

      EZ_TEST_INT(a1.GetCount(), 100);
      for (ezUInt32 i = 0; i < a1.GetCount(); ++i)
        EZ_TEST_INT(a1[i].m_iData, 20 + i);

      // move operator
      a1 = DequeTestDetail::CreateArray(200, 50);

      EZ_TEST_INT(a1.GetCount(), 200);
      for (ezUInt32 i = 0; i < a1.GetCount(); ++i)
        EZ_TEST_INT(a1[i].m_iData, 50 + i);
    }

    EZ_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator = / operator == / operator !=")
  {
    ezDeque<ezInt32> a1;
    ezDeque<ezInt32> a2;

    EZ_TEST_BOOL(a1 == a2);

    for (ezInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);

    EZ_TEST_BOOL(a1 != a2);

    a2 = a1;

    EZ_TEST_BOOL(a1.GetCount() == a2.GetCount());

    for (ezUInt32 i = 0; i < a1.GetCount(); ++i)
      EZ_TEST_BOOL(a1[i] == a2[i]);

    EZ_TEST_BOOL(a1 == a2);
    EZ_TEST_BOOL(a2 == a1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Index operator")
  {
    ezDeque<ezInt32> a1;
    a1.SetCount(100);

    for (ezInt32 i = 0; i < 100; ++i)
      a1[i] = i;

    for (ezInt32 i = 0; i < 100; ++i)
      EZ_TEST_INT(a1[i], i);

    ezDeque<ezInt32> ca1;
    ca1 = a1;

    for (ezInt32 i = 0; i < 100; ++i)
      EZ_TEST_INT(ca1[i], i);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetCount / GetCount / IsEmpty")
  {
    ezDeque<ezInt32> a1;

    EZ_TEST_BOOL(a1.IsEmpty());

    for (ezInt32 i = 0; i < 128; ++i)
    {
      a1.SetCount(i + 1);
      EZ_TEST_INT(a1[i], 0); // default init
      a1[i] = i;
      EZ_TEST_INT(a1[i], i);

      EZ_TEST_INT(a1.GetCount(), i + 1);
      EZ_TEST_BOOL(!a1.IsEmpty());
    }

    for (ezInt32 i = 0; i < 128; ++i)
      EZ_TEST_INT(a1[i], i);

    for (ezInt32 i = 128; i >= 0; --i)
    {
      a1.SetCount(i);

      EZ_TEST_INT(a1.GetCount(), i);

      for (ezInt32 i2 = 0; i2 < i; ++i2)
        EZ_TEST_INT(a1[i2], i2);
    }

    EZ_TEST_BOOL(a1.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetCountUninitialized")
  {
    ezDeque<ezInt32> a1;

    EZ_TEST_BOOL(a1.IsEmpty());

    for (ezInt32 i = 0; i < 128; ++i)
    {
      a1.SetCountUninitialized(i + 1);
      // no default init
      a1[i] = i;
      EZ_TEST_INT(a1[i], i);

      EZ_TEST_INT(a1.GetCount(), i + 1);
      EZ_TEST_BOOL(!a1.IsEmpty());
    }

    for (ezInt32 i = 0; i < 128; ++i)
      EZ_TEST_INT(a1[i], i);

    for (ezInt32 i = 128; i >= 0; --i)
    {
      a1.SetCountUninitialized(i);

      EZ_TEST_INT(a1.GetCount(), i);

      for (ezInt32 i2 = 0; i2 < i; ++i2)
        EZ_TEST_INT(a1[i2], i2);
    }

    EZ_TEST_BOOL(a1.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "EnsureCount")
  {
    ezDeque<ezInt32> a1;

    EZ_TEST_INT(a1.GetCount(), 0);

    a1.EnsureCount(0);
    EZ_TEST_INT(a1.GetCount(), 0);

    a1.EnsureCount(1);
    EZ_TEST_INT(a1.GetCount(), 1);

    a1.EnsureCount(2);
    EZ_TEST_INT(a1.GetCount(), 2);

    a1.EnsureCount(1);
    EZ_TEST_INT(a1.GetCount(), 2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Clear")
  {
    ezDeque<ezInt32> a1;
    a1.Clear();

    a1.PushBack(3);
    a1.Clear();

    EZ_TEST_BOOL(a1.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    ezDeque<ezInt32> a1;

    for (ezInt32 i = -100; i < 100; ++i)
      EZ_TEST_BOOL(!a1.Contains(i));

    for (ezInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);

    for (ezInt32 i = 0; i < 100; ++i)
    {
      EZ_TEST_BOOL(a1.Contains(i));
      EZ_TEST_INT(a1.IndexOf(i), i);
      EZ_TEST_INT(a1.LastIndexOf(i), i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Insert")
  {
    ezDeque<ezInt32> a1;

    // always inserts at the front
    for (ezInt32 i = 0; i < 100; ++i)
      a1.InsertAt(0, i);

    for (ezInt32 i = 0; i < 100; ++i)
      EZ_TEST_INT(a1[i], 99 - i);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveAndCopy")
  {
    ezDeque<ezInt32> a1;

    for (ezInt32 i = 0; i < 100; ++i)
      a1.PushBack(i % 2);

    while (a1.RemoveAndCopy(1))
    {
    }

    EZ_TEST_BOOL(a1.GetCount() == 50);

    for (ezUInt32 i = 0; i < a1.GetCount(); ++i)
      EZ_TEST_INT(a1[i], 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveAndSwap")
  {
    ezDeque<ezInt32> a1;

    for (ezInt32 i = 0; i < 10; ++i)
      a1.InsertAt(i, i); // inserts at the end

    a1.RemoveAndSwap(9);
    a1.RemoveAndSwap(7);
    a1.RemoveAndSwap(5);
    a1.RemoveAndSwap(3);
    a1.RemoveAndSwap(1);

    EZ_TEST_INT(a1.GetCount(), 5);

    for (ezInt32 i = 0; i < 5; ++i)
      EZ_TEST_BOOL(ezMath::IsEven(a1[i]));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveAtAndCopy")
  {
    ezDeque<ezInt32> a1;

    for (ezInt32 i = 0; i < 10; ++i)
      a1.InsertAt(i, i); // inserts at the end

    a1.RemoveAtAndCopy(9);
    a1.RemoveAtAndCopy(7);
    a1.RemoveAtAndCopy(5);
    a1.RemoveAtAndCopy(3);
    a1.RemoveAtAndCopy(1);

    EZ_TEST_INT(a1.GetCount(), 5);

    for (ezInt32 i = 0; i < 5; ++i)
      EZ_TEST_INT(a1[i], i * 2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveAtAndSwap")
  {
    ezDeque<ezInt32> a1;

    for (ezInt32 i = 0; i < 10; ++i)
      a1.InsertAt(i, i); // inserts at the end

    a1.RemoveAtAndSwap(9);
    a1.RemoveAtAndSwap(7);
    a1.RemoveAtAndSwap(5);
    a1.RemoveAtAndSwap(3);
    a1.RemoveAtAndSwap(1);

    EZ_TEST_INT(a1.GetCount(), 5);

    for (ezInt32 i = 0; i < 5; ++i)
      EZ_TEST_BOOL(ezMath::IsEven(a1[i]));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandAndGetRef")
  {
    ezDeque<ezInt32> a1;

    for (ezInt32 i = 0; i < 20; ++i)
    {
      ezInt32& intRef = a1.ExpandAndGetRef();
      intRef = i * 5;
    }


    EZ_TEST_BOOL(a1.GetCount() == 20);

    for (ezInt32 i = 0; i < 20; ++i)
    {
      EZ_TEST_INT(a1[i], i * 5);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PushBack / PopBack / PeekBack")
  {
    ezDeque<ezInt32> a1;

    for (ezInt32 i = 0; i < 10; ++i)
    {
      a1.PushBack(i);
      EZ_TEST_INT(a1.PeekBack(), i);
    }

    for (ezInt32 i = 9; i >= 0; --i)
    {
      EZ_TEST_INT(a1.PeekBack(), i);
      a1.PopBack();
    }

    a1.PushBack(23);
    a1.PushBack(2);
    a1.PushBack(3);

    a1.PopBack(2);
    EZ_TEST_INT(a1.PeekBack(), 23);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PushFront / PopFront / PeekFront")
  {
    ezDeque<ezInt32> a1;

    for (ezInt32 i = 0; i < 10; ++i)
    {
      a1.PushFront(i);
      EZ_TEST_INT(a1.PeekFront(), i);
    }

    for (ezInt32 i = 9; i >= 0; --i)
    {
      EZ_TEST_INT(a1.PeekFront(), i);
      a1.PopFront();
    }

    a1.PushFront(23);
    a1.PushFront(2);
    a1.PushFront(3);

    a1.PopFront(2);
    EZ_TEST_INT(a1.PeekFront(), 23);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Construction / Destruction")
  {
    {
      EZ_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

      ezDeque<DequeTestDetail::st> a1;
      ezDeque<DequeTestDetail::st> a2;

      EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
      EZ_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

      a1.PushBack(DequeTestDetail::st(1));
      EZ_TEST_BOOL(DequeTestDetail::st::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a1.InsertAt(0, DequeTestDetail::st(2));
      EZ_TEST_BOOL(DequeTestDetail::st::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a2 = a1;
      EZ_TEST_BOOL(DequeTestDetail::st::HasDone(2, 0)); // two copies

      a1.Clear();
      EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 2));

      a1.PushBack(DequeTestDetail::st(3));
      a1.PushBack(DequeTestDetail::st(4));
      a1.PushBack(DequeTestDetail::st(5));
      a1.PushBack(DequeTestDetail::st(6));

      EZ_TEST_BOOL(DequeTestDetail::st::HasDone(8, 4)); // four temporaries

      a1.RemoveAndCopy(DequeTestDetail::st(3));
      EZ_TEST_BOOL(DequeTestDetail::st::HasDone(1, 2)); // one temporary, one destroyed

      a1.RemoveAndCopy(DequeTestDetail::st(3));
      EZ_TEST_BOOL(DequeTestDetail::st::HasDone(1, 1)); // one temporary, none destroyed

      a1.RemoveAtAndCopy(0);
      EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 1)); // one destroyed

      a1.RemoveAtAndSwap(0);
      EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 1)); // one destroyed
    }

    // tests the destructor of a2 and a1
    EZ_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Reserve")
  {
    EZ_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

    ezDeque<DequeTestDetail::st> a;

    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    EZ_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

    a.Reserve(100);

    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    EZ_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

    a.SetCount(10);
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(10, 0));

    a.Reserve(100);
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0));

    a.SetCount(100);
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(90, 0));

    a.Reserve(200);
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0)); // nothing had to be copied over

    a.SetCount(200);
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(100, 0));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compact")
  {
    EZ_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

    ezDeque<DequeTestDetail::st> a;

    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    EZ_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

    a.SetCount(100);
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(100, 0));

    a.SetCount(200);
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(100, 0));

    a.SetCount(10);
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 190));

    // no reallocations and copying, if the memory is already available
    a.SetCount(200);
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(190, 0));

    a.SetCount(10);
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 190));

    // now we remove the spare memory
    a.Compact();
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0));

    // this time the array needs to be relocated, and thus the already present elements need to be copied
    a.SetCount(200);
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(190, 0));

    // this does not deallocate memory
    a.Clear();
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 200));

    a.SetCount(100);
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(100, 0));

    // therefore no object relocation
    a.SetCount(200);
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(100, 0));

    a.Clear();
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(0, 200));

    // this will deallocate ALL memory
    a.Compact();

    a.SetCount(100);
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(100, 0));

    // this time objects need to be relocated
    a.SetCount(200);
    EZ_TEST_BOOL(DequeTestDetail::st::HasDone(100, 0));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetContiguousRange")
  {
    // deques allocate data in 4 KB chunks, so an integer deque will have 1024 ints per chunk

    ezDeque<ezInt32> d;

    for (ezUInt32 i = 0; i < 100 * 1024; ++i)
      d.PushBack(i);

    ezDynamicArray<ezInt32> a;
    a.SetCountUninitialized(d.GetCount());

    ezUInt32 uiArrayPos = 0;

    for (ezUInt32 i = 0; i < 100; ++i)
    {
      const ezUInt32 uiOffset = i * 1024 + i;

      const ezUInt32 uiRange = d.GetContiguousRange(uiOffset);

      EZ_TEST_INT(uiRange, 1024 - i);

      ezMemoryUtils::Copy(&a[uiArrayPos], &d[uiOffset], uiRange);

      uiArrayPos += uiRange;
    }

    a.SetCountUninitialized(uiArrayPos);

    uiArrayPos = 0;

    for (ezUInt32 i = 0; i < 100; ++i)
    {
      const ezUInt32 uiOffset = i * 1024 + i;
      const ezUInt32 uiRange = 1024 - i;

      for (ezUInt32 r = 0; r < uiRange; ++r)
      {
        EZ_TEST_INT(a[uiArrayPos], uiOffset + r);
        ++uiArrayPos;
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Swap")
  {
    ezDeque<ezInt32> a1, a2;

    ezInt32 content1[] = {1, 2, 3, 4};
    ezInt32 content2[] = {5, 6, 7, 8, 9};
    for (ezInt32 i : content1)
    {
      a1.PushBack(i);
    }
    for (ezInt32 i : content2)
    {
      a2.PushBack(i);
    }

    ezInt32* a1Ptr = &a1[0];
    ezInt32* a2Ptr = &a2[0];

    a1.Swap(a2);

    // The pointers should be simply swapped
    EZ_TEST_BOOL(a2Ptr == &a1[0]);
    EZ_TEST_BOOL(a1Ptr == &a2[0]);

    EZ_TEST_INT(EZ_ARRAY_SIZE(content1), a2.GetCount());
    EZ_TEST_INT(EZ_ARRAY_SIZE(content2), a1.GetCount());

    // The data should be swapped
    for (int i = 0; i < EZ_ARRAY_SIZE(content1); ++i)
    {
      EZ_TEST_INT(content1[i], a2[i]);
    }
    for (int i = 0; i < EZ_ARRAY_SIZE(content2); ++i)
    {
      EZ_TEST_INT(content2[i], a1[i]);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Move PushBack / PushFront")
  {
    ezDeque<ezUniquePtr<ezUInt32>> a1, a2;
    a1.PushBack(ezUniquePtr<ezUInt32>(EZ_DEFAULT_NEW(ezUInt32, 1)));
    a1.PushBack(ezUniquePtr<ezUInt32>(EZ_DEFAULT_NEW(ezUInt32, 2)));

    a2.PushFront(ezUniquePtr<ezUInt32>(EZ_DEFAULT_NEW(ezUInt32, 3)));
    a2.PushFront(ezUniquePtr<ezUInt32>(EZ_DEFAULT_NEW(ezUInt32, 4)));

    a1.Swap(a2);

    EZ_TEST_INT(*a1[0].Borrow(), 4);
    EZ_TEST_INT(*a1[1].Borrow(), 3);

    EZ_TEST_INT(*a2[0].Borrow(), 1);
    EZ_TEST_INT(*a2[1].Borrow(), 2);
  }
}
