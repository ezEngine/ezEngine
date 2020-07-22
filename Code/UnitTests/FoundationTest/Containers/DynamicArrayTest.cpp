#include <FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Types/UniquePtr.h>

namespace DynamicArrayTestDetail
{
  typedef ezConstructionCounter st;

  static int g_iDummyCounter = 0;

  class Dummy
  {
  public:
    int a;
    int b;
    std::string s;

    Dummy()
      : a(0)
      , b(g_iDummyCounter++)
      , s("Test")
    {
    }
    Dummy(int a)
      : a(a)
      , b(g_iDummyCounter++)
      , s("Test")
    {
    }

    bool operator<=(const Dummy& dummy) const { return a <= dummy.a; }
    bool operator>=(const Dummy& dummy) const { return a >= dummy.a; }
    bool operator>(const Dummy& dummy) const { return a > dummy.a; }
    bool operator<(const Dummy& dummy) const { return a < dummy.a; }
    bool operator==(const Dummy& dummy) const { return a == dummy.a; }
  };

  ezAllocatorBase* g_pTestAllocator;

  struct ezTestAllocatorWrapper
  {
    static ezAllocatorBase* GetAllocator() { return g_pTestAllocator; }
  };

  template <typename T = st, typename AllocatorWrapper = ezTestAllocatorWrapper>
  static ezDynamicArray<T, AllocatorWrapper> CreateArray(ezUInt32 uiSize, ezUInt32 uiOffset)
  {
    ezDynamicArray<T, AllocatorWrapper> a;
    a.SetCount(uiSize);

    for (ezUInt32 i = 0; i < uiSize; ++i)
      a[i] = T(uiOffset + i);

    return a;
  }
} // namespace DynamicArrayTestDetail

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
EZ_CHECK_AT_COMPILETIME(sizeof(ezDynamicArray<ezInt32>) == 24);
#else
EZ_CHECK_AT_COMPILETIME(sizeof(ezDynamicArray<ezInt32>) == 16);
#endif

EZ_CREATE_SIMPLE_TEST_GROUP(Containers);

EZ_CREATE_SIMPLE_TEST(Containers, DynamicArray)
{
  ezProxyAllocator proxy("DynamicArrayTestAllocator", ezFoundation::GetDefaultAllocator());
  DynamicArrayTestDetail::g_pTestAllocator = &proxy;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezDynamicArray<ezInt32> a1;
    ezDynamicArray<DynamicArrayTestDetail::st> a2;

    EZ_TEST_BOOL(a1.GetCount() == 0);
    EZ_TEST_BOOL(a2.GetCount() == 0);
    EZ_TEST_BOOL(a1.IsEmpty());
    EZ_TEST_BOOL(a2.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy Constructor")
  {
    ezDynamicArray<ezInt32, DynamicArrayTestDetail::ezTestAllocatorWrapper> a1;

    EZ_TEST_BOOL(a1.GetHeapMemoryUsage() == 0);

    for (ezInt32 i = 0; i < 32; ++i)
      a1.PushBack(rand() % 100000);

    EZ_TEST_BOOL(a1.GetHeapMemoryUsage() >= 32 * sizeof(ezInt32));

    ezDynamicArray<ezInt32> a2 = a1;
    ezDynamicArray<ezInt32> a3(a1);

    EZ_TEST_BOOL(a1 == a2);
    EZ_TEST_BOOL(a1 == a3);
    EZ_TEST_BOOL(a2 == a3);

    ezInt32 test[] = {1, 2, 3, 4};
    ezArrayPtr<ezInt32> aptr(test);

    ezDynamicArray<ezInt32> a4(aptr);

    EZ_TEST_BOOL(a4 == aptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Move Constructor / Operator")
  {
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    {
      // move constructor
      ezDynamicArray<DynamicArrayTestDetail::st, DynamicArrayTestDetail::ezTestAllocatorWrapper> a1(DynamicArrayTestDetail::CreateArray(100, 20));

      EZ_TEST_INT(a1.GetCount(), 100);
      for (ezUInt32 i = 0; i < a1.GetCount(); ++i)
        EZ_TEST_INT(a1[i].m_iData, 20 + i);

      // move operator
      a1 = DynamicArrayTestDetail::CreateArray(200, 50);

      EZ_TEST_INT(a1.GetCount(), 200);
      for (ezUInt32 i = 0; i < a1.GetCount(); ++i)
        EZ_TEST_INT(a1[i].m_iData, 50 + i);
    }

    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    {
      // move assignment with different allocators
      ezConstructionCounterRelocatable::Reset();
      ezProxyAllocator proxyAllocator("test allocator", ezFoundation::GetDefaultAllocator());
      {
        ezDynamicArray<ezConstructionCounterRelocatable> a1(&proxyAllocator);

        a1 = DynamicArrayTestDetail::CreateArray<ezConstructionCounterRelocatable, ezDefaultAllocatorWrapper>(8, 70);
        EZ_TEST_BOOL(ezConstructionCounterRelocatable::HasDone(8, 0));
        EZ_TEST_BOOL(a1.GetAllocator() == &proxyAllocator); // allocator must not change

        EZ_TEST_INT(a1.GetCount(), 8);
        for (ezUInt32 i = 0; i < a1.GetCount(); ++i)
          EZ_TEST_INT(a1[i].m_iData, 70 + i);

        a1 = DynamicArrayTestDetail::CreateArray<ezConstructionCounterRelocatable, ezDefaultAllocatorWrapper>(32, 100);
        EZ_TEST_BOOL(ezConstructionCounterRelocatable::HasDone(32, 8));
        EZ_TEST_BOOL(a1.GetAllocator() == &proxyAllocator); // allocator must not change

        EZ_TEST_INT(a1.GetCount(), 32);
        for (ezUInt32 i = 0; i < a1.GetCount(); ++i)
          EZ_TEST_INT(a1[i].m_iData, 100 + i);
      }

      EZ_TEST_BOOL(ezConstructionCounterRelocatable::HasAllDestructed());
      ezConstructionCounterRelocatable::Reset();

      auto allocatorStats = proxyAllocator.GetStats();
      EZ_TEST_BOOL(allocatorStats.m_uiNumAllocations == allocatorStats.m_uiNumDeallocations); // check for memory leak?
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Convert to ArrayPtr")
  {
    ezDynamicArray<ezInt32> a1;

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
    ezDynamicArray<ezInt32, DynamicArrayTestDetail::ezTestAllocatorWrapper> a1;
    ezDynamicArray<ezInt32> a2;

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
    ezDynamicArray<ezInt32> a1, a2;

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
    ezDynamicArray<ezInt32> a1;
    a1.SetCountUninitialized(100);

    for (ezInt32 i = 0; i < 100; ++i)
      a1[i] = i;

    for (ezInt32 i = 0; i < 100; ++i)
      EZ_TEST_INT(a1[i], i);

    ezDynamicArray<ezInt32> ca1;
    ca1 = a1;

    for (ezInt32 i = 0; i < 100; ++i)
      EZ_TEST_INT(ca1[i], i);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetCount / GetCount / IsEmpty")
  {
    ezDynamicArray<ezInt32> a1;

    EZ_TEST_BOOL(a1.IsEmpty());

    for (ezInt32 i = 0; i < 128; ++i)
    {
      a1.SetCount(i + 1);
      EZ_TEST_INT(a1[i], 0);
      a1[i] = i;

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

    a1.SetCountUninitialized(32);
    EZ_TEST_INT(a1.GetCount(), 32);
    a1[31] = 45;
    EZ_TEST_INT(a1[31], 45);

    // Test SetCount with fill value
    {
      ezDynamicArray<ezInt32> a2;
      a2.PushBack(5);
      a2.PushBack(3);
      a2.SetCount(10, 42);

      if (EZ_TEST_INT(a2.GetCount(), 10).Succeeded())
      {
        EZ_TEST_INT(a2[0], 5);
        EZ_TEST_INT(a2[1], 3);
        EZ_TEST_INT(a2[4], 42);
        EZ_TEST_INT(a2[9], 42);
      }

      a2.Clear();
      a2.PushBack(1);
      a2.PushBack(2);
      a2.PushBack(3);

      a2.SetCount(2, 10);
      if (EZ_TEST_INT(a2.GetCount(), 2).Succeeded())
      {
        EZ_TEST_INT(a2[0], 1);
        EZ_TEST_INT(a2[1], 2);
      }
    }
  }

  // Test SetCount with fill value
  {
    ezDynamicArray<ezInt32> a2;
    a2.PushBack(5);
    a2.PushBack(3);
    a2.SetCount(10, 42);

    if (EZ_TEST_INT(a2.GetCount(), 10).Succeeded())
    {
      EZ_TEST_INT(a2[0], 5);
      EZ_TEST_INT(a2[1], 3);
      EZ_TEST_INT(a2[4], 42);
      EZ_TEST_INT(a2[9], 42);
    }

    a2.Clear();
    a2.PushBack(1);
    a2.PushBack(2);
    a2.PushBack(3);

    a2.SetCount(2, 10);
    if (EZ_TEST_INT(a2.GetCount(), 2).Succeeded())
    {
      EZ_TEST_INT(a2[0], 1);
      EZ_TEST_INT(a2[1], 2);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "EnsureCount")
  {
    ezDynamicArray<ezInt32> a1;

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
    ezDynamicArray<ezInt32> a1;
    a1.Clear();

    a1.PushBack(3);
    a1.Clear();

    EZ_TEST_BOOL(a1.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    ezDynamicArray<ezInt32> a1;

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PushBackUnchecked / PushBackRange")
  {
    ezDynamicArray<ezInt32> a1;
    a1.Reserve(100);

    for (ezInt32 i = 0; i < 100; ++i)
      a1.PushBackUnchecked(i);

    for (ezInt32 i = 0; i < 100; ++i)
      EZ_TEST_INT(a1[i], i);

    ezInt32 temp[] = {100, 101, 102, 103, 104};
    ezArrayPtr<ezInt32> range(temp);

    a1.PushBackRange(range);

    EZ_TEST_INT(a1.GetCount(), 105);
    for (ezUInt32 i = 0; i < a1.GetCount(); ++i)
      EZ_TEST_INT(a1[i], i);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Insert")
  {
    ezDynamicArray<ezInt32> a1;

    // always inserts at the front
    for (ezInt32 i = 0; i < 100; ++i)
      a1.Insert(i, 0);

    for (ezInt32 i = 0; i < 100; ++i)
      EZ_TEST_INT(a1[i], 99 - i);

    ezUniquePtr<DynamicArrayTestDetail::st> ptr = EZ_DEFAULT_NEW(DynamicArrayTestDetail::st);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasConstructed(1));

    {
      ezDynamicArray<ezUniquePtr<DynamicArrayTestDetail::st>> a2;
      for (ezUInt32 i = 0; i < 10; ++i)
        a2.Insert(ezUniquePtr<DynamicArrayTestDetail::st>(), 0);

      a2.Insert(std::move(ptr), 0);
      EZ_TEST_BOOL(ptr == nullptr);
      EZ_TEST_BOOL(a2[0] != nullptr);

      for (ezUInt32 i = 1; i < a2.GetCount(); ++i)
        EZ_TEST_BOOL(a2[i] == nullptr);
    }

    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveAndCopy")
  {
    ezDynamicArray<ezInt32> a1;

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
    ezDynamicArray<ezInt32> a1;

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
    ezDynamicArray<ezInt32> a1;

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

    ezUniquePtr<DynamicArrayTestDetail::st> ptr = EZ_DEFAULT_NEW(DynamicArrayTestDetail::st);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasConstructed(1));

    {
      ezDynamicArray<ezUniquePtr<DynamicArrayTestDetail::st>> a2;
      for (ezUInt32 i = 0; i < 10; ++i)
        a2.Insert(ezUniquePtr<DynamicArrayTestDetail::st>(), 0);

      a2.PushBack(std::move(ptr));
      EZ_TEST_BOOL(ptr == nullptr);
      EZ_TEST_BOOL(a2[10] != nullptr);

      a2.RemoveAtAndCopy(0);
      EZ_TEST_BOOL(a2[9] != nullptr);
      EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0));
    }

    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveAtAndSwap")
  {
    ezDynamicArray<ezInt32> a1;

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

    ezUniquePtr<DynamicArrayTestDetail::st> ptr = EZ_DEFAULT_NEW(DynamicArrayTestDetail::st);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasConstructed(1));

    {
      ezDynamicArray<ezUniquePtr<DynamicArrayTestDetail::st>> a2;
      for (ezUInt32 i = 0; i < 10; ++i)
        a2.Insert(ezUniquePtr<DynamicArrayTestDetail::st>(), 0);

      a2.PushBack(std::move(ptr));
      EZ_TEST_BOOL(ptr == nullptr);
      EZ_TEST_BOOL(a2[10] != nullptr);

      a2.RemoveAtAndSwap(0);
      EZ_TEST_BOOL(a2[0] != nullptr);
    }

    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PushBack / PopBack / PeekBack")
  {
    ezDynamicArray<ezInt32> a1;

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandAndGetRef")
  {
    ezDynamicArray<ezInt32> a1;

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Construction / Destruction")
  {
    {
      EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

      ezDynamicArray<DynamicArrayTestDetail::st> a1;
      ezDynamicArray<DynamicArrayTestDetail::st> a2;

      EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
      EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

      a1.PushBack(DynamicArrayTestDetail::st(1));
      EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a1.Insert(DynamicArrayTestDetail::st(2), 0);
      EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a2 = a1;
      EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(2, 0)); // two copies

      a1.Clear();
      EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 2));

      a1.PushBack(DynamicArrayTestDetail::st(3));
      a1.PushBack(DynamicArrayTestDetail::st(4));
      a1.PushBack(DynamicArrayTestDetail::st(5));
      a1.PushBack(DynamicArrayTestDetail::st(6));

      EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(8, 4)); // four temporaries

      a1.RemoveAndCopy(DynamicArrayTestDetail::st(3));
      EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(1, 2)); // one temporary, one destroyed

      a1.RemoveAndCopy(DynamicArrayTestDetail::st(3));
      EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(1, 1)); // one temporary, none destroyed

      a1.RemoveAtAndCopy(0);
      EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 1)); // one destroyed

      a1.RemoveAtAndSwap(0);
      EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 1)); // one destroyed
    }

    // tests the destructor of a2 and a1
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SortingPrimitives")
  {
    ezDynamicArray<ezUInt32> list;

    list.Sort();

    for (ezUInt32 i = 0; i < 450; i++)
    {
      list.PushBack(std::rand());
    }
    list.Sort();

    for (ezUInt32 i = 1; i < list.GetCount(); i++)
    {
      EZ_TEST_BOOL(list[i - 1] <= list[i]);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SortingObjects")
  {
    ezDynamicArray<DynamicArrayTestDetail::Dummy> list;
    list.Reserve(128);

    for (ezUInt32 i = 0; i < 100; i++)
    {
      list.PushBack(DynamicArrayTestDetail::Dummy(rand()));
    }
    list.Sort();

    for (ezUInt32 i = 1; i < list.GetCount(); i++)
    {
      EZ_TEST_BOOL(list[i - 1] <= list[i]);
      EZ_TEST_BOOL(list[i].s == "Test");
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SortingMovableObjects")
  {
    {
      ezDynamicArray<ezUniquePtr<DynamicArrayTestDetail::st>> list;
      list.Reserve(128);

      for (ezUInt32 i = 0; i < 100; i++)
      {
        list.PushBack(EZ_DEFAULT_NEW(DynamicArrayTestDetail::st));
      }
      list.Sort();

      for (ezUInt32 i = 1; i < list.GetCount(); i++)
      {
        EZ_TEST_BOOL(list[i - 1] <= list[i]);
      }
    }

    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Various")
  {
    ezDynamicArray<DynamicArrayTestDetail::Dummy> list;
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
    DynamicArrayTestDetail::Dummy d = list.PeekBack();
    list.PopBack();
    EZ_TEST_BOOL(d.a == 5);
    EZ_TEST_BOOL(list.GetCount() == 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Assignment")
  {
    ezDynamicArray<DynamicArrayTestDetail::Dummy> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(DynamicArrayTestDetail::Dummy(rand()));
    }

    ezDynamicArray<DynamicArrayTestDetail::Dummy> list2;
    for (int i = 0; i < 8; i++)
    {
      list2.PushBack(DynamicArrayTestDetail::Dummy(rand()));
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
      list2.PushBack(DynamicArrayTestDetail::Dummy(rand()));
    }

    list = list2;
    EZ_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    EZ_TEST_BOOL(list == list2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Count")
  {
    ezDynamicArray<DynamicArrayTestDetail::Dummy> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(DynamicArrayTestDetail::Dummy(rand()));
    }
    list.SetCount(32);
    list.SetCount(4);

    list.Compact();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Reserve")
  {
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    ezDynamicArray<DynamicArrayTestDetail::st> a;

    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    a.Reserve(100);

    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    a.SetCount(10);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(10, 0));

    a.Reserve(100);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0));

    a.SetCount(100);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(90, 0));

    a.Reserve(200);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 100)); // had to copy some elements over

    a.SetCount(200);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compact")
  {
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    ezDynamicArray<DynamicArrayTestDetail::st> a;

    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    a.SetCount(100);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));

    a.SetCount(200);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(200, 100));

    a.SetCount(10);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 190));

    // no reallocations and copying, if the memory is already available
    a.SetCount(200);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(190, 0));

    a.SetCount(10);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 190));

    // now we remove the spare memory
    a.Compact();
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(10, 10));

    // this time the array needs to be relocated, and thus the already present elements need to be copied
    a.SetCount(200);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(200, 10));

    // this does not deallocate memory
    a.Clear();
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 200));

    a.SetCount(100);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));

    // therefore no object relocation
    a.SetCount(200);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));

    a.Clear();
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 200));

    // this will deallocate ALL memory
    EZ_TEST_BOOL(a.GetHeapMemoryUsage() > 0);
    a.Compact();
    EZ_TEST_BOOL(a.GetHeapMemoryUsage() == 0);

    a.SetCount(100);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));

    // this time objects need to be relocated
    a.SetCount(200);
    EZ_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(200, 100));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "STL Iterator")
  {
    ezDynamicArray<ezInt32> a1;

    for (ezInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    // STL sort
    std::sort(begin(a1), end(a1));

    for (ezInt32 i = 1; i < 1000; ++i)
    {
      EZ_TEST_BOOL(a1[i - 1] <= a1[i]);
    }

    // foreach
    ezInt32 prev = 0;
    ezInt32 sum1 = 0;
    for (ezInt32 val : a1)
    {
      EZ_TEST_BOOL(prev <= val);
      prev = val;
      sum1 += val;
    }

    prev = 1000;
    const auto endIt = rend(a1);
    ezInt32 sum2 = 0;
    for (auto it = rbegin(a1); it != endIt; ++it)
    {
      EZ_TEST_BOOL(prev > (*it));
      prev = (*it);
      sum2 += (*it);
    }

    EZ_TEST_BOOL(sum1 == sum2);

    // const array
    const ezDynamicArray<ezInt32>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(begin(a2), end(a2), 400);
    EZ_TEST_BOOL(*lb == a2[400]);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "STL Reverse Iterator")
  {
    ezDynamicArray<ezInt32> a1;

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
    const ezDynamicArray<ezInt32>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(rbegin(a2), rend(a2), 400);
    EZ_TEST_BOOL(*lb == a2[1000 - 400 - 1]);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetArrayPtr")
  {
    ezDynamicArray<ezInt32> a1;
    a1.SetCountUninitialized(10);

    EZ_TEST_BOOL(a1.GetArrayPtr().GetCount() == 10);
    EZ_TEST_BOOL(a1.GetArrayPtr().GetPtr() == a1.GetData());

    const ezDynamicArray<ezInt32>& a1ref = a1;

    EZ_TEST_BOOL(a1ref.GetArrayPtr().GetCount() == 10);
    EZ_TEST_BOOL(a1ref.GetArrayPtr().GetPtr() == a1ref.GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Swap")
  {
    ezDynamicArray<ezInt32> a1, a2;

    ezInt32 content1[] = {1, 2, 3, 4};
    ezInt32 content2[] = {5, 6, 7, 8, 9};

    a1 = ezMakeArrayPtr(content1);
    a2 = ezMakeArrayPtr(content2);

    ezInt32* a1Ptr = a1.GetData();
    ezInt32* a2Ptr = a2.GetData();

    a1.Swap(a2);

    // The pointers should be simply swapped
    EZ_TEST_BOOL(a2Ptr == a1.GetData());
    EZ_TEST_BOOL(a1Ptr == a2.GetData());

    // The data should be swapped
    EZ_TEST_BOOL(a1.GetArrayPtr() == ezMakeArrayPtr(content2));
    EZ_TEST_BOOL(a2.GetArrayPtr() == ezMakeArrayPtr(content1));
  }

#if EZ_ENABLED(EZ_PLATFORM_64BIT)

  // disabled, because this is a very slow test
  EZ_TEST_BLOCK(ezTestBlock::DisabledNoWarning, "Large Allocation")
  {
    const ezUInt32 uiMaxNumElements = 0xFFFFFFFF - 16; // max supported elements due to alignment restrictions

    // this will allocate about 16 GB memory, the pure allocation is really fast
    ezDynamicArray<ezUInt32> byteArray;
    byteArray.SetCountUninitialized(uiMaxNumElements);

    const ezUInt32 uiCheckElements = byteArray.GetCount();
    const ezUInt32 uiSkipElements = 1024;

    // this will touch the memory and thus enforce that it is indeed made available by the OS
    // this takes a while
    for (ezUInt64 i = 0; i < uiCheckElements; i += uiSkipElements)
    {
      const ezUInt32 idx = i & 0xFFFFFFFF;
      byteArray[idx] = idx;
    }

    // check that the assigned values are all correct
    // again, this takes quite a while
    for (ezUInt64 i = 0; i < uiCheckElements; i += uiSkipElements)
    {
      const ezUInt32 idx = i & 0xFFFFFFFF;
      EZ_TEST_INT(byteArray[idx], idx);
    }
  }
#endif
}
