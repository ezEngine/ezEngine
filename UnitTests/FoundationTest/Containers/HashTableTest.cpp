#include <PCH.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/StaticArray.h>

namespace
{
  typedef ezConstructionCounter st;

  struct Collision
  {
    ezUInt32 hash;
    int key;

    inline Collision(ezUInt32 hash, int key)
    {
      this->hash = hash;
      this->key = key;
    }

    inline bool operator==(const Collision& other) const
    {
      return key == other.key;
    }

    EZ_DECLARE_POD_TYPE();
  };
}

template <>
struct ezHashHelper<Collision>
{
  EZ_FORCE_INLINE static ezUInt32 Hash(const Collision& value)
  {
    return value.hash;
  }

  EZ_FORCE_INLINE static bool Equal(const Collision& a, const Collision& b)
  {
    return a == b;
  }
};

template <>
struct ezHashHelper<st>
{
  EZ_FORCE_INLINE static ezUInt32 Hash(const st& value)
  {
    return ezHashHelper<ezInt32>::Hash(value.m_iData);
  }

  EZ_FORCE_INLINE static bool Equal(const st& a, const st& b)
  {
    return a == b;
  }
};

EZ_CREATE_SIMPLE_TEST(Containers, HashTable)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezHashTable<ezInt32, st> table1;

    EZ_TEST_BOOL(table1.GetCount() == 0);
    EZ_TEST_BOOL(table1.IsEmpty());

    ezUInt32 counter = 0;
    for (ezHashTable<ezInt32, st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      ++counter;
    }
    EZ_TEST_INT(counter, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy Constructor/Assignment/Iterator")
  {
    ezHashTable<ezInt32, st> table1;

    for (ezInt32 i = 0; i < 64; ++i)
    {
      ezInt32 key;
      
      do
      {
        key = rand() % 100000;
      }
      while (table1.Contains(key));

      table1.Insert(key, ezConstructionCounter(i));
    }

    // insert an element at the very end
    table1.Insert(47, ezConstructionCounter(64));

    ezHashTable<ezInt32, st> table2;
    table2 = table1;
    ezHashTable<ezInt32, st> table3(table1);

    EZ_TEST_INT(table1.GetCount(), 65);
    EZ_TEST_INT(table2.GetCount(), 65);
    EZ_TEST_INT(table3.GetCount(), 65);

    ezUInt32 uiCounter = 0;
    for (ezHashTable<ezInt32, st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      ezConstructionCounter value;

      EZ_TEST_BOOL(table2.TryGetValue(it.Key(), value));
      EZ_TEST_BOOL(it.Value() == value);

      EZ_TEST_BOOL(table3.TryGetValue(it.Key(), value));
      EZ_TEST_BOOL(it.Value() == value);

      ++uiCounter;
    }
    EZ_TEST_INT(uiCounter, table1.GetCount());

    for (ezHashTable<ezInt32, st>::Iterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      it.Value() = st(42);
    }

    for (ezHashTable<ezInt32, st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      ezConstructionCounter value;

      EZ_TEST_BOOL(table1.TryGetValue(it.Key(), value));
      EZ_TEST_BOOL(it.Value() == value);
      EZ_TEST_BOOL(value.m_iData == 42);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Collision Tests")
  {
    ezHashTable<Collision, int> map2;

    map2[Collision(0, 0)] = 0;
    map2[Collision(1, 1)] = 1;
    map2[Collision(0, 2)] = 2;
    map2[Collision(1, 3)] = 3;
    map2[Collision(1, 4)] = 4;
    map2[Collision(0, 5)] = 5;

    EZ_TEST_BOOL(map2[Collision(0, 0)] == 0);
    EZ_TEST_BOOL(map2[Collision(1, 1)] == 1);
    EZ_TEST_BOOL(map2[Collision(0, 2)] == 2);
    EZ_TEST_BOOL(map2[Collision(1, 3)] == 3);
    EZ_TEST_BOOL(map2[Collision(1, 4)] == 4);
    EZ_TEST_BOOL(map2[Collision(0, 5)] == 5);

    EZ_TEST_BOOL(map2.Contains(Collision(0, 0)));
    EZ_TEST_BOOL(map2.Contains(Collision(1, 1)));
    EZ_TEST_BOOL(map2.Contains(Collision(0, 2)));
    EZ_TEST_BOOL(map2.Contains(Collision(1, 3)));
    EZ_TEST_BOOL(map2.Contains(Collision(1, 4)));
    EZ_TEST_BOOL(map2.Contains(Collision(0, 5)));

    EZ_TEST_BOOL(map2.Remove(Collision(0, 0)));
    EZ_TEST_BOOL(map2.Remove(Collision(1, 1)));

    EZ_TEST_BOOL(map2[Collision(0, 2)] == 2);
    EZ_TEST_BOOL(map2[Collision(1, 3)] == 3);
    EZ_TEST_BOOL(map2[Collision(1, 4)] == 4);
    EZ_TEST_BOOL(map2[Collision(0, 5)] == 5);

    EZ_TEST_BOOL(!map2.Contains(Collision(0, 0)));
    EZ_TEST_BOOL(!map2.Contains(Collision(1, 1)));
    EZ_TEST_BOOL(map2.Contains(Collision(0, 2)));
    EZ_TEST_BOOL(map2.Contains(Collision(1, 3)));
    EZ_TEST_BOOL(map2.Contains(Collision(1, 4)));
    EZ_TEST_BOOL(map2.Contains(Collision(0, 5)));

    map2[Collision(0, 6)] = 6;
    map2[Collision(1, 7)] = 7;

    EZ_TEST_BOOL(map2[Collision(0, 2)] == 2);
    EZ_TEST_BOOL(map2[Collision(1, 3)] == 3);
    EZ_TEST_BOOL(map2[Collision(1, 4)] == 4);
    EZ_TEST_BOOL(map2[Collision(0, 5)] == 5);
    EZ_TEST_BOOL(map2[Collision(0, 6)] == 6);
    EZ_TEST_BOOL(map2[Collision(1, 7)] == 7);

    EZ_TEST_BOOL(map2.Contains(Collision(0, 2)));
    EZ_TEST_BOOL(map2.Contains(Collision(1, 3)));
    EZ_TEST_BOOL(map2.Contains(Collision(1, 4)));
    EZ_TEST_BOOL(map2.Contains(Collision(0, 5)));
    EZ_TEST_BOOL(map2.Contains(Collision(0, 6)));
    EZ_TEST_BOOL(map2.Contains(Collision(1, 7)));

    EZ_TEST_BOOL(map2.Remove(Collision(1, 4)));
    EZ_TEST_BOOL(map2.Remove(Collision(0, 6)));

    EZ_TEST_BOOL(map2[Collision(0, 2)] == 2);
    EZ_TEST_BOOL(map2[Collision(1, 3)] == 3);
    EZ_TEST_BOOL(map2[Collision(0, 5)] == 5);
    EZ_TEST_BOOL(map2[Collision(1, 7)] == 7);

    EZ_TEST_BOOL(!map2.Contains(Collision(1, 4)));
    EZ_TEST_BOOL(!map2.Contains(Collision(0, 6)));
    EZ_TEST_BOOL(map2.Contains(Collision(0, 2)));
    EZ_TEST_BOOL(map2.Contains(Collision(1, 3)));
    EZ_TEST_BOOL(map2.Contains(Collision(0, 5)));
    EZ_TEST_BOOL(map2.Contains(Collision(1, 7)));

    map2[Collision(0, 2)] = 3;
    map2[Collision(0, 5)] = 6;
    map2[Collision(1, 3)] = 4;

    EZ_TEST_BOOL(map2[Collision(0, 2)] == 3);
    EZ_TEST_BOOL(map2[Collision(0, 5)] == 6);
    EZ_TEST_BOOL(map2[Collision(1, 3)] == 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Clear")
  {
    EZ_TEST_BOOL(st::HasAllDestructed());

    {
      ezHashTable<ezUInt32, st> m1;
      m1[0] = st(1);
      EZ_TEST_BOOL(st::HasDone(2, 1)); // for inserting new elements 1 temporary is created (and destroyed)

      m1[1] = st(3);
      EZ_TEST_BOOL(st::HasDone(2, 1)); // for inserting new elements 2 temporary is created (and destroyed)

      m1[0] = st(2);
      EZ_TEST_BOOL(st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      EZ_TEST_BOOL(st::HasDone(0, 2));
      EZ_TEST_BOOL(st::HasAllDestructed());
    }

    {
      ezHashTable<st, ezUInt32> m1;
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Insert/TryGetValue")
  {
    ezHashTable<ezInt32, st> a1;

    for (ezInt32 i = 0; i < 10; ++i)
    {
      EZ_TEST_BOOL(!a1.Insert(i, i - 20));
    }

    for (ezInt32 i = 0; i < 10; ++i)
    {
      st oldValue;
      EZ_TEST_BOOL(a1.Insert(i, i, &oldValue));
      EZ_TEST_INT(oldValue.m_iData, i - 20);
    }

    st value;
    EZ_TEST_BOOL(a1.TryGetValue(9, value));
    EZ_TEST_INT(value.m_iData, 9);

    EZ_TEST_BOOL(!a1.TryGetValue(11, value));
    EZ_TEST_INT(value.m_iData, 9);

    st* pValue;
    EZ_TEST_BOOL(a1.TryGetValue(9, pValue));
    EZ_TEST_INT(pValue->m_iData, 9);

    pValue->m_iData = 20;
    EZ_TEST_INT(a1[9].m_iData, 20);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove/Compact")
  {
    ezHashTable<ezInt32, st> a;

    EZ_TEST_BOOL(a.GetHeapMemoryUsage() == 0);

    for (ezInt32 i = 0; i < 1000; ++i)
    {
      a.Insert(i, i);
      EZ_TEST_INT(a.GetCount(), i + 1);
    }

    EZ_TEST_BOOL(a.GetHeapMemoryUsage() >= 1000 * (sizeof(ezInt32) + sizeof(st)));

    a.Compact();

    for (ezInt32 i = 0; i < 1000; ++i)
      EZ_TEST_INT(a[i].m_iData, i);


    for (ezInt32 i = 0; i < 500; ++i)
    {
      st oldValue;
      EZ_TEST_BOOL(a.Remove(i, &oldValue));
      EZ_TEST_INT(oldValue.m_iData, i);
    }
    
    a.Compact();

    for (ezInt32 i = 500; i < 1000; ++i)
      EZ_TEST_INT(a[i].m_iData, i);

    a.Clear();
    a.Compact();

    EZ_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator[]")
  {
    ezHashTable<ezInt32, ezInt32> a;

    a.Insert(4, 20);
    a[2] = 30;

    EZ_TEST_INT(a[4], 20);
    EZ_TEST_INT(a[2], 30);
    EZ_TEST_INT(a[1], 0); // new values are default constructed
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator==/!=")
  {
    ezStaticArray<ezInt32, 64> keys[2];

    for (ezUInt32 i = 0; i < 64; ++i)
    {
      keys[0].PushBack(rand());
    }

    keys[1] = keys[0];

    ezHashTable<ezInt32, st> t[2];

    for (ezUInt32 i = 0; i < 2; ++i)
    {
      while (!keys[i].IsEmpty())
      {
        const ezUInt32 uiIndex = rand() % keys[i].GetCount();
        const ezInt32 key = keys[i][uiIndex];
        t[i].Insert(key, st(key * 3456));

        keys[i].RemoveAtSwap(uiIndex);
      }
    }

    EZ_TEST_BOOL(t[0] == t[1]);

    t[0].Insert(32, st(64));
    EZ_TEST_BOOL(t[0] != t[1]);

    t[1].Insert(32, st(47));
    EZ_TEST_BOOL(t[0] != t[1]);
  }
}
