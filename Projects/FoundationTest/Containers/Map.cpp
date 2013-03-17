#include <PCH.h>

typedef ezConstructionCounter st;

EZ_CREATE_SIMPLE_TEST(Containers, Map)
{
  EZ_TEST_BLOCK(true, "Constructor")
  {
    ezMap<ezUInt32, ezUInt32> m;
    ezMap<st, ezUInt32> m2;
    ezMap<st, st> m3;
  }

  EZ_TEST_BLOCK(true, "IsEmpty")
  {
    ezMap<ezUInt32, ezUInt32> m;
    EZ_TEST(m.IsEmpty());

    m[1] = 2;
    EZ_TEST(!m.IsEmpty());

    m.Clear();
    EZ_TEST(m.IsEmpty());
  }

  EZ_TEST_BLOCK(true, "GetCount")
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

  EZ_TEST_BLOCK(true, "Clear")
  {
    EZ_TEST(st::HasAllDestructed());

    {
      ezMap<ezUInt32, st> m1;
      m1[0] = st(1);
      EZ_TEST(st::HasDone(3, 2)); // for inserting new elements 2 temporaries are created (and destroyed)

      m1[1] = st(3);
      EZ_TEST(st::HasDone(3, 2)); // for inserting new elements 2 temporaries are created (and destroyed)

      m1[0] = st(2);
      EZ_TEST(st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      EZ_TEST(st::HasDone(0, 2));
      EZ_TEST(st::HasAllDestructed());
    }

    {
      ezMap<st, ezUInt32> m1;
      m1[st(0)] = 1;
      EZ_TEST(st::HasDone(2, 1)); // one temporary

      m1[st(1)] = 3;
      EZ_TEST(st::HasDone(2, 1)); // one temporary

      m1[st(0)] = 2;
      EZ_TEST(st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      EZ_TEST(st::HasDone(0, 2));
      EZ_TEST(st::HasAllDestructed());
    }
  }

  EZ_TEST_BLOCK(true, "Insert")
  {
    ezMap<ezUInt32, ezUInt32> m;
    m.Insert(1, 10);
    m.Insert(3, 30);
    m.Insert(7, 70);
    m.Insert(9, 90);
    m.Insert(4, 40);
    m.Insert(2, 20);
    m.Insert(8, 80);
    m.Insert(5, 50);
    m.Insert(6, 60);

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
  }

  EZ_TEST_BLOCK(true, "Find")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (ezInt32 i = 1000-1; i >= 0; --i)
      EZ_TEST_INT(m.Find(i).Value(), i * 10);
  }


  EZ_TEST_BLOCK(true, "operator[]")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (ezInt32 i = 1000-1; i >= 0; --i)
      EZ_TEST_INT(m[i], i * 10);
  }

  EZ_TEST_BLOCK(true, "Erase (Iterator)")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (ezInt32 i = 0; i < 1000-1; ++i)
    {
      ezMap<ezUInt32, ezUInt32>::Iterator itNext = m.Erase(m.Find(i));
      EZ_TEST(!m.Find(i).IsValid());
      EZ_TEST(itNext.Key() == i + 1);

      EZ_TEST_INT(m.GetCount(), 1000-1 - i);
    }
  }

  EZ_TEST_BLOCK(true, "Erase (Key)")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (ezInt32 i = 0; i < 1000; ++i)
    {
      m.Erase(i);
      EZ_TEST(!m.Find(i).IsValid());

      EZ_TEST_INT(m.GetCount(), 1000-1 - i);
    }
  }

  EZ_TEST_BLOCK(true, "operator=")
  {
    ezMap<ezUInt32, ezUInt32> m, m2;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    m2 = m;

    for (ezInt32 i = 1000-1; i >= 0; --i)
      EZ_TEST_INT(m2[i], i * 10);
  }

  EZ_TEST_BLOCK(true, "Copy Constructor")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    ezMap<ezUInt32, ezUInt32> m2 (m);

    for (ezInt32 i = 1000-1; i >= 0; --i)
      EZ_TEST_INT(m2[i], i * 10);
  }

  EZ_TEST_BLOCK(true, "GetIterator / Forward Iteration")
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
  }

  EZ_TEST_BLOCK(true, "GetIterator / Forward Iteration (const)")
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
  }

  EZ_TEST_BLOCK(true, "GetLastIterator / Backward Iteration")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    ezInt32 i = 1000-1;
    for (ezMap<ezUInt32, ezUInt32>::Iterator it = m.GetLastIterator(); it.IsValid(); --it)
    {
      EZ_TEST_INT(it.Key(), i);
      EZ_TEST_INT(it.Value(), i * 10);
      --i;
    }
  }

  EZ_TEST_BLOCK(true, "GetLastIterator / Backward Iteration (const)")
  {
    ezMap<ezUInt32, ezUInt32> m;

    for (ezInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    const ezMap<ezUInt32, ezUInt32> m2(m);

    ezInt32 i = 1000-1;
    for (ezMap<ezUInt32, ezUInt32>::ConstIterator it = m2.GetLastIterator(); it.IsValid(); --it)
    {
      EZ_TEST_INT(it.Key(), i);
      EZ_TEST_INT(it.Value(), i * 10);
      --i;
    }
  }

  EZ_TEST_BLOCK(true, "LowerBound")
  {
    ezMap<ezInt32, ezInt32> m, m2;

    m[0] =  0;
    m[3] = 30;
    m[7] = 70;
    m[9] = 90;

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

    EZ_TEST(!m.LowerBound(10).IsValid());
  }

  EZ_TEST_BLOCK(true, "UpperBound")
  {
    ezMap<ezInt32, ezInt32> m, m2;

    m[0] =  0;
    m[3] = 30;
    m[7] = 70;
    m[9] = 90;

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
    EZ_TEST(!m.UpperBound(9).IsValid());
    EZ_TEST(!m.UpperBound(10).IsValid());
  }

  EZ_TEST_BLOCK(true, "Insert / Erase")
  {
    // Tests whether reusing of elements makes problems

    ezMap<ezInt32, ezInt32> m;

    for (ezUInt32 r = 0; r < 5; ++r)
    {
      // Insert
      for (ezUInt32 i = 0; i < 10000; ++i)
        m.Insert(i, i * 10);

      EZ_TEST_INT(m.GetCount(), 10000);

      // Erase
      for (ezUInt32 i = 0; i < 5000; ++i)
        m.Erase(i);

      // Insert others
      for (ezUInt32 j = 1; j < 1000; ++j)
        m.Insert(20000 * j, j);

      // Erase
      for (ezUInt32 i = 0; i < 5000; ++i)
        m.Erase(5000 + i);

      // Erase others
      for (ezUInt32 j = 1; j < 1000; ++j)
      {
        EZ_TEST(m.Find(20000 * j).IsValid());
        m.Erase(20000 * j);
      }
    }

    EZ_TEST(m.IsEmpty());
  }
}


