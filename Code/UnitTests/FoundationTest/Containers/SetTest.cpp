#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Set.h>
#include <Foundation/Memory/CommonAllocators.h>

EZ_CREATE_SIMPLE_TEST(Containers, Set)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezSet<ezUInt32> m;
    ezSet<ezConstructionCounter, ezUInt32> m2;
    ezSet<ezConstructionCounter, ezConstructionCounter> m3;
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEmpty")
  {
    ezSet<ezUInt32> m;
    EZ_TEST_BOOL(m.IsEmpty());

    m.Insert(1);
    EZ_TEST_BOOL(!m.IsEmpty());

    m.Clear();
    EZ_TEST_BOOL(m.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetCount")
  {
    ezSet<ezUInt32> m;
    EZ_TEST_INT(m.GetCount(), 0);

    m.Insert(0);
    EZ_TEST_INT(m.GetCount(), 1);

    m.Insert(1);
    EZ_TEST_INT(m.GetCount(), 2);

    m.Insert(2);
    EZ_TEST_INT(m.GetCount(), 3);

    m.Insert(1);
    EZ_TEST_INT(m.GetCount(), 3);

    m.Clear();
    EZ_TEST_INT(m.GetCount(), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Clear")
  {
    EZ_TEST_BOOL(ezConstructionCounter::HasAllDestructed());

    {
      ezSet<ezConstructionCounter> m1;
      m1.Insert(ezConstructionCounter(1));
      EZ_TEST_BOOL(ezConstructionCounter::HasDone(2, 1));

      m1.Insert(ezConstructionCounter(3));
      EZ_TEST_BOOL(ezConstructionCounter::HasDone(2, 1));

      m1.Insert(ezConstructionCounter(1));
      EZ_TEST_BOOL(ezConstructionCounter::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      EZ_TEST_BOOL(ezConstructionCounter::HasDone(0, 2));
      EZ_TEST_BOOL(ezConstructionCounter::HasAllDestructed());
    }

    {
      ezSet<ezConstructionCounter> m1;
      m1.Insert(ezConstructionCounter(0));
      EZ_TEST_BOOL(ezConstructionCounter::HasDone(2, 1)); // one temporary

      m1.Insert(ezConstructionCounter(1));
      EZ_TEST_BOOL(ezConstructionCounter::HasDone(2, 1)); // one temporary

      m1.Insert(ezConstructionCounter(0));
      EZ_TEST_BOOL(ezConstructionCounter::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      EZ_TEST_BOOL(ezConstructionCounter::HasDone(0, 2));
      EZ_TEST_BOOL(ezConstructionCounter::HasAllDestructed());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Insert")
  {
    ezSet<ezUInt32> m;
    EZ_TEST_BOOL(m.GetHeapMemoryUsage() == 0);

    EZ_TEST_BOOL(m.Insert(1).IsValid());
    EZ_TEST_BOOL(m.Insert(1).IsValid());

    m.Insert(3);
    auto it7 = m.Insert(7);
    m.Insert(9);
    m.Insert(4);
    m.Insert(2);
    m.Insert(8);
    m.Insert(5);
    m.Insert(6);

    EZ_TEST_BOOL(m.Insert(1).Key() == 1);
    EZ_TEST_BOOL(m.Insert(3).Key() == 3);
    EZ_TEST_BOOL(m.Insert(7) == it7);

    EZ_TEST_BOOL(m.GetHeapMemoryUsage() >= sizeof(ezUInt32) * 1 * 9);

    EZ_TEST_BOOL(m.Find(1).IsValid());
    EZ_TEST_BOOL(m.Find(2).IsValid());
    EZ_TEST_BOOL(m.Find(3).IsValid());
    EZ_TEST_BOOL(m.Find(4).IsValid());
    EZ_TEST_BOOL(m.Find(5).IsValid());
    EZ_TEST_BOOL(m.Find(6).IsValid());
    EZ_TEST_BOOL(m.Find(7).IsValid());
    EZ_TEST_BOOL(m.Find(8).IsValid());
    EZ_TEST_BOOL(m.Find(9).IsValid());

    EZ_TEST_BOOL(!m.Find(0).IsValid());
    EZ_TEST_BOOL(!m.Find(10).IsValid());

    EZ_TEST_INT(m.GetCount(), 9);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains")
  {
    ezSet<ezUInt32> m;
    m.Insert(1);
    m.Insert(3);
    m.Insert(7);
    m.Insert(9);
    m.Insert(4);
    m.Insert(2);
    m.Insert(8);
    m.Insert(5);
    m.Insert(6);

    EZ_TEST_BOOL(m.Contains(1));
    EZ_TEST_BOOL(m.Contains(2));
    EZ_TEST_BOOL(m.Contains(3));
    EZ_TEST_BOOL(m.Contains(4));
    EZ_TEST_BOOL(m.Contains(5));
    EZ_TEST_BOOL(m.Contains(6));
    EZ_TEST_BOOL(m.Contains(7));
    EZ_TEST_BOOL(m.Contains(8));
    EZ_TEST_BOOL(m.Contains(9));

    EZ_TEST_BOOL(!m.Contains(0));
    EZ_TEST_BOOL(!m.Contains(10));

    EZ_TEST_INT(m.GetCount(), 9);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Set Operations")
  {
    ezSet<ezUInt32> base;
    base.Insert(1);
    base.Insert(3);
    base.Insert(5);

    ezSet<ezUInt32> empty;

    ezSet<ezUInt32> disjunct;
    disjunct.Insert(2);
    disjunct.Insert(4);
    disjunct.Insert(6);

    ezSet<ezUInt32> subSet;
    subSet.Insert(1);
    subSet.Insert(5);

    ezSet<ezUInt32> superSet;
    superSet.Insert(1);
    superSet.Insert(3);
    superSet.Insert(5);
    superSet.Insert(7);

    ezSet<ezUInt32> nonDisjunctNonEmptySubSet;
    nonDisjunctNonEmptySubSet.Insert(1);
    nonDisjunctNonEmptySubSet.Insert(4);
    nonDisjunctNonEmptySubSet.Insert(5);

    // ContainsSet
    EZ_TEST_BOOL(base.ContainsSet(base));

    EZ_TEST_BOOL(base.ContainsSet(empty));
    EZ_TEST_BOOL(!empty.ContainsSet(base));

    EZ_TEST_BOOL(!base.ContainsSet(disjunct));
    EZ_TEST_BOOL(!disjunct.ContainsSet(base));

    EZ_TEST_BOOL(base.ContainsSet(subSet));
    EZ_TEST_BOOL(!subSet.ContainsSet(base));

    EZ_TEST_BOOL(!base.ContainsSet(superSet));
    EZ_TEST_BOOL(superSet.ContainsSet(base));

    EZ_TEST_BOOL(!base.ContainsSet(nonDisjunctNonEmptySubSet));
    EZ_TEST_BOOL(!nonDisjunctNonEmptySubSet.ContainsSet(base));

    // Union
    {
      ezSet<ezUInt32> res;

      res.Union(base);
      EZ_TEST_BOOL(res.ContainsSet(base));
      EZ_TEST_BOOL(base.ContainsSet(res));
      res.Union(subSet);
      EZ_TEST_BOOL(res.ContainsSet(base));
      EZ_TEST_BOOL(res.ContainsSet(subSet));
      EZ_TEST_BOOL(base.ContainsSet(res));
      res.Union(superSet);
      EZ_TEST_BOOL(res.ContainsSet(base));
      EZ_TEST_BOOL(res.ContainsSet(subSet));
      EZ_TEST_BOOL(res.ContainsSet(superSet));
      EZ_TEST_BOOL(superSet.ContainsSet(res));
    }

    // Difference
    {
      ezSet<ezUInt32> res;
      res.Union(base);
      res.Difference(empty);
      EZ_TEST_BOOL(res.ContainsSet(base));
      EZ_TEST_BOOL(base.ContainsSet(res));
      res.Difference(disjunct);
      EZ_TEST_BOOL(res.ContainsSet(base));
      EZ_TEST_BOOL(base.ContainsSet(res));
      res.Difference(subSet);
      EZ_TEST_INT(res.GetCount(), 1);
      EZ_TEST_BOOL(res.Contains(3));
    }

    // Intersection
    {
      ezSet<ezUInt32> res;
      res.Union(base);
      res.Intersection(disjunct);
      EZ_TEST_BOOL(res.IsEmpty());
      res.Union(base);
      res.Intersection(subSet);
      EZ_TEST_BOOL(base.ContainsSet(subSet));
      EZ_TEST_BOOL(res.ContainsSet(subSet));
      EZ_TEST_BOOL(subSet.ContainsSet(res));
      res.Intersection(superSet);
      EZ_TEST_BOOL(superSet.ContainsSet(res));
      EZ_TEST_BOOL(res.ContainsSet(subSet));
      EZ_TEST_BOOL(subSet.ContainsSet(res));
      res.Intersection(empty);
      EZ_TEST_BOOL(res.IsEmpty());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Find")
  {
    ezSet<ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (ezInt32 i = 1000 - 1; i >= 0; --i)
      EZ_TEST_INT(m.Find(i).Key(), i);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove (non-existing)")
  {
    ezSet<ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      EZ_TEST_BOOL(!m.Remove(i));

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (ezInt32 i = 0; i < 1000; ++i)
      EZ_TEST_BOOL(m.Remove(i + 500) == (i < 500));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove (Iterator)")
  {
    ezSet<ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (ezInt32 i = 0; i < 1000 - 1; ++i)
    {
      ezSet<ezUInt32>::Iterator itNext = m.Remove(m.Find(i));
      EZ_TEST_BOOL(!m.Find(i).IsValid());
      EZ_TEST_BOOL(itNext.Key() == i + 1);

      EZ_TEST_INT(m.GetCount(), 1000 - 1 - i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove (Key)")
  {
    ezSet<ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (ezInt32 i = 0; i < 1000; ++i)
    {
      EZ_TEST_BOOL(m.Remove(i));
      EZ_TEST_BOOL(!m.Find(i).IsValid());

      EZ_TEST_INT(m.GetCount(), 1000 - 1 - i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator=")
  {
    ezSet<ezUInt32> m, m2;

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    m2 = m;

    for (ezInt32 i = 1000 - 1; i >= 0; --i)
      EZ_TEST_BOOL(m2.Find(i).IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy Constructor")
  {
    ezSet<ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    ezSet<ezUInt32> m2(m);

    for (ezInt32 i = 1000 - 1; i >= 0; --i)
      EZ_TEST_BOOL(m2.Find(i).IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetIterator / Forward Iteration")
  {
    ezSet<ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    ezInt32 i = 0;
    for (ezSet<ezUInt32>::Iterator it = m.GetIterator(); it.IsValid(); ++it)
    {
      EZ_TEST_INT(it.Key(), i);
      ++i;
    }

    EZ_TEST_INT(i, 1000);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetIterator / Forward Iteration (const)")
  {
    ezSet<ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    const ezSet<ezUInt32> m2(m);

    ezInt32 i = 0;
    for (ezSet<ezUInt32>::Iterator it = m2.GetIterator(); it.IsValid(); ++it)
    {
      EZ_TEST_INT(it.Key(), i);
      ++i;
    }

    EZ_TEST_INT(i, 1000);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "LowerBound")
  {
    ezSet<ezInt32> m, m2;

    m.Insert(0);
    m.Insert(3);
    m.Insert(7);
    m.Insert(9);

    EZ_TEST_INT(m.LowerBound(-1).Key(), 0);
    EZ_TEST_INT(m.LowerBound(0).Key(), 0);
    EZ_TEST_INT(m.LowerBound(1).Key(), 3);
    EZ_TEST_INT(m.LowerBound(2).Key(), 3);
    EZ_TEST_INT(m.LowerBound(3).Key(), 3);
    EZ_TEST_INT(m.LowerBound(4).Key(), 7);
    EZ_TEST_INT(m.LowerBound(5).Key(), 7);
    EZ_TEST_INT(m.LowerBound(6).Key(), 7);
    EZ_TEST_INT(m.LowerBound(7).Key(), 7);
    EZ_TEST_INT(m.LowerBound(8).Key(), 9);
    EZ_TEST_INT(m.LowerBound(9).Key(), 9);

    EZ_TEST_BOOL(!m.LowerBound(10).IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "UpperBound")
  {
    ezSet<ezInt32> m, m2;

    m.Insert(0);
    m.Insert(3);
    m.Insert(7);
    m.Insert(9);

    EZ_TEST_INT(m.UpperBound(-1).Key(), 0);
    EZ_TEST_INT(m.UpperBound(0).Key(), 3);
    EZ_TEST_INT(m.UpperBound(1).Key(), 3);
    EZ_TEST_INT(m.UpperBound(2).Key(), 3);
    EZ_TEST_INT(m.UpperBound(3).Key(), 7);
    EZ_TEST_INT(m.UpperBound(4).Key(), 7);
    EZ_TEST_INT(m.UpperBound(5).Key(), 7);
    EZ_TEST_INT(m.UpperBound(6).Key(), 7);
    EZ_TEST_INT(m.UpperBound(7).Key(), 9);
    EZ_TEST_INT(m.UpperBound(8).Key(), 9);
    EZ_TEST_BOOL(!m.UpperBound(9).IsValid());
    EZ_TEST_BOOL(!m.UpperBound(10).IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Insert / Remove")
  {
    // Tests whether reusing of elements makes problems

    ezSet<ezInt32> m;

    for (ezUInt32 r = 0; r < 5; ++r)
    {
      // Insert
      for (ezUInt32 i = 0; i < 10000; ++i)
        m.Insert(i);

      EZ_TEST_INT(m.GetCount(), 10000);

      // Remove
      for (ezUInt32 i = 0; i < 5000; ++i)
        EZ_TEST_BOOL(m.Remove(i));

      // Insert others
      for (ezUInt32 j = 1; j < 1000; ++j)
        m.Insert(20000 * j);

      // Remove
      for (ezUInt32 i = 0; i < 5000; ++i)
        EZ_TEST_BOOL(m.Remove(5000 + i));

      // Remove others
      for (ezUInt32 j = 1; j < 1000; ++j)
      {
        EZ_TEST_BOOL(m.Find(20000 * j).IsValid());
        EZ_TEST_BOOL(m.Remove(20000 * j));
      }
    }

    EZ_TEST_BOOL(m.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Iterator")
  {
    ezSet<ezUInt32> m;
    for (ezUInt32 i = 0; i < 1000; ++i)
      m.Insert(i + 1);

    EZ_TEST_INT(std::find(begin(m), end(m), 500).Key(), 500);

    auto itfound = std::find_if(begin(m), end(m), [](ezUInt32 uiVal)
      { return uiVal == 500; });

    EZ_TEST_BOOL(std::find(begin(m), end(m), 500) == itfound);

    ezUInt32 prev = *begin(m);
    for (ezUInt32 val : m)
    {
      EZ_TEST_BOOL(val >= prev);
      prev = val;
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator == / !=")
  {
    ezSet<ezUInt32> m, m2;

    EZ_TEST_BOOL(m == m2);

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i * 10);

    EZ_TEST_BOOL(m != m2);

    m2 = m;

    EZ_TEST_BOOL(m == m2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompatibleKeyType")
  {
    {
      ezSet<ezString> stringSet;
      const char* szChar = "Char";
      const char* szString = "ViewBla";
      ezStringView sView(szString, szString + 4);
      ezStringBuilder sBuilder("Builder");
      ezString sString("String");
      stringSet.Insert(szChar);
      stringSet.Insert(sView);
      stringSet.Insert(sBuilder);
      stringSet.Insert(sString);

      EZ_TEST_BOOL(stringSet.Contains(szChar));
      EZ_TEST_BOOL(stringSet.Contains(sView));
      EZ_TEST_BOOL(stringSet.Contains(sBuilder));
      EZ_TEST_BOOL(stringSet.Contains(sString));

      EZ_TEST_BOOL(stringSet.Remove(szChar));
      EZ_TEST_BOOL(stringSet.Remove(sView));
      EZ_TEST_BOOL(stringSet.Remove(sBuilder));
      EZ_TEST_BOOL(stringSet.Remove(sString));
    }

    // dynamic array as key, check for allocations in comparisons
    {
      ezProxyAllocator testAllocator("Test", ezFoundation::GetDefaultAllocator());
      ezLocalAllocatorWrapper allocWrapper(&testAllocator);
      using TestDynArray = ezDynamicArray<int, ezLocalAllocatorWrapper>;
      TestDynArray a;
      TestDynArray b;
      for (int i = 0; i < 10; ++i)
      {
        a.PushBack(i);
        b.PushBack(i * 2);
      }

      ezSet<TestDynArray> arraySet;
      arraySet.Insert(a);
      arraySet.Insert(b);

      ezArrayPtr<const int> aPtr = a.GetArrayPtr();
      ezArrayPtr<const int> bPtr = b.GetArrayPtr();

      ezUInt64 oldAllocCount = testAllocator.GetStats().m_uiNumAllocations;

      EZ_TEST_BOOL(arraySet.Contains(aPtr));
      EZ_TEST_BOOL(arraySet.Contains(bPtr));
      EZ_TEST_BOOL(arraySet.Contains(a));

      EZ_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

      EZ_TEST_BOOL(arraySet.Remove(aPtr));
      EZ_TEST_BOOL(arraySet.Remove(bPtr));

      EZ_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);
    }
  }

  constexpr ezUInt32 uiSetSize = sizeof(ezSet<ezString>);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Swap")
  {
    ezUInt8 set1Mem[uiSetSize];
    ezUInt8 set2Mem[uiSetSize];
    ezMemoryUtils::PatternFill(set1Mem, 0xCA, uiSetSize);
    ezMemoryUtils::PatternFill(set2Mem, 0xCA, uiSetSize);

    ezStringBuilder tmp;
    ezSet<ezString>* set1 = new (set1Mem)(ezSet<ezString>);
    ezSet<ezString>* set2 = new (set2Mem)(ezSet<ezString>);

    for (ezUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      set1->Insert(tmp);

      tmp.SetFormat("{0}{0}{0}", i);
      set2->Insert(tmp);
    }

    set1->Swap(*set2);

    // test swapped elements
    for (ezUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      EZ_TEST_BOOL(set2->Contains(tmp));

      tmp.SetFormat("{0}{0}{0}", i);
      EZ_TEST_BOOL(set1->Contains(tmp));
    }

    // test iterators after swap
    {
      for (const auto& element : *set1)
      {
        EZ_TEST_BOOL(!set2->Contains(element));
      }

      for (const auto& element : *set2)
      {
        EZ_TEST_BOOL(!set1->Contains(element));
      }
    }

    // due to a compiler bug in VS 2017, PatternFill cannot be called here, because it will move the memset BEFORE the destructor call!
    // seems to be fixed in VS 2019 though

    set1->~ezSet<ezString>();
    // ezMemoryUtils::PatternFill(set1Mem, 0xBA, uiSetSize);

    set2->~ezSet<ezString>();
    ezMemoryUtils::PatternFill(set2Mem, 0xBA, uiSetSize);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Swap Empty")
  {
    ezUInt8 set1Mem[uiSetSize];
    ezUInt8 set2Mem[uiSetSize];
    ezMemoryUtils::PatternFill(set1Mem, 0xCA, uiSetSize);
    ezMemoryUtils::PatternFill(set2Mem, 0xCA, uiSetSize);

    ezStringBuilder tmp;
    ezSet<ezString>* set1 = new (set1Mem)(ezSet<ezString>);
    ezSet<ezString>* set2 = new (set2Mem)(ezSet<ezString>);

    for (ezUInt32 i = 0; i < 100; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      set1->Insert(tmp);
    }

    set1->Swap(*set2);
    EZ_TEST_BOOL(set1->IsEmpty());

    set1->~ezSet<ezString>();
    ezMemoryUtils::PatternFill(set1Mem, 0xBA, uiSetSize);

    // test swapped elements
    for (ezUInt32 i = 0; i < 100; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      EZ_TEST_BOOL(set2->Contains(tmp));
    }

    // test iterators after swap
    {
      for (const auto& element : *set2)
      {
        EZ_TEST_BOOL(set2->Contains(element));
      }
    }

    set2->~ezSet<ezString>();
    ezMemoryUtils::PatternFill(set2Mem, 0xBA, uiSetSize);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetReverseIterator")
  {
    ezSet<ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    ezInt32 i = 1000 - 1;
    for (ezSet<ezUInt32>::ReverseIterator it = m.GetReverseIterator(); it.IsValid(); ++it)
    {
      EZ_TEST_INT(it.Key(), i);
      --i;
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetReverseIterator (const)")
  {
    ezSet<ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    const ezSet<ezUInt32> m2(m);

    ezInt32 i = 1000 - 1;
    for (ezSet<ezUInt32>::ReverseIterator it = m2.GetReverseIterator(); it.IsValid(); ++it)
    {
      EZ_TEST_INT(it.Key(), i);
      --i;
    }
  }
}
