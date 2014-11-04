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
    m.Insert(1);
    m.Insert(3);
    m.Insert(7);
    m.Insert(9);
    m.Insert(4);
    m.Insert(2);
    m.Insert(8);
    m.Insert(5);
    m.Insert(6);

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Find")
  {
    ezSet<ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (ezInt32 i = 1000-1; i >= 0; --i)
      EZ_TEST_INT(m.Find(i).Key(), i);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove (non-existing)")
  {
    ezSet<ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Remove(i);

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Remove(i + 500);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove (Iterator)")
  {
    ezSet<ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (ezInt32 i = 0; i < 1000-1; ++i)
    {
      ezSet<ezUInt32>::Iterator itNext = m.Remove(m.Find(i));
      EZ_TEST_BOOL(!m.Find(i).IsValid());
      EZ_TEST_BOOL(itNext.Key() == i + 1);

      EZ_TEST_INT(m.GetCount(), 1000-1 - i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove (Key)")
  {
    ezSet<ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (ezInt32 i = 0; i < 1000; ++i)
    {
      m.Remove(i);
      EZ_TEST_BOOL(!m.Find(i).IsValid());

      EZ_TEST_INT(m.GetCount(), 1000-1 - i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator=")
  {
    ezSet<ezUInt32> m, m2;

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    m2 = m;

    for (ezInt32 i = 1000-1; i >= 0; --i)
      EZ_TEST_BOOL(m2.Find(i).IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy Constructor")
  {
    ezSet<ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    ezSet<ezUInt32> m2 (m);

    for (ezInt32 i = 1000-1; i >= 0; --i)
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

    ezInt32 i = 1000-1;
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

    ezInt32 i = 1000-1;
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

    EZ_TEST_INT(m.LowerBound(-1).Key(),0);
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

    EZ_TEST_INT(m.UpperBound(-1).Key(),0);
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
        m.Remove(i);

      // Insert others
      for (ezUInt32 j = 1; j < 1000; ++j)
        m.Insert(20000 * j);

      // Remove
      for (ezUInt32 i = 0; i < 5000; ++i)
        m.Remove(5000 + i);

      // Remove others
      for (ezUInt32 j = 1; j < 1000; ++j)
      {
        EZ_TEST_BOOL(m.Find(20000 * j).IsValid());
        m.Remove(20000 * j);
      }
    }

    EZ_TEST_BOOL(m.IsEmpty());
  }
}


