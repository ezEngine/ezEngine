#include <PCH.h>
#include <Foundation/Containers/Map.h>
#include <algorithm>
#include <iterator>

typedef ezConstructionCounter st;

EZ_CREATE_SIMPLE_TEST(Containers, Map)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Iterator")
  {
    ezMap<ezUInt32, ezUInt32> m;
    for (ezUInt32 i = 0; i < 1000; ++i)
      m[i] = i + 1;

    EZ_TEST_INT(std::find(begin(m), end(m), 500).Key(), 499);

    auto itfound = std::find_if(begin(m), end(m), [](ezUInt32 val){return val == 500;});

    EZ_TEST_BOOL(std::find(begin(m), end(m), 500) == itfound);

    ezUInt32 prev = *begin(m);
    for (ezUInt32 val : m)
    {
      EZ_TEST_BOOL(val >= prev);
      prev = val;
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezMap<ezUInt32, ezUInt32> m;
    ezMap<st, ezUInt32> m2;
    ezMap<st, st> m3;
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEmpty")
  {
    ezMap<ezUInt32, ezUInt32> m;
    EZ_TEST_BOOL(m.IsEmpty());

    m[1] = 2;
    EZ_TEST_BOOL(!m.IsEmpty());

    m.Clear();
    EZ_TEST_BOOL(m.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetCount")
  {
    ezMap<ezUInt32, ezUInt32> m;
    EZ_TEST_INT(m.GetCount(), 0);

    m[0] = 1;
    EZ_TEST_INT(m.GetCount(), 1);

    m[1] = 2;
    EZ_TEST_INT(m.GetCount(), 2);

    m[2] = 3;
    EZ_TEST_INT(m.GetCount(), 3);

    m[0] = 1;
    EZ_TEST_INT(m.GetCount(), 3);

    m.Clear();
    EZ_TEST_INT(m.GetCount(), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Clear")
  {
    EZ_TEST_BOOL(st::HasAllDestructed());

    {
      ezMap<ezUInt32, st> m1;
      m1[0] = st(1);
      EZ_TEST_BOOL(st::HasDone(3, 2)); // for inserting new elements 2 temporaries are created (and destroyed)

      m1[1] = st(3);
      EZ_TEST_BOOL(st::HasDone(3, 2)); // for inserting new elements 2 temporaries are created (and destroyed)

      m1[0] = st(2);
      EZ_TEST_BOOL(st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      EZ_TEST_BOOL(st::HasDone(0, 2));
      EZ_TEST_BOOL(st::HasAllDestructed());
    }

    {
      ezMap<st, ezUInt32> m1;
      m1[st(0)] = 1;
      EZ_TEST_BOOL(st::HasDone(2, 1)); // one temporary

      m1[st(1)] = 3;
      EZ_TEST_BOOL(st::HasDone(2, 1)); // one temporary

      m1[st(0)] = 2;
      EZ_TEST_BOOL(st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      EZ_TEST_BOOL(st::HasDone(0, 2));
      EZ_TEST_BOOL(st::HasAllDestructed());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Insert")
  {
    ezMap<ezUInt32, ezUInt32> m;

    EZ_TEST_BOOL(m.GetHeapMemoryUsage() == 0);

    EZ_TEST_BOOL(m.Insert(1, 10).IsValid());
    EZ_TEST_BOOL(m.Insert(1, 10).IsValid());
    m.Insert(3, 30);
    auto it7 = m.Insert(7, 70);
    m.Insert(9, 90);
    m.Insert(4, 40);
    m.Insert(2, 20);
    m.Insert(8, 80);
    m.Insert(5, 50);
    m.Insert(6, 60);

    EZ_TEST_BOOL(m.Insert(7, 70).Value() == 70);
    EZ_TEST_BOOL(m.Insert(7, 70) == it7);

    EZ_TEST_BOOL(m.GetHeapMemoryUsage() >= sizeof(ezUInt32) * 2 * 9);

    EZ_TEST_INT(m[1], 10);
    EZ_TEST_INT(m[2], 20);
    EZ_TEST_INT(m[3], 30);
    EZ_TEST_INT(m[4], 40);
    EZ_TEST_INT(m[5], 50);
    EZ_TEST_INT(m[6], 60);
    EZ_TEST_INT(m[7], 70);
    EZ_TEST_INT(m[8], 80);
    EZ_TEST_INT(m[9], 90);

    EZ_TEST_INT(m.GetCount(), 9);

    for (ezUInt32 i = 0; i < 1000000; ++i)
      m[i] = i;

    EZ_TEST_BOOL(m.GetHeapMemoryUsage() >= sizeof(ezUInt32) * 2 * 1000000);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Find")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (ezInt32 i = 1000 - 1; i >= 0; --i)
      EZ_TEST_INT(m.Find(i).Value(), i * 10);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; i += 2)
      m[i] = i * 10;

    for (ezInt32 i = 0; i < 1000; i += 2)
    {
      EZ_TEST_BOOL(m.Contains(i));
      EZ_TEST_BOOL(!m.Contains(i + 1));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindOrAdd")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
    {
      bool bExisted = true;
      m.FindOrAdd(i, &bExisted).Value() = i * 10;
      EZ_TEST_BOOL(!bExisted);
    }

    for (ezInt32 i = 1000 - 1; i >= 0; --i)
    {
      bool bExisted = false;
      EZ_TEST_INT(m.FindOrAdd(i, &bExisted).Value(), i * 10);
      EZ_TEST_BOOL(bExisted);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator[]")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (ezInt32 i = 1000 - 1; i >= 0; --i)
      EZ_TEST_INT(m[i], i * 10);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove (non-existing)")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
    {
      EZ_TEST_BOOL(!m.Remove(i));
    }

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (ezInt32 i = 0; i < 1000; ++i)
    {
      EZ_TEST_BOOL(m.Remove(i + 500) == i < 500);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove (Iterator)")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (ezInt32 i = 0; i < 1000 - 1; ++i)
    {
      ezMap<ezUInt32, ezUInt32>::Iterator itNext = m.Remove(m.Find(i));
      EZ_TEST_BOOL(!m.Find(i).IsValid());
      EZ_TEST_BOOL(itNext.Key() == i + 1);

      EZ_TEST_INT(m.GetCount(), 1000 - 1 - i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove (Key)")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (ezInt32 i = 0; i < 1000; ++i)
    {
      EZ_TEST_BOOL(m.Remove(i));
      EZ_TEST_BOOL(!m.Find(i).IsValid());

      EZ_TEST_INT(m.GetCount(), 1000 - 1 - i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator=")
  {
    ezMap<ezUInt32, ezUInt32> m, m2;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    m2 = m;

    for (ezInt32 i = 1000 - 1; i >= 0; --i)
      EZ_TEST_INT(m2[i], i * 10);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy Constructor")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    ezMap<ezUInt32, ezUInt32> m2(m);

    for (ezInt32 i = 1000 - 1; i >= 0; --i)
      EZ_TEST_INT(m2[i], i * 10);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetIterator / Forward Iteration")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    ezInt32 i = 0;
    for (ezMap<ezUInt32, ezUInt32>::Iterator it = m.GetIterator(); it.IsValid(); ++it)
    {
      EZ_TEST_INT(it.Key(), i);
      EZ_TEST_INT(it.Value(), i * 10);
      ++i;
    }

    EZ_TEST_INT(i, 1000);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetIterator / Forward Iteration (const)")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    const ezMap<ezUInt32, ezUInt32> m2(m);

    ezInt32 i = 0;
    for (ezMap<ezUInt32, ezUInt32>::ConstIterator it = m2.GetIterator(); it.IsValid(); ++it)
    {
      EZ_TEST_INT(it.Key(), i);
      EZ_TEST_INT(it.Value(), i * 10);
      ++i;
    }

    EZ_TEST_INT(i, 1000);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLastIterator / Backward Iteration")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    ezInt32 i = 1000 - 1;
    for (ezMap<ezUInt32, ezUInt32>::Iterator it = m.GetLastIterator(); it.IsValid(); --it)
    {
      EZ_TEST_INT(it.Key(), i);
      EZ_TEST_INT(it.Value(), i * 10);
      --i;
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLastIterator / Backward Iteration (const)")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    const ezMap<ezUInt32, ezUInt32> m2(m);

    ezInt32 i = 1000 - 1;
    for (ezMap<ezUInt32, ezUInt32>::ConstIterator it = m2.GetLastIterator(); it.IsValid(); --it)
    {
      EZ_TEST_INT(it.Key(), i);
      EZ_TEST_INT(it.Value(), i * 10);
      --i;
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "LowerBound")
  {
    ezMap<ezInt32, ezInt32> m, m2;

    m[0] = 0;
    m[3] = 30;
    m[7] = 70;
    m[9] = 90;

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
    ezMap<ezInt32, ezInt32> m, m2;

    m[0] = 0;
    m[3] = 30;
    m[7] = 70;
    m[9] = 90;

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

    ezMap<ezInt32, ezInt32> m;

    for (ezUInt32 r = 0; r < 5; ++r)
    {
      // Insert
      for (ezUInt32 i = 0; i < 10000; ++i)
        m.Insert(i, i * 10);

      EZ_TEST_INT(m.GetCount(), 10000);

      // Remove
      for (ezUInt32 i = 0; i < 5000; ++i)
        EZ_TEST_BOOL(m.Remove(i));

      // Insert others
      for (ezUInt32 j = 1; j < 1000; ++j)
        m.Insert(20000 * j, j);

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator == / !=")
  {
    ezMap<ezUInt32, ezUInt32> m, m2;

    EZ_TEST_BOOL(m == m2);

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    EZ_TEST_BOOL(m != m2);

    m2 = m;

    EZ_TEST_BOOL(m == m2);
  }
}


