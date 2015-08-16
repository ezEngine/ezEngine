#include <PCH.h>
#include <Foundation/Containers/Set.h>

typedef ezConstructionCounter st;

EZ_CREATE_SIMPLE_TEST(Containers, Set)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezSet<ezUInt32> m;
    ezSet<st, ezUInt32> m2;
    ezSet<st, st> m3;
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
    EZ_TEST_BOOL(st::HasAllDestructed());

    {
      ezSet<st> m1;
      m1.Insert(st(1));
      EZ_TEST_BOOL(st::HasDone(2, 1));

      m1.Insert(st(3));
      EZ_TEST_BOOL(st::HasDone(2, 1));

      m1.Insert(st(1));
      EZ_TEST_BOOL(st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      EZ_TEST_BOOL(st::HasDone(0, 2));
      EZ_TEST_BOOL(st::HasAllDestructed());
    }

    {
      ezSet<st> m1;
      m1.Insert(st(0));
      EZ_TEST_BOOL(st::HasDone(2, 1)); // one temporary

      m1.Insert(st(1));
      EZ_TEST_BOOL(st::HasDone(2, 1)); // one temporary

      m1.Insert(st(0));
      EZ_TEST_BOOL(st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      EZ_TEST_BOOL(st::HasDone(0, 2));
      EZ_TEST_BOOL(st::HasAllDestructed());
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

    // Contains
    EZ_TEST_BOOL(base.Contains(base));

    EZ_TEST_BOOL(base.Contains(empty));
    EZ_TEST_BOOL(!empty.Contains(base));

    EZ_TEST_BOOL(!base.Contains(disjunct));
    EZ_TEST_BOOL(!disjunct.Contains(base));

    EZ_TEST_BOOL(base.Contains(subSet));
    EZ_TEST_BOOL(!subSet.Contains(base));

    EZ_TEST_BOOL(!base.Contains(superSet));
    EZ_TEST_BOOL(superSet.Contains(base));

    EZ_TEST_BOOL(!base.Contains(nonDisjunctNonEmptySubSet));
    EZ_TEST_BOOL(!nonDisjunctNonEmptySubSet.Contains(base));

    // Union
    {
      ezSet<ezUInt32> res;

      res.Union(base);
      EZ_TEST_BOOL(res.Contains(base));
      EZ_TEST_BOOL(base.Contains(res));
      res.Union(subSet);
      EZ_TEST_BOOL(res.Contains(base));
      EZ_TEST_BOOL(res.Contains(subSet));
      EZ_TEST_BOOL(base.Contains(res));
      res.Union(superSet);
      EZ_TEST_BOOL(res.Contains(base));
      EZ_TEST_BOOL(res.Contains(subSet));
      EZ_TEST_BOOL(res.Contains(superSet));
      EZ_TEST_BOOL(superSet.Contains(res));
    }

    // Difference
    {
      ezSet<ezUInt32> res;
      res.Union(base);
      res.Difference(empty);
      EZ_TEST_BOOL(res.Contains(base));
      EZ_TEST_BOOL(base.Contains(res));
      res.Difference(disjunct);
      EZ_TEST_BOOL(res.Contains(base));
      EZ_TEST_BOOL(base.Contains(res));
      res.Difference(subSet);
      EZ_TEST_INT(res.GetCount(), 1);
      res.Contains(3);
    }

    // Intersection
    {
      ezSet<ezUInt32> res;
      res.Union(base);
      res.Intersection(disjunct);
      EZ_TEST_BOOL(res.IsEmpty());
      res.Union(base);
      res.Intersection(subSet);
      EZ_TEST_BOOL(base.Contains(subSet));
      EZ_TEST_BOOL(res.Contains(subSet));
      EZ_TEST_BOOL(subSet.Contains(res));
      res.Intersection(superSet);
      EZ_TEST_BOOL(superSet.Contains(res));
      EZ_TEST_BOOL(res.Contains(subSet));
      EZ_TEST_BOOL(subSet.Contains(res));
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
      EZ_TEST_BOOL(m.Remove(i + 500) == i < 500);
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLastIterator / Backward Iteration")
  {
    ezSet<ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    ezInt32 i = 1000 - 1;
    for (ezSet<ezUInt32>::Iterator it = m.GetLastIterator(); it.IsValid(); --it)
    {
      EZ_TEST_INT(it.Key(), i);
      --i;
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLastIterator / Backward Iteration (const)")
  {
    ezSet<ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    const ezSet<ezUInt32> m2(m);

    ezInt32 i = 1000 - 1;
    for (ezSet<ezUInt32>::Iterator it = m2.GetLastIterator(); it.IsValid(); --it)
    {
      EZ_TEST_INT(it.Key(), i);
      --i;
    }
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

    auto itfound = std::find_if(begin(m), end(m), [](ezUInt32 val){return val == 500;});

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
}


