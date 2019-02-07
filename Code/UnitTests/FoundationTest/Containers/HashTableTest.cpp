#include <FoundationTestPCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Strings/String.h>

namespace HashTableTestDetail
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

    inline bool operator==(const Collision& other) const { return key == other.key; }

    EZ_DECLARE_POD_TYPE();
  };

  class OnlyMovable
  {
  public:
    OnlyMovable(ezUInt32 hash)
        : hash(hash)
        , m_NumTimesMoved(0)
    {
    }
    OnlyMovable(OnlyMovable&& other) { *this = std::move(other); }

    void operator=(OnlyMovable&& other)
    {
      hash = other.hash;
      m_NumTimesMoved = 0;
      ++other.m_NumTimesMoved;
    }

    bool operator==(const OnlyMovable& other) const { return hash == other.hash; }

    int m_NumTimesMoved;
    ezUInt32 hash;

  private:
    OnlyMovable(const OnlyMovable&);
    void operator=(const OnlyMovable&);
  };
}

template <>
struct ezHashHelper<HashTableTestDetail::Collision>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const HashTableTestDetail::Collision& value) { return value.hash; }

  EZ_ALWAYS_INLINE static bool Equal(const HashTableTestDetail::Collision& a, const HashTableTestDetail::Collision& b) { return a == b; }
};

template <>
struct ezHashHelper<HashTableTestDetail::OnlyMovable>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const HashTableTestDetail::OnlyMovable& value) { return value.hash; }

  EZ_ALWAYS_INLINE static bool Equal(const HashTableTestDetail::OnlyMovable& a, const HashTableTestDetail::OnlyMovable& b)
  {
    return a.hash == b.hash;
  }
};

EZ_CREATE_SIMPLE_TEST(Containers, HashTable)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezHashTable<ezInt32, HashTableTestDetail::st> table1;

    EZ_TEST_BOOL(table1.GetCount() == 0);
    EZ_TEST_BOOL(table1.IsEmpty());

    ezUInt32 counter = 0;
    for (ezHashTable<ezInt32, HashTableTestDetail::st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      ++counter;
    }
    EZ_TEST_INT(counter, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy Constructor/Assignment/Iterator")
  {
    ezHashTable<ezInt32, HashTableTestDetail::st> table1;

    for (ezInt32 i = 0; i < 64; ++i)
    {
      ezInt32 key;

      do
      {
        key = rand() % 100000;
      } while (table1.Contains(key));

      table1.Insert(key, ezConstructionCounter(i));
    }

    // insert an element at the very end
    table1.Insert(47, ezConstructionCounter(64));

    ezHashTable<ezInt32, HashTableTestDetail::st> table2;
    table2 = table1;
    ezHashTable<ezInt32, HashTableTestDetail::st> table3(table1);

    EZ_TEST_INT(table1.GetCount(), 65);
    EZ_TEST_INT(table2.GetCount(), 65);
    EZ_TEST_INT(table3.GetCount(), 65);

    ezUInt32 uiCounter = 0;
    for (ezHashTable<ezInt32, HashTableTestDetail::st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      ezConstructionCounter value;

      EZ_TEST_BOOL(table2.TryGetValue(it.Key(), value));
      EZ_TEST_BOOL(it.Value() == value);
      EZ_TEST_BOOL(*table2.GetValue(it.Key()) == it.Value());

      EZ_TEST_BOOL(table3.TryGetValue(it.Key(), value));
      EZ_TEST_BOOL(it.Value() == value);
      EZ_TEST_BOOL(*table3.GetValue(it.Key()) == it.Value());

      ++uiCounter;
    }
    EZ_TEST_INT(uiCounter, table1.GetCount());

    for (ezHashTable<ezInt32, HashTableTestDetail::st>::Iterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      it.Value() = HashTableTestDetail::st(42);
    }

    for (ezHashTable<ezInt32, HashTableTestDetail::st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      ezConstructionCounter value;

      EZ_TEST_BOOL(table1.TryGetValue(it.Key(), value));
      EZ_TEST_BOOL(it.Value() == value);
      EZ_TEST_BOOL(value.m_iData == 42);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Move Copy Constructor/Assignment")
  {
    ezHashTable<ezInt32, HashTableTestDetail::st> table1;
    for (ezInt32 i = 0; i < 64; ++i)
    {
      table1.Insert(i, ezConstructionCounter(i));
    }

    ezUInt64 memoryUsage = table1.GetHeapMemoryUsage();

    ezHashTable<ezInt32, HashTableTestDetail::st> table2;
    table2 = std::move(table1);

    EZ_TEST_INT(table1.GetCount(), 0);
    EZ_TEST_INT(table1.GetHeapMemoryUsage(), 0);
    EZ_TEST_INT(table2.GetCount(), 64);
    EZ_TEST_INT(table2.GetHeapMemoryUsage(), memoryUsage);

    ezHashTable<ezInt32, HashTableTestDetail::st> table3(std::move(table2));

    EZ_TEST_INT(table2.GetCount(), 0);
    EZ_TEST_INT(table2.GetHeapMemoryUsage(), 0);
    EZ_TEST_INT(table3.GetCount(), 64);
    EZ_TEST_INT(table3.GetHeapMemoryUsage(), memoryUsage);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Move Insert")
  {
    HashTableTestDetail::OnlyMovable noCopyObject(42);

    {
      ezHashTable<HashTableTestDetail::OnlyMovable, int> noCopyKey;
      // noCopyKey.Insert(noCopyObject, 10); // Should not compile
      noCopyKey.Insert(std::move(noCopyObject), 10);
      EZ_TEST_INT(noCopyObject.m_NumTimesMoved, 1);
      EZ_TEST_BOOL(noCopyKey.Contains(noCopyObject));
    }

    {
      ezHashTable<int, HashTableTestDetail::OnlyMovable> noCopyValue;
      // noCopyValue.Insert(10, noCopyObject); // Should not compile
      noCopyValue.Insert(10, std::move(noCopyObject));
      EZ_TEST_INT(noCopyObject.m_NumTimesMoved, 2);
      EZ_TEST_BOOL(noCopyValue.Contains(10));
    }

    {
      ezHashTable<HashTableTestDetail::OnlyMovable, HashTableTestDetail::OnlyMovable> noCopyAnything;
      // noCopyAnything.Insert(10, noCopyObject); // Should not compile
      // noCopyAnything.Insert(noCopyObject, 10); // Should not compile
      noCopyAnything.Insert(std::move(noCopyObject), std::move(noCopyObject));
      EZ_TEST_INT(noCopyObject.m_NumTimesMoved, 4);
      EZ_TEST_BOOL(noCopyAnything.Contains(noCopyObject));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Collision Tests")
  {
    ezHashTable<HashTableTestDetail::Collision, int> map2;

    map2[HashTableTestDetail::Collision(0, 0)] = 0;
    map2[HashTableTestDetail::Collision(1, 1)] = 1;
    map2[HashTableTestDetail::Collision(0, 2)] = 2;
    map2[HashTableTestDetail::Collision(1, 3)] = 3;
    map2[HashTableTestDetail::Collision(1, 4)] = 4;
    map2[HashTableTestDetail::Collision(0, 5)] = 5;

    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 0)] == 0);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 1)] == 1);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 2);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 3);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 4)] == 4);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 5);

    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 0)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 1)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 2)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 3)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 4)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 5)));

    EZ_TEST_BOOL(map2.Remove(HashTableTestDetail::Collision(0, 0)));
    EZ_TEST_BOOL(map2.Remove(HashTableTestDetail::Collision(1, 1)));

    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 2);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 3);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 4)] == 4);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 5);

    EZ_TEST_BOOL(!map2.Contains(HashTableTestDetail::Collision(0, 0)));
    EZ_TEST_BOOL(!map2.Contains(HashTableTestDetail::Collision(1, 1)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 2)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 3)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 4)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 5)));

    map2[HashTableTestDetail::Collision(0, 6)] = 6;
    map2[HashTableTestDetail::Collision(1, 7)] = 7;

    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 2);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 3);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 4)] == 4);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 5);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 6)] == 6);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 7)] == 7);

    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 2)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 3)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 4)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 5)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 6)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 7)));

    EZ_TEST_BOOL(map2.Remove(HashTableTestDetail::Collision(1, 4)));
    EZ_TEST_BOOL(map2.Remove(HashTableTestDetail::Collision(0, 6)));

    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 2);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 3);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 5);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 7)] == 7);

    EZ_TEST_BOOL(!map2.Contains(HashTableTestDetail::Collision(1, 4)));
    EZ_TEST_BOOL(!map2.Contains(HashTableTestDetail::Collision(0, 6)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 2)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 3)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 5)));
    EZ_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 7)));

    map2[HashTableTestDetail::Collision(0, 2)] = 3;
    map2[HashTableTestDetail::Collision(0, 5)] = 6;
    map2[HashTableTestDetail::Collision(1, 3)] = 4;

    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 3);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 6);
    EZ_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Clear")
  {
    EZ_TEST_BOOL(HashTableTestDetail::st::HasAllDestructed());

    {
      ezHashTable<ezUInt32, HashTableTestDetail::st> m1;
      m1[0] = HashTableTestDetail::st(1);
      EZ_TEST_BOOL(HashTableTestDetail::st::HasDone(2, 1)); // for inserting new elements 1 temporary is created (and destroyed)

      m1[1] = HashTableTestDetail::st(3);
      EZ_TEST_BOOL(HashTableTestDetail::st::HasDone(2, 1)); // for inserting new elements 2 temporary is created (and destroyed)

      m1[0] = HashTableTestDetail::st(2);
      EZ_TEST_BOOL(HashTableTestDetail::st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      EZ_TEST_BOOL(HashTableTestDetail::st::HasDone(0, 2));
      EZ_TEST_BOOL(HashTableTestDetail::st::HasAllDestructed());
    }

    {
      ezHashTable<HashTableTestDetail::st, ezUInt32> m1;
      m1[HashTableTestDetail::st(0)] = 1;
      EZ_TEST_BOOL(HashTableTestDetail::st::HasDone(2, 1)); // one temporary

      m1[HashTableTestDetail::st(1)] = 3;
      EZ_TEST_BOOL(HashTableTestDetail::st::HasDone(2, 1)); // one temporary

      m1[HashTableTestDetail::st(0)] = 2;
      EZ_TEST_BOOL(HashTableTestDetail::st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      EZ_TEST_BOOL(HashTableTestDetail::st::HasDone(0, 2));
      EZ_TEST_BOOL(HashTableTestDetail::st::HasAllDestructed());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Insert/TryGetValue/GetValue")
  {
    ezHashTable<ezInt32, HashTableTestDetail::st> a1;

    for (ezInt32 i = 0; i < 10; ++i)
    {
      EZ_TEST_BOOL(!a1.Insert(i, i - 20));
    }

    for (ezInt32 i = 0; i < 10; ++i)
    {
      HashTableTestDetail::st oldValue;
      EZ_TEST_BOOL(a1.Insert(i, i, &oldValue));
      EZ_TEST_INT(oldValue.m_iData, i - 20);
    }

    HashTableTestDetail::st value;
    EZ_TEST_BOOL(a1.TryGetValue(9, value));
    EZ_TEST_INT(value.m_iData, 9);
    EZ_TEST_INT(a1.GetValue(9)->m_iData, 9);

    EZ_TEST_BOOL(!a1.TryGetValue(11, value));
    EZ_TEST_INT(value.m_iData, 9);
    EZ_TEST_BOOL(a1.GetValue(11) == nullptr);

    HashTableTestDetail::st* pValue;
    EZ_TEST_BOOL(a1.TryGetValue(9, pValue));
    EZ_TEST_INT(pValue->m_iData, 9);

    pValue->m_iData = 20;
    EZ_TEST_INT(a1[9].m_iData, 20);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove/Compact")
  {
    ezHashTable<ezInt32, HashTableTestDetail::st> a;

    EZ_TEST_BOOL(a.GetHeapMemoryUsage() == 0);

    for (ezInt32 i = 0; i < 1000; ++i)
    {
      a.Insert(i, i);
      EZ_TEST_INT(a.GetCount(), i + 1);
    }

    EZ_TEST_BOOL(a.GetHeapMemoryUsage() >= 1000 * (sizeof(ezInt32) + sizeof(HashTableTestDetail::st)));

    a.Compact();

    for (ezInt32 i = 0; i < 1000; ++i)
      EZ_TEST_INT(a[i].m_iData, i);


    for (ezInt32 i = 0; i < 250; ++i)
    {
      HashTableTestDetail::st oldValue;
      EZ_TEST_BOOL(a.Remove(i, &oldValue));
      EZ_TEST_INT(oldValue.m_iData, i);
    }
    EZ_TEST_INT(a.GetCount(), 750);

    for (ezHashTable<ezInt32, HashTableTestDetail::st>::Iterator it = a.GetIterator(); it.IsValid();)
    {
      if (it.Key() < 500)
        it = a.Remove(it);
      else
        ++it;
    }
    EZ_TEST_INT(a.GetCount(), 500);
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

    ezHashTable<ezInt32, HashTableTestDetail::st> t[2];

    for (ezUInt32 i = 0; i < 2; ++i)
    {
      while (!keys[i].IsEmpty())
      {
        const ezUInt32 uiIndex = rand() % keys[i].GetCount();
        const ezInt32 key = keys[i][uiIndex];
        t[i].Insert(key, HashTableTestDetail::st(key * 3456));

        keys[i].RemoveAtAndSwap(uiIndex);
      }
    }

    EZ_TEST_BOOL(t[0] == t[1]);

    t[0].Insert(32, HashTableTestDetail::st(64));
    EZ_TEST_BOOL(t[0] != t[1]);

    t[1].Insert(32, HashTableTestDetail::st(47));
    EZ_TEST_BOOL(t[0] != t[1]);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompatibleKeyType")
  {
    ezHashTable<ezString, int> stringTable;
    const char* szChar = "Char";
    const char* szString = "ViewBla";
    ezStringView sView(szString, szString + 4);
    ezStringBuilder sBuilder("Builder");
    ezString sString("String");
    EZ_TEST_BOOL(!stringTable.Insert(szChar, 1));
    EZ_TEST_BOOL(!stringTable.Insert(sView, 2));
    EZ_TEST_BOOL(!stringTable.Insert(sBuilder, 3));
    EZ_TEST_BOOL(!stringTable.Insert(sString, 4));
    EZ_TEST_BOOL(stringTable.Insert("View", 2));

    EZ_TEST_BOOL(stringTable.Contains(szChar));
    EZ_TEST_BOOL(stringTable.Contains(sView));
    EZ_TEST_BOOL(stringTable.Contains(sBuilder));
    EZ_TEST_BOOL(stringTable.Contains(sString));

    EZ_TEST_INT(*stringTable.GetValue(szChar), 1);
    EZ_TEST_INT(*stringTable.GetValue(sView), 2);
    EZ_TEST_INT(*stringTable.GetValue(sBuilder), 3);
    EZ_TEST_INT(*stringTable.GetValue(sString), 4);

    EZ_TEST_BOOL(stringTable.Remove(szChar));
    EZ_TEST_BOOL(stringTable.Remove(sView));
    EZ_TEST_BOOL(stringTable.Remove(sBuilder));
    EZ_TEST_BOOL(stringTable.Remove(sString));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Swap")
  {
    ezStringBuilder tmp;
    ezHashTable<ezString, ezInt32> map1;
    ezHashTable<ezString, ezInt32> map2;

    for (ezUInt32 i = 0; i < 1000; ++i)
    {
      tmp.Format("stuff{}bla", i);
      map1[tmp] = i;

      tmp.Format("{0}{0}{0}", i);
      map2[tmp] = i;
    }

    map1.Swap(map2);

    for (ezUInt32 i = 0; i < 1000; ++i)
    {
      tmp.Format("stuff{}bla", i);
      EZ_TEST_BOOL(map2.Contains(tmp));
      EZ_TEST_INT(map2[tmp], i);

      tmp.Format("{0}{0}{0}", i);
      EZ_TEST_BOOL(map1.Contains(tmp));
      EZ_TEST_INT(map1[tmp], i);
    }
  }
}
