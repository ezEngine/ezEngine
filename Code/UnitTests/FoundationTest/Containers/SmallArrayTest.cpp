#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>

namespace SmallArrayTestDetail
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

  class NonMovableClass
  {
  public:
    NonMovableClass(int iVal)
    {
      m_val = iVal;
      m_pVal = &m_val;
    }

    NonMovableClass(const NonMovableClass& other)
    {
      m_val = other.m_val;
      m_pVal = &m_val;
    }

    void operator=(const NonMovableClass& other) { m_val = other.m_val; }

    int m_val = 0;
    int* m_pVal = nullptr;
  };

  template <typename T>
  static ezSmallArray<T, 16> CreateArray(ezUInt32 uiSize, ezUInt32 uiOffset, ezUInt32 uiUserData)
  {
    ezSmallArray<T, 16> a;
    a.SetCount(static_cast<ezUInt16>(uiSize));

    for (ezUInt32 i = 0; i < uiSize; ++i)
    {
      a[i] = T(uiOffset + i);
    }

    a.template GetUserData<ezUInt32>() = uiUserData;

    return a;
  }

  struct ExternalCounter
  {
    EZ_DECLARE_MEM_RELOCATABLE_TYPE();

    ExternalCounter() = default;

    ExternalCounter(int& ref_iCounter)
      : m_counter{&ref_iCounter}
    {
    }

    ~ExternalCounter()
    {
      if (m_counter)
        (*m_counter)++;
    }

    int* m_counter{};
  };
} // namespace SmallArrayTestDetail

static void TakesDynamicArray(ezDynamicArray<int>& ref_ar, int iNum, int iStart);

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
static_assert(sizeof(ezSmallArray<ezInt32, 1>) == 16);
#else
static_assert(sizeof(ezSmallArray<ezInt32, 1>) == 12);
#endif

static_assert(ezGetTypeClass<ezSmallArray<ezInt32, 1>>::value == ezTypeIsMemRelocatable::value);
static_assert(ezGetTypeClass<ezSmallArray<SmallArrayTestDetail::NonMovableClass, 1>>::value == ezTypeIsClass::value);

EZ_CREATE_SIMPLE_TEST(Containers, SmallArray)
{
  ezConstructionCounter::Reset();

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezSmallArray<ezInt32, 16> a1;
    ezSmallArray<ezConstructionCounter, 16> a2;

    EZ_TEST_BOOL(a1.GetCount() == 0);
    EZ_TEST_BOOL(a2.GetCount() == 0);
    EZ_TEST_BOOL(a1.IsEmpty());
    EZ_TEST_BOOL(a2.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy Constructor")
  {
    ezSmallArray<ezInt32, 16> a1;

    EZ_TEST_BOOL(a1.GetHeapMemoryUsage() == 0);

    for (ezInt32 i = 0; i < 32; ++i)
    {
      a1.PushBack(rand() % 100000);

      if (i < 16)
      {
        EZ_TEST_BOOL(a1.GetHeapMemoryUsage() == 0);
      }
      else
      {
        EZ_TEST_BOOL(a1.GetHeapMemoryUsage() >= i * sizeof(ezInt32));
      }
    }

    a1.GetUserData<ezUInt32>() = 11;

    ezSmallArray<ezInt32, 16> a2 = a1;
    ezSmallArray<ezInt32, 16> a3(a1);

    EZ_TEST_BOOL(a1 == a2);
    EZ_TEST_BOOL(a1 == a3);
    EZ_TEST_BOOL(a2 == a3);

    EZ_TEST_INT(a2.GetUserData<ezUInt32>(), 11);
    EZ_TEST_INT(a3.GetUserData<ezUInt32>(), 11);

    ezInt32 test[] = {1, 2, 3, 4};
    ezArrayPtr<ezInt32> aptr(test);

    ezSmallArray<ezInt32, 16> a4(aptr);

    EZ_TEST_BOOL(a4 == aptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Move Constructor / Operator")
  {
    EZ_TEST_BOOL(ezConstructionCounter::HasAllDestructed());

    {
      // move constructor external storage
      ezSmallArray<ezConstructionCounter, 16> a1(SmallArrayTestDetail::CreateArray<ezConstructionCounter>(100, 20, 11));

      EZ_TEST_INT(a1.GetCount(), 100);
      for (ezUInt32 i = 0; i < a1.GetCount(); ++i)
        EZ_TEST_INT(a1[i].m_iData, 20 + i);

      EZ_TEST_INT(a1.GetUserData<ezUInt32>(), 11);

      // move operator external storage
      a1 = SmallArrayTestDetail::CreateArray<ezConstructionCounter>(200, 50, 22);

      EZ_TEST_INT(a1.GetCount(), 200);
      for (ezUInt32 i = 0; i < a1.GetCount(); ++i)
        EZ_TEST_INT(a1[i].m_iData, 50 + i);

      EZ_TEST_INT(a1.GetUserData<ezUInt32>(), 22);
    }

    EZ_TEST_BOOL(ezConstructionCounter::HasAllDestructed());
    ezConstructionCounter::Reset();

    {
      // move constructor internal storage
      ezSmallArray<ezConstructionCounter, 16> a2(SmallArrayTestDetail::CreateArray<ezConstructionCounter>(10, 30, 11));

      EZ_TEST_INT(a2.GetCount(), 10);
      for (ezUInt32 i = 0; i < a2.GetCount(); ++i)
        EZ_TEST_INT(a2[i].m_iData, 30 + i);

      EZ_TEST_INT(a2.GetUserData<ezUInt32>(), 11);

      // move operator internal storage
      a2 = SmallArrayTestDetail::CreateArray<ezConstructionCounter>(8, 70, 22);

      EZ_TEST_INT(a2.GetCount(), 8);
      for (ezUInt32 i = 0; i < a2.GetCount(); ++i)
        EZ_TEST_INT(a2[i].m_iData, 70 + i);

      EZ_TEST_INT(a2.GetUserData<ezUInt32>(), 22);
    }

    EZ_TEST_BOOL(ezConstructionCounter::HasAllDestructed());
    ezConstructionCounter::Reset();

    ezConstructionCounterRelocatable::Reset();
    {
      // move constructor external storage relocatable
      ezSmallArray<ezConstructionCounterRelocatable, 16> a1(SmallArrayTestDetail::CreateArray<ezConstructionCounterRelocatable>(100, 20, 11));

      EZ_TEST_BOOL(ezConstructionCounterRelocatable::HasDone(100, 0));

      EZ_TEST_INT(a1.GetCount(), 100);
      for (ezUInt32 i = 0; i < a1.GetCount(); ++i)
        EZ_TEST_INT(a1[i].m_iData, 20 + i);

      EZ_TEST_INT(a1.GetUserData<ezUInt32>(), 11);

      // move operator external storage
      a1 = SmallArrayTestDetail::CreateArray<ezConstructionCounterRelocatable>(200, 50, 22);
      EZ_TEST_BOOL(ezConstructionCounterRelocatable::HasDone(200, 100));

      EZ_TEST_INT(a1.GetCount(), 200);
      for (ezUInt32 i = 0; i < a1.GetCount(); ++i)
        EZ_TEST_INT(a1[i].m_iData, 50 + i);

      EZ_TEST_INT(a1.GetUserData<ezUInt32>(), 22);
    }

    EZ_TEST_BOOL(ezConstructionCounterRelocatable::HasAllDestructed());
    ezConstructionCounterRelocatable::Reset();

    {
      // move constructor internal storage relocatable
      ezSmallArray<ezConstructionCounterRelocatable, 16> a2(SmallArrayTestDetail::CreateArray<ezConstructionCounterRelocatable>(10, 30, 11));
      EZ_TEST_BOOL(ezConstructionCounterRelocatable::HasDone(10, 0));

      EZ_TEST_INT(a2.GetCount(), 10);
      for (ezUInt32 i = 0; i < a2.GetCount(); ++i)
        EZ_TEST_INT(a2[i].m_iData, 30 + i);

      EZ_TEST_INT(a2.GetUserData<ezUInt32>(), 11);

      // move operator internal storage
      a2 = SmallArrayTestDetail::CreateArray<ezConstructionCounterRelocatable>(8, 70, 22);
      EZ_TEST_BOOL(ezConstructionCounterRelocatable::HasDone(8, 10));

      EZ_TEST_INT(a2.GetCount(), 8);
      for (ezUInt32 i = 0; i < a2.GetCount(); ++i)
        EZ_TEST_INT(a2[i].m_iData, 70 + i);

      EZ_TEST_INT(a2.GetUserData<ezUInt32>(), 22);
    }

    EZ_TEST_BOOL(ezConstructionCounterRelocatable::HasAllDestructed());
    ezConstructionCounterRelocatable::Reset();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Convert to ArrayPtr")
  {
    ezSmallArray<ezInt32, 16> a1;

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
    ezSmallArray<ezInt32, 16> a1, a2;

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
    ezSmallArray<ezInt32, 16> a1, a2;

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

    EZ_TEST_BOOL((a1 < a2) == false);
    a2.PushBack(100);
    EZ_TEST_BOOL(a1 < a2);
    a1.PushBack(99);
    EZ_TEST_BOOL(a1 < a2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Index operator")
  {
    ezSmallArray<ezInt32, 16> a1;
    a1.SetCountUninitialized(100);

    for (ezInt32 i = 0; i < 100; ++i)
      a1[i] = i;

    for (ezInt32 i = 0; i < 100; ++i)
      EZ_TEST_INT(a1[i], i);

    const ezSmallArray<ezInt32, 16> ca1 = a1;

    for (ezInt32 i = 0; i < 100; ++i)
      EZ_TEST_INT(ca1[i], i);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetCount / GetCount / IsEmpty")
  {
    ezSmallArray<ezInt32, 16> a1;

    EZ_TEST_BOOL(a1.IsEmpty());

    for (ezInt32 i = 0; i < 128; ++i)
    {
      a1.SetCount(static_cast<ezUInt16>(i + 1));
      EZ_TEST_INT(a1[i], 0);
      a1[i] = i;

      EZ_TEST_INT(a1.GetCount(), i + 1);
      EZ_TEST_BOOL(!a1.IsEmpty());
    }

    for (ezInt32 i = 0; i < 128; ++i)
      EZ_TEST_INT(a1[i], i);

    for (ezInt32 i = 128; i >= 0; --i)
    {
      a1.SetCount(static_cast<ezUInt16>(i));

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
      ezSmallArray<ezInt32, 2> a2;
      a2.PushBack(5);
      a2.PushBack(3);
      a2.SetCount(10, 42);

      if (EZ_TEST_INT(a2.GetCount(), 10))
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
      if (EZ_TEST_INT(a2.GetCount(), 2))
      {
        EZ_TEST_INT(a2[0], 1);
        EZ_TEST_INT(a2[1], 2);
      }
    }
  }

  // Test SetCount with fill value
  {
    ezSmallArray<ezInt32, 2> a2;
    a2.PushBack(5);
    a2.PushBack(3);
    a2.SetCount(10, 42);

    if (EZ_TEST_INT(a2.GetCount(), 10))
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
    if (EZ_TEST_INT(a2.GetCount(), 2))
    {
      EZ_TEST_INT(a2[0], 1);
      EZ_TEST_INT(a2[1], 2);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Clear")
  {
    ezSmallArray<ezInt32, 16> a1;
    a1.Clear();

    a1.PushBack(3);
    a1.Clear();

    EZ_TEST_BOOL(a1.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    ezSmallArray<ezInt32, 16> a1;

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
    ezSmallArray<ezInt32, 16> a1;

    // always inserts at the front
    for (ezInt32 i = 0; i < 100; ++i)
      a1.InsertAt(0, i);

    for (ezInt32 i = 0; i < 100; ++i)
      EZ_TEST_INT(a1[i], 99 - i);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveAndCopy")
  {
    ezSmallArray<ezInt32, 16> a1;

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
    ezSmallArray<ezInt32, 16> a1;

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
    ezSmallArray<ezInt32, 16> a1;

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
    ezSmallArray<ezInt32, 16> a1;

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PushBack / PopBack / PeekBack")
  {
    ezSmallArray<ezInt32, 16> a1;

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
    ezSmallArray<ezInt32, 16> a1;

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
      EZ_TEST_BOOL(ezConstructionCounter::HasAllDestructed());

      ezSmallArray<ezConstructionCounter, 16> a1;
      ezSmallArray<ezConstructionCounter, 16> a2;

      EZ_TEST_BOOL(ezConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
      EZ_TEST_BOOL(ezConstructionCounter::HasAllDestructed());

      a1.PushBack(ezConstructionCounter(1));
      EZ_TEST_BOOL(ezConstructionCounter::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a1.InsertAt(0, ezConstructionCounter(2));
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compact")
  {
    ezSmallArray<ezInt32, 16> a;

    for (ezInt32 i = 0; i < 1008; ++i)
    {
      a.PushBack(i);
      EZ_TEST_INT(a.GetCount(), i + 1);
    }

    EZ_TEST_BOOL(a.GetHeapMemoryUsage() > 0);
    a.Compact();
    EZ_TEST_BOOL(a.GetHeapMemoryUsage() > 0);

    for (ezInt32 i = 0; i < 1008; ++i)
      EZ_TEST_INT(a[i], i);

    // this tests whether the static array is reused properly
    a.SetCount(15);
    a.Compact();
    EZ_TEST_BOOL(a.GetHeapMemoryUsage() == 0);

    for (ezInt32 i = 0; i < 15; ++i)
      EZ_TEST_INT(a[i], i);

    a.Clear();
    a.Compact();
    EZ_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SortingPrimitives")
  {
    ezSmallArray<ezUInt32, 16> list;

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
    ezSmallArray<SmallArrayTestDetail::Dummy, 16> list;
    list.Reserve(128);

    for (ezUInt32 i = 0; i < 100; i++)
    {
      list.PushBack(SmallArrayTestDetail::Dummy(rand()));
    }
    list.Sort();

    SmallArrayTestDetail::Dummy last = 0;
    for (ezUInt32 i = 0; i < list.GetCount(); i++)
    {
      EZ_TEST_BOOL(last <= list[i]);
      last = list[i];
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Various")
  {
    ezSmallArray<SmallArrayTestDetail::Dummy, 16> list;
    list.PushBack(1);
    list.PushBack(2);
    list.PushBack(3);
    list.InsertAt(3, 4);
    list.InsertAt(1, 0);
    list.InsertAt(5, 0);

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
    SmallArrayTestDetail::Dummy d = list.PeekBack();
    list.PopBack();
    EZ_TEST_BOOL(d.a == 5);
    EZ_TEST_BOOL(list.GetCount() == 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Assignment")
  {
    ezSmallArray<SmallArrayTestDetail::Dummy, 16> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(SmallArrayTestDetail::Dummy(rand()));
    }
    list.GetUserData<ezUInt32>() = 11;

    ezSmallArray<SmallArrayTestDetail::Dummy, 16> list2;
    for (int i = 0; i < 8; i++)
    {
      list2.PushBack(SmallArrayTestDetail::Dummy(rand()));
    }
    list2.GetUserData<ezUInt32>() = 22;

    list = list2;
    EZ_TEST_INT(list.GetCount(), list2.GetCount());
    EZ_TEST_INT(list.GetUserData<ezUInt32>(), list2.GetUserData<ezUInt32>());

    list2.Clear();
    EZ_TEST_BOOL(list2.GetCount() == 0);

    list2 = list;
    EZ_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    EZ_TEST_BOOL(list == list2);

    for (int i = 0; i < 16; i++)
    {
      list2.PushBack(SmallArrayTestDetail::Dummy(rand()));
    }

    list = list2;
    EZ_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    EZ_TEST_BOOL(list == list2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Count")
  {
    ezSmallArray<SmallArrayTestDetail::Dummy, 16> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(SmallArrayTestDetail::Dummy(rand()));
    }
    list.SetCount(32);
    list.SetCount(4);

    list.Compact();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Reserve")
  {
    EZ_TEST_BOOL(ezConstructionCounter::HasAllDestructed());

    ezSmallArray<ezConstructionCounter, 16> a;

    EZ_TEST_BOOL(ezConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
    EZ_TEST_BOOL(ezConstructionCounter::HasAllDestructed());

    a.Reserve(100);

    EZ_TEST_BOOL(ezConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
    EZ_TEST_BOOL(ezConstructionCounter::HasAllDestructed());

    a.SetCount(10);
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(10, 0));

    a.Reserve(100);
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(0, 0));

    a.SetCount(100);
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(90, 0));

    a.Reserve(200);
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(100, 100)); // had to copy some elements over

    a.SetCount(200);
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(100, 0));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compact")
  {
    EZ_TEST_BOOL(ezConstructionCounter::HasAllDestructed());

    ezSmallArray<ezConstructionCounter, 16> a;

    EZ_TEST_BOOL(ezConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
    EZ_TEST_BOOL(ezConstructionCounter::HasAllDestructed());

    a.SetCount(100);
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(100, 0));

    a.SetCount(200);
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(200, 100));

    a.SetCount(10);
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(0, 190));

    // no reallocations and copying, if the memory is already available
    a.SetCount(200);
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(190, 0));

    a.SetCount(10);
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(0, 190));

    // now we remove the spare memory
    a.Compact();
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(10, 10));

    // this time the array needs to be relocated, and thus the already present elements need to be copied
    a.SetCount(200);
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(200, 10));

    // this does not deallocate memory
    a.Clear();
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(0, 200));

    a.SetCount(100);
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(100, 0));

    // therefore no object relocation
    a.SetCount(200);
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(100, 0));

    a.Clear();
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(0, 200));

    // this will deallocate ALL memory
    a.Compact();

    a.SetCount(10);
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(10, 0));

    // this time objects need to be relocated
    a.SetCount(200);
    EZ_TEST_BOOL(ezConstructionCounter::HasDone(200, 10));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "STL Iterator")
  {
    ezSmallArray<ezInt32, 16> a1;

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
    const ezSmallArray<ezInt32, 16>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(begin(a2), end(a2), 400);
    EZ_TEST_BOOL(*lb == a2[400]);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "STL Reverse Iterator")
  {
    ezSmallArray<ezInt32, 16> a1;

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
    const ezSmallArray<ezInt32, 16>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(rbegin(a2), rend(a2), 400);
    EZ_TEST_BOOL(*lb == a2[1000 - 400 - 1]);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Move")
  {
    int counter = 0;
    {
      ezSmallArray<SmallArrayTestDetail::ExternalCounter, 2> a, b;
      EZ_TEST_BOOL(counter == 0);

      a.PushBack(SmallArrayTestDetail::ExternalCounter(counter));
      EZ_TEST_BOOL(counter == 1);

      b = std::move(a);
      EZ_TEST_BOOL(counter == 1);
    }
    EZ_TEST_BOOL(counter == 2);

    counter = 0;
    {
      ezSmallArray<SmallArrayTestDetail::ExternalCounter, 2> a, b;
      EZ_TEST_BOOL(counter == 0);

      a.PushBack(SmallArrayTestDetail::ExternalCounter(counter));
      a.PushBack(SmallArrayTestDetail::ExternalCounter(counter));
      a.PushBack(SmallArrayTestDetail::ExternalCounter(counter));
      a.PushBack(SmallArrayTestDetail::ExternalCounter(counter));
      EZ_TEST_BOOL(counter == 4);

      b = std::move(a);
      EZ_TEST_BOOL(counter == 4);
    }
    EZ_TEST_BOOL(counter == 8);
  }
}
