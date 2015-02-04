#include <PCH.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Containers/DynamicArray.h>

typedef ezConstructionCounter st;

namespace
{
  static int g_iDummyCounter = 0;

  class Dummy
  {
  public:
    int a;
    int b;
    std::string s;

    Dummy() : a(0), b(g_iDummyCounter++), s("Test") { }
    Dummy(int a) : a(a), b(g_iDummyCounter++), s("Test") { }
    
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
}

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
  EZ_CHECK_AT_COMPILETIME(sizeof(ezDynamicArray<ezInt32>) == 24);
#else
  EZ_CHECK_AT_COMPILETIME(sizeof(ezDynamicArray<ezInt32>) == 16);
#endif

EZ_CREATE_SIMPLE_TEST_GROUP(Containers);

static ezDynamicArray<st, ezTestAllocatorWrapper> CreateArray(ezUInt32 uiSize, ezUInt32 uiOffset)
{
  ezDynamicArray<st, ezTestAllocatorWrapper> a;
  a.SetCount(uiSize);

  for (ezUInt32 i = 0; i < uiSize; ++i)
    a[i] = uiOffset + i;

  return a;
}

EZ_CREATE_SIMPLE_TEST(Containers, DynamicArray)
{
  ezProxyAllocator proxy("DynamicArrayTestAllocator", ezFoundation::GetDefaultAllocator());
  g_pTestAllocator = &proxy;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezDynamicArray<ezInt32> a1;
    ezDynamicArray<st> a2;

    EZ_TEST_BOOL(a1.GetCount() == 0);
    EZ_TEST_BOOL(a2.GetCount() == 0);
    EZ_TEST_BOOL(a1.IsEmpty());
    EZ_TEST_BOOL(a2.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy Constructor")
  {
    ezDynamicArray<ezInt32, ezTestAllocatorWrapper> a1;

    EZ_TEST_BOOL(a1.GetHeapMemoryUsage() == 0);

    for (ezInt32 i = 0; i < 32; ++i)
      a1.PushBack(rand() % 100000);

    EZ_TEST_BOOL(a1.GetHeapMemoryUsage() >= 32 * sizeof(ezInt32));

    ezDynamicArray<ezInt32> a2 = a1;
    ezDynamicArray<ezInt32> a3 (a1);

    EZ_TEST_BOOL(a1 == a2);
    EZ_TEST_BOOL(a1 == a3);
    EZ_TEST_BOOL(a2 == a3);

    ezInt32 test[] = { 1, 2, 3, 4 };
    ezArrayPtr<ezInt32> aptr(test);

    ezDynamicArray<ezInt32> a4(aptr);

    EZ_TEST_BOOL(a4 == aptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Move Constructor / Operator")
  {
    EZ_TEST_BOOL(st::HasAllDestructed());

    {
      // move constructor
      ezDynamicArray<st, ezTestAllocatorWrapper> a1 (CreateArray(100, 20));

      EZ_TEST_INT(a1.GetCount(), 100);
      for (ezUInt32 i = 0; i < a1.GetCount(); ++i)
        EZ_TEST_INT(a1[i].m_iData, 20 + i);

      // move operator
      a1 = CreateArray(200, 50);

      EZ_TEST_INT(a1.GetCount(), 200);
      for (ezUInt32 i = 0; i < a1.GetCount(); ++i)
        EZ_TEST_INT(a1[i].m_iData, 50 + i);
    }

    EZ_TEST_BOOL(st::HasAllDestructed());
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

    EZ_TEST_BOOL(ap.GetCount () == a1.GetCount());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator =")
  {
    ezDynamicArray<ezInt32, ezTestAllocatorWrapper> a1;
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
    a1.SetCount(100);

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

    ezInt32 temp[] = { 100, 101, 102, 103, 104 };
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
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove")
  {
    ezDynamicArray<ezInt32> a1;

    for (ezInt32 i = 0; i < 100; ++i)
      a1.PushBack(i % 2);

    while (a1.Remove(1))
    {
    }

    EZ_TEST_BOOL(a1.GetCount() == 50);

    for (ezUInt32 i = 0; i < a1.GetCount(); ++i)
      EZ_TEST_INT(a1[i], 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveSwap")
  {
    ezDynamicArray<ezInt32> a1;

    for (ezInt32 i = 0; i < 10; ++i)
      a1.Insert(i, i); // inserts at the end

    a1.RemoveSwap(9);
    a1.RemoveSwap(7);
    a1.RemoveSwap(5);
    a1.RemoveSwap(3);
    a1.RemoveSwap(1);

    EZ_TEST_INT(a1.GetCount(), 5);

    for (ezInt32 i = 0; i < 5; ++i)
      EZ_TEST_BOOL(ezMath::IsEven(a1[i]));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveAt")
  {
    ezDynamicArray<ezInt32> a1;

    for (ezInt32 i = 0; i < 10; ++i)
      a1.Insert(i, i); // inserts at the end

    a1.RemoveAt(9);
    a1.RemoveAt(7);
    a1.RemoveAt(5);
    a1.RemoveAt(3);
    a1.RemoveAt(1);

    EZ_TEST_INT(a1.GetCount(), 5);

    for (ezInt32 i = 0; i < 5; ++i)
      EZ_TEST_INT(a1[i], i * 2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveAtSwap")
  {
    ezDynamicArray<ezInt32> a1;

    for (ezInt32 i = 0; i < 10; ++i)
      a1.Insert(i, i); // inserts at the end

    a1.RemoveAtSwap(9);
    a1.RemoveAtSwap(7);
    a1.RemoveAtSwap(5);
    a1.RemoveAtSwap(3);
    a1.RemoveAtSwap(1);

    EZ_TEST_INT(a1.GetCount(), 5);

    for (ezInt32 i = 0; i < 5; ++i)
      EZ_TEST_BOOL(ezMath::IsEven(a1[i]));
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
      EZ_TEST_BOOL(st::HasAllDestructed());

      ezDynamicArray<st> a1;
      ezDynamicArray<st> a2;

      EZ_TEST_BOOL(st::HasDone(0, 0)); // nothing has been constructed / destructed in between
      EZ_TEST_BOOL(st::HasAllDestructed());

      a1.PushBack(st(1));
      EZ_TEST_BOOL(st::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a1.Insert(st(2), 0);
      EZ_TEST_BOOL(st::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a2 = a1;
      EZ_TEST_BOOL(st::HasDone(2, 0)); // two copies

      a1.Clear();
      EZ_TEST_BOOL(st::HasDone(0, 2));

      a1.PushBack(st(3));
      a1.PushBack(st(4));
      a1.PushBack(st(5));
      a1.PushBack(st(6));

      EZ_TEST_BOOL(st::HasDone(8, 4)); // four temporaries

      a1.Remove(st(3));
      EZ_TEST_BOOL(st::HasDone(1, 2)); // one temporary, one destroyed

      a1.Remove(st(3));
      EZ_TEST_BOOL(st::HasDone(1, 1)); // one temporary, none destroyed

      a1.RemoveAt(0);
      EZ_TEST_BOOL(st::HasDone(0, 1)); // one destroyed

      a1.RemoveAtSwap(0);
      EZ_TEST_BOOL(st::HasDone(0, 1)); // one destroyed
    }

    // tests the destructor of a2 and a1
    EZ_TEST_BOOL(st::HasAllDestructed());
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

    ezUInt32 last = 0;
    for (ezUInt32 i = 0; i < list.GetCount(); i++)
    {
      EZ_TEST_BOOL(last <= list[i]);
      last = list[i];
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SortingObjects")
  {
    ezDynamicArray<Dummy> list;
    list.Reserve(128);

    for (ezUInt32 i = 0; i < 100; i++)
    {
      list.PushBack(Dummy(rand()));
    }
    list.Sort();

    Dummy last = 0;
    for (ezUInt32 i = 0; i < list.GetCount(); i++)
    {
      EZ_TEST_BOOL(last <= list[i]);
      last = list[i];
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Various")
  {
    ezDynamicArray<Dummy> list;
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

    list.RemoveAt(3);
    list.RemoveAtSwap(2);

    EZ_TEST_BOOL(list[0].a == 1);
    EZ_TEST_BOOL(list[1].a == 0);
    EZ_TEST_BOOL(list[2].a == 0);
    EZ_TEST_BOOL(list[3].a == 4);
    EZ_TEST_BOOL(list.GetCount() == 4);
    EZ_TEST_BOOL(list.IndexOf(0) == 1);
    EZ_TEST_BOOL(list.LastIndexOf(0) == 2);

    list.PushBack(5);
    EZ_TEST_BOOL(list[4].a == 5);
    Dummy d = list.PeekBack();
    list.PopBack();
    EZ_TEST_BOOL(d.a == 5);
    EZ_TEST_BOOL(list.GetCount() == 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Assignment")
  {
    ezDynamicArray<Dummy> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(Dummy(rand()));
    }

    ezDynamicArray<Dummy> list2;
    for (int i = 0; i < 8; i++)
    {
      list2.PushBack(Dummy(rand()));
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
      list2.PushBack(Dummy(rand()));
    }

    list = list2;
    EZ_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    EZ_TEST_BOOL(list == list2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Count")
  {
    ezDynamicArray<Dummy> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(Dummy(rand()));
    }
    list.SetCount(32);
    list.SetCount(4);

    list.Compact();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Reserve")
  {
    EZ_TEST_BOOL(st::HasAllDestructed());

    ezDynamicArray<st> a;

    EZ_TEST_BOOL(st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    EZ_TEST_BOOL(st::HasAllDestructed());

    a.Reserve(100);

    EZ_TEST_BOOL(st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    EZ_TEST_BOOL(st::HasAllDestructed());

    a.SetCount(10);
    EZ_TEST_BOOL(st::HasDone(10, 0));

    a.Reserve(100);
    EZ_TEST_BOOL(st::HasDone(0, 0));

    a.SetCount(100);
    EZ_TEST_BOOL(st::HasDone(90, 0));

    a.Reserve(200);
    EZ_TEST_BOOL(st::HasDone(100, 100)); // had to copy some elements over

    a.SetCount(200);
    EZ_TEST_BOOL(st::HasDone(100, 0));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compact")
  {
    EZ_TEST_BOOL(st::HasAllDestructed());

    ezDynamicArray<st> a;

    EZ_TEST_BOOL(st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    EZ_TEST_BOOL(st::HasAllDestructed());

    a.SetCount(100);
    EZ_TEST_BOOL(st::HasDone(100, 0));

    a.SetCount(200);
    EZ_TEST_BOOL(st::HasDone(200, 100));

    a.SetCount(10);
    EZ_TEST_BOOL(st::HasDone(0, 190));

    // no reallocations and copying, if the memory is already available
    a.SetCount(200);
    EZ_TEST_BOOL(st::HasDone(190, 0));

    a.SetCount(10);
    EZ_TEST_BOOL(st::HasDone(0, 190));

    // now we remove the spare memory
    a.Compact();
    EZ_TEST_BOOL(st::HasDone(10, 10));

    // this time the array needs to be relocated, and thus the already present elements need to be copied
    a.SetCount(200);
    EZ_TEST_BOOL(st::HasDone(200, 10));

    // this does not deallocate memory
    a.Clear();
    EZ_TEST_BOOL(st::HasDone(0, 200));

    a.SetCount(100);
    EZ_TEST_BOOL(st::HasDone(100, 0));

    // therefore no object relocation
    a.SetCount(200);
    EZ_TEST_BOOL(st::HasDone(100, 0));

    a.Clear();
    EZ_TEST_BOOL(st::HasDone(0, 200));

    // this will deallocate ALL memory
    EZ_TEST_BOOL(a.GetHeapMemoryUsage() > 0);
    a.Compact();
    EZ_TEST_BOOL(a.GetHeapMemoryUsage() == 0);

    a.SetCount(100);
    EZ_TEST_BOOL(st::HasDone(100, 0));

    // this time objects need to be relocated
    a.SetCount(200);
    EZ_TEST_BOOL(st::HasDone(200, 100));
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
    ezUInt32 prev = 0;
    for(ezUInt32 val : a1)
    {
      EZ_TEST_BOOL(prev <= val);
      prev = val;
    }

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
    for(ezUInt32 val : a1)
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
      a1.SetCount(10);

      EZ_TEST_BOOL(a1.GetArrayPtr().GetCount() == 10);
      EZ_TEST_BOOL(a1.GetArrayPtr().GetPtr() == a1.GetData());

      const ezDynamicArray<ezInt32>& a1ref = a1;

      EZ_TEST_BOOL(a1ref.GetArrayPtr().GetCount() == 10);
      EZ_TEST_BOOL(a1ref.GetArrayPtr().GetPtr() == a1ref.GetData());
  }
}



