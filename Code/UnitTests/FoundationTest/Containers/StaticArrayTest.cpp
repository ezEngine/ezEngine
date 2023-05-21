#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/StaticArray.h>

namespace StaticArrayTestDetail
{
  class Dummy
  {
  public:
    int a;
    std::string s;

    Dummy()
      : a(0)
      , s("Test")
    {
    }
    Dummy(int a)
      : a(a)
      , s("Test")
    {
    }
    Dummy(const Dummy& other)

      = default;
    ~Dummy() = default;

    Dummy& operator=(const Dummy& other) = default;

    bool operator<=(const Dummy& dummy) const { return a <= dummy.a; }
    bool operator>=(const Dummy& dummy) const { return a >= dummy.a; }
    bool operator>(const Dummy& dummy) const { return a > dummy.a; }
    bool operator<(const Dummy& dummy) const { return a < dummy.a; }
    bool operator==(const Dummy& dummy) const { return a == dummy.a; }
  };
} // namespace StaticArrayTestDetail

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
static_assert(sizeof(ezStaticArray<ezInt32, 1>) == 24);
#else
static_assert(sizeof(ezStaticArray<ezInt32, 1>) == 16);
#endif

static_assert(ezGetTypeClass<ezStaticArray<ezInt32, 1>>::value == ezTypeIsMemRelocatable::value);
static_assert(ezGetTypeClass<ezStaticArray<StaticArrayTestDetail::Dummy, 1>>::value == ezTypeIsClass::value);

EZ_CREATE_SIMPLE_TEST(Containers, StaticArray)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezStaticArray<ezInt32, 32> a1;
    ezStaticArray<ezConstructionCounter, 32> a2;

    EZ_TEST_BOOL(a1.GetCount() == 0);
    EZ_TEST_BOOL(a2.GetCount() == 0);
    EZ_TEST_BOOL(a1.IsEmpty());
    EZ_TEST_BOOL(a2.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy Constructor")
  {
    ezStaticArray<ezInt32, 32> a1;

    for (ezInt32 i = 0; i < 32; ++i)
      a1.PushBack(rand() % 100000);

    ezStaticArray<ezInt32, 64> a2 = a1;
    ezStaticArray<ezInt32, 32> a3(a1);

    EZ_TEST_BOOL(a1 == a2);
    EZ_TEST_BOOL(a1 == a3);
    EZ_TEST_BOOL(a2 == a3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Convert to ArrayPtr")
  {
    ezStaticArray<ezInt32, 128> a1;

    for (ezInt32 i = 0; i < 100; ++i)
    {
      ezInt32 r = rand() % 100000;
      a1.PushBack(r);
    }

    ezArrayPtr<ezInt32> ap = a1;

    EZ_TEST_BOOL(ap.GetCount() == a1.GetCount());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator =")
  {
    ezStaticArray<ezInt32, 128> a1, a2;

    for (ezInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);

    a2 = a1;

    EZ_TEST_BOOL(a1 == a2);

    ezArrayPtr<ezInt32> arrayPtr(a1);

    a2 = arrayPtr;

    EZ_TEST_BOOL(a2 == arrayPtr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator == / !=")
  {
    ezStaticArray<ezInt32, 128> a1, a2;

    EZ_TEST_BOOL(a1 == a1);
    EZ_TEST_BOOL(a2 == a2);
    EZ_TEST_BOOL(a1 == a2);

    EZ_TEST_BOOL((a1 != a1) == false);
    EZ_TEST_BOOL((a2 != a2) == false);
    EZ_TEST_BOOL((a1 != a2) == false);

    for (ezInt32 i = 0; i < 100; ++i)
    {
      ezInt32 r = rand() % 100000;
      a1.PushBack(r);
      a2.PushBack(r);
    }

    EZ_TEST_BOOL(a1 == a1);
    EZ_TEST_BOOL(a2 == a2);
    EZ_TEST_BOOL(a1 == a2);

    EZ_TEST_BOOL((a1 != a2) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Index operator")
  {
    ezStaticArray<ezInt32, 128> a1;
    a1.SetCountUninitialized(100);

    for (ezInt32 i = 0; i < 100; ++i)
      a1[i] = i;

    for (ezInt32 i = 0; i < 100; ++i)
      EZ_TEST_INT(a1[i], i);

    const ezStaticArray<ezInt32, 128> ca1 = a1;

    for (ezInt32 i = 0; i < 100; ++i)
      EZ_TEST_INT(ca1[i], i);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetCount / GetCount / IsEmpty")
  {
    ezStaticArray<ezInt32, 128> a1;

    EZ_TEST_BOOL(a1.IsEmpty());

    for (ezInt32 i = 0; i < 128; ++i)
    {
      a1.SetCount(i + 1);
      EZ_TEST_INT(a1[i], 0);
      a1[i] = i;

      EZ_TEST_INT((int)a1.GetCount(), i + 1);
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

    a1.SetCountUninitialized(32);
    EZ_TEST_INT(a1.GetCount(), 32);
    a1[31] = 45;
    EZ_TEST_INT(a1[31], 45);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Clear")
  {
    ezStaticArray<ezInt32, 128> a1;
    a1.Clear();

    a1.PushBack(3);
    a1.Clear();

    EZ_TEST_BOOL(a1.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    ezStaticArray<ezInt32, 128> a1;

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
    ezStaticArray<ezInt32, 128> a1;

    // always inserts at the front
    for (ezInt32 i = 0; i < 100; ++i)
      a1.Insert(i, 0);

    for (ezInt32 i = 0; i < 100; ++i)
      EZ_TEST_INT(a1[i], 99 - i);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveAndCopy")
  {
    ezStaticArray<ezInt32, 128> a1;

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
    ezStaticArray<ezInt32, 128> a1;

    for (ezInt32 i = 0; i < 10; ++i)
      a1.Insert(i, i); // inserts at the end

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
    ezStaticArray<ezInt32, 128> a1;

    for (ezInt32 i = 0; i < 10; ++i)
      a1.Insert(i, i); // inserts at the end

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
    ezStaticArray<ezInt32, 128> a1;

    for (ezInt32 i = 0; i < 10; ++i)
      a1.Insert(i, i); // inserts at the end

    a1.RemoveAtAndSwap(9);
    a1.RemoveAtAndSwap(7);
    a1.RemoveAtAndSwap(5);
    a1.RemoveAtAndSwap(3);
    a1.RemoveAtAndSwap(1);

    EZ_TEST_INT(a1.GetCount(), 5);

    for (ezInt32 i = 0; i < 5; ++i)
      EZ_TEST_BOOL(ezMath::IsEven(a1[i]));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PushBack / PopBack / PeekBack")
  {
    ezStaticArray<ezInt32, 128> a1;

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Construction / Destruction")
  {
    {
      EZ_TEST_BOOL(ezConstructionCounter::HasAllDestructed());

      ezStaticArray<ezConstructionCounter, 128> a1;
      ezStaticArray<ezConstructionCounter, 100> a2;

      EZ_TEST_BOOL(ezConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
      EZ_TEST_BOOL(ezConstructionCounter::HasAllDestructed());

      a1.PushBack(ezConstructionCounter(1));
      EZ_TEST_BOOL(ezConstructionCounter::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a1.Insert(ezConstructionCounter(2), 0);
      EZ_TEST_BOOL(ezConstructionCounter::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a2 = a1;
      EZ_TEST_BOOL(ezConstructionCounter::HasDone(2, 0)); // two copies

      a1.Clear();
      EZ_TEST_BOOL(ezConstructionCounter::HasDone(0, 2));

      a1.PushBack(ezConstructionCounter(3));
      a1.PushBack(ezConstructionCounter(4));
      a1.PushBack(ezConstructionCounter(5));
      a1.PushBack(ezConstructionCounter(6));

      EZ_TEST_BOOL(ezConstructionCounter::HasDone(8, 4)); // four temporaries

      a1.RemoveAndCopy(ezConstructionCounter(3));
      EZ_TEST_BOOL(ezConstructionCounter::HasDone(1, 2)); // one temporary, one destroyed

      a1.RemoveAndCopy(ezConstructionCounter(3));
      EZ_TEST_BOOL(ezConstructionCounter::HasDone(1, 1)); // one temporary, none destroyed

      a1.RemoveAtAndCopy(0);
      EZ_TEST_BOOL(ezConstructionCounter::HasDone(0, 1)); // one destroyed

      a1.RemoveAtAndSwap(0);
      EZ_TEST_BOOL(ezConstructionCounter::HasDone(0, 1)); // one destroyed
    }

    // tests the destructor of a2 and a1
    EZ_TEST_BOOL(ezConstructionCounter::HasAllDestructed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SortingPrimitives")
  {
    ezStaticArray<ezUInt32, 128> list;

    list.Sort();

    for (ezUInt32 i = 0; i < 45; i++)
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SortingObjects")
  {
    ezStaticArray<StaticArrayTestDetail::Dummy, 128> list;

    for (ezUInt32 i = 0; i < 100; i++)
    {
      list.PushBack(StaticArrayTestDetail::Dummy(rand()));
    }
    list.Sort();

    StaticArrayTestDetail::Dummy last = 0;
    for (ezUInt32 i = 0; i < list.GetCount(); i++)
    {
      EZ_TEST_BOOL(last <= list[i]);
      last = list[i];
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Various")
  {
    ezStaticArray<StaticArrayTestDetail::Dummy, 32> list;
    list.PushBack(1);
    list.PushBack(2);
    list.PushBack(3);
    list.Insert(4, 3);
    list.Insert(0, 1);
    list.Insert(0, 5);

    EZ_TEST_BOOL(list[0].a == 1);
    EZ_TEST_BOOL(list[1].a == 0);
    EZ_TEST_BOOL(list[2].a == 2);
    EZ_TEST_BOOL(list[3].a == 3);
    EZ_TEST_BOOL(list[4].a == 4);
    EZ_TEST_BOOL(list[5].a == 0);
    EZ_TEST_BOOL(list.GetCount() == 6);

    list.RemoveAtAndCopy(3);
    list.RemoveAtAndSwap(2);

    EZ_TEST_BOOL(list[0].a == 1);
    EZ_TEST_BOOL(list[1].a == 0);
    EZ_TEST_BOOL(list[2].a == 0);
    EZ_TEST_BOOL(list[3].a == 4);
    EZ_TEST_BOOL(list.GetCount() == 4);
    EZ_TEST_BOOL(list.IndexOf(0) == 1);
    EZ_TEST_BOOL(list.LastIndexOf(0) == 2);

    list.PushBack(5);
    EZ_TEST_BOOL(list[4].a == 5);
    StaticArrayTestDetail::Dummy d = list.PeekBack();
    list.PopBack();
    EZ_TEST_BOOL(d.a == 5);
    EZ_TEST_BOOL(list.GetCount() == 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Assignment")
  {
    ezStaticArray<StaticArrayTestDetail::Dummy, 32> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(StaticArrayTestDetail::Dummy(rand()));
    }

    ezStaticArray<StaticArrayTestDetail::Dummy, 32> list2;
    for (int i = 0; i < 8; i++)
    {
      list2.PushBack(StaticArrayTestDetail::Dummy(rand()));
    }

    list = list2;
    EZ_TEST_BOOL(list.GetCount() == list2.GetCount());

    list2.Clear();
    EZ_TEST_BOOL(list2.GetCount() == 0);

    list2 = list;
    EZ_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    EZ_TEST_BOOL(list == list2);

    for (int i = 0; i < 16; i++)
    {
      list2.PushBack(StaticArrayTestDetail::Dummy(rand()));
    }

    list = list2;
    EZ_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    EZ_TEST_BOOL(list == list2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Count")
  {
    ezStaticArray<StaticArrayTestDetail::Dummy, 32> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(StaticArrayTestDetail::Dummy(rand()));
    }
    list.SetCount(32);
    list.SetCount(4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "STL Iterator")
  {
    ezStaticArray<ezInt32, 1024> a1;

    for (ezInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    // STL sort
    std::sort(begin(a1), end(a1));

    for (ezInt32 i = 1; i < 1000; ++i)
    {
      EZ_TEST_BOOL(a1[i - 1] <= a1[i]);
    }

    // foreach
    ezUInt32 prev = 0;
    for (ezUInt32 val : a1)
    {
      EZ_TEST_BOOL(prev <= val);
      prev = val;
    }

    // const array
    const ezStaticArray<ezInt32, 1024>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(begin(a2), end(a2), 400);
    EZ_TEST_BOOL(*lb == a2[400]);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "STL Reverse Iterator")
  {
    ezStaticArray<ezInt32, 1024> a1;

    for (ezInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    // STL sort
    std::sort(rbegin(a1), rend(a1));

    for (ezInt32 i = 1; i < 1000; ++i)
    {
      EZ_TEST_BOOL(a1[i - 1] >= a1[i]);
    }

    // foreach
    ezUInt32 prev = 1000;
    for (ezUInt32 val : a1)
    {
      EZ_TEST_BOOL(prev >= val);
      prev = val;
    }

    // const array
    const ezStaticArray<ezInt32, 1024>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(rbegin(a2), rend(a2), 400);
    EZ_TEST_BOOL(*lb == a2[1000 - 400 - 1]);
  }
}
