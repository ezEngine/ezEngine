#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Memory/CommonAllocators.h>

namespace
{
  using st = ezConstructionCounter;

  struct Collision
  {
    ezUInt32 hash;
    int key;

    inline Collision(ezUInt32 uiHash, int iKey)
    {
      this->hash = uiHash;
      this->key = iKey;
    }

    inline bool operator==(const Collision& other) const { return key == other.key; }

    EZ_DECLARE_POD_TYPE();
  };

  class OnlyMovable
  {
  public:
    OnlyMovable(ezUInt32 uiHash)
      : hash(uiHash)

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

    int m_NumTimesMoved = 0;
    ezUInt32 hash;

  private:
    OnlyMovable(const OnlyMovable&);
    void operator=(const OnlyMovable&);
  };
} // namespace

template <>
struct ezHashHelper<Collision>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const Collision& value) { return value.hash; }

  EZ_ALWAYS_INLINE static bool Equal(const Collision& a, const Collision& b) { return a == b; }
};

template <>
struct ezHashHelper<OnlyMovable>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const OnlyMovable& value) { return value.hash; }

  EZ_ALWAYS_INLINE static bool Equal(const OnlyMovable& a, const OnlyMovable& b) { return a.hash == b.hash; }
};

EZ_CREATE_SIMPLE_TEST(Containers, HashSet)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezHashSet<ezInt32> table1;

    EZ_TEST_BOOL(table1.GetCount() == 0);
    EZ_TEST_BOOL(table1.IsEmpty());

    ezUInt32 counter = 0;
    for (auto it = table1.GetIterator(); it.IsValid(); ++it)
    {
      ++counter;
    }
    EZ_TEST_INT(counter, 0);

    EZ_TEST_BOOL(begin(table1) == end(table1));
    EZ_TEST_BOOL(cbegin(table1) == cend(table1));
    table1.Reserve(10);
    EZ_TEST_BOOL(begin(table1) == end(table1));
    EZ_TEST_BOOL(cbegin(table1) == cend(table1));

    for (auto value : table1)
    {
      ++counter;
    }
    EZ_TEST_INT(counter, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy Constructor/Assignment/Iterator")
  {
    ezHashSet<ezInt32> table1;

    for (ezInt32 i = 0; i < 64; ++i)
    {
      ezInt32 key;

      do
      {
        key = rand() % 100000;
      } while (table1.Contains(key));

      table1.Insert(key);
    }

    // insert an element at the very end
    table1.Insert(47);

    ezHashSet<ezInt32> table2;
    table2 = table1;
    ezHashSet<ezInt32> table3(table1);

    EZ_TEST_INT(table1.GetCount(), 65);
    EZ_TEST_INT(table2.GetCount(), 65);
    EZ_TEST_INT(table3.GetCount(), 65);
    EZ_TEST_BOOL(begin(table1) != end(table1));
    EZ_TEST_BOOL(cbegin(table1) != cend(table1));

    ezUInt32 uiCounter = 0;
    for (auto it = table1.GetIterator(); it.IsValid(); ++it)
    {
      ezConstructionCounter value;
      EZ_TEST_BOOL(table2.Contains(it.Key()));
      EZ_TEST_BOOL(table3.Contains(it.Key()));
      ++uiCounter;
    }
    EZ_TEST_INT(uiCounter, table1.GetCount());

    uiCounter = 0;
    for (const auto& value : table1)
    {
      EZ_TEST_BOOL(table2.Contains(value));
      EZ_TEST_BOOL(table3.Contains(value));
      ++uiCounter;
    }
    EZ_TEST_INT(uiCounter, table1.GetCount());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Move Copy Constructor/Assignment")
  {
    ezHashSet<st> set1;
    for (ezInt32 i = 0; i < 64; ++i)
    {
      set1.Insert(ezConstructionCounter(i));
    }

    ezUInt64 memoryUsage = set1.GetHeapMemoryUsage();

    ezHashSet<st> set2;
    set2 = std::move(set1);

    EZ_TEST_INT(set1.GetCount(), 0);
    EZ_TEST_INT(set1.GetHeapMemoryUsage(), 0);
    EZ_TEST_INT(set2.GetCount(), 64);
    EZ_TEST_INT(set2.GetHeapMemoryUsage(), memoryUsage);

    ezHashSet<st> set3(std::move(set2));

    EZ_TEST_INT(set2.GetCount(), 0);
    EZ_TEST_INT(set2.GetHeapMemoryUsage(), 0);
    EZ_TEST_INT(set3.GetCount(), 64);
    EZ_TEST_INT(set3.GetHeapMemoryUsage(), memoryUsage);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Collision Tests")
  {
    ezHashSet<Collision> set2;

    set2.Insert(Collision(0, 0));
    set2.Insert(Collision(1, 1));
    set2.Insert(Collision(0, 2));
    set2.Insert(Collision(1, 3));
    set2.Insert(Collision(1, 4));
    set2.Insert(Collision(0, 5));

    EZ_TEST_BOOL(set2.Contains(Collision(0, 0)));
    EZ_TEST_BOOL(set2.Contains(Collision(1, 1)));
    EZ_TEST_BOOL(set2.Contains(Collision(0, 2)));
    EZ_TEST_BOOL(set2.Contains(Collision(1, 3)));
    EZ_TEST_BOOL(set2.Contains(Collision(1, 4)));
    EZ_TEST_BOOL(set2.Contains(Collision(0, 5)));

    EZ_TEST_BOOL(set2.Remove(Collision(0, 0)));
    EZ_TEST_BOOL(set2.Remove(Collision(1, 1)));

    EZ_TEST_BOOL(!set2.Contains(Collision(0, 0)));
    EZ_TEST_BOOL(!set2.Contains(Collision(1, 1)));
    EZ_TEST_BOOL(set2.Contains(Collision(0, 2)));
    EZ_TEST_BOOL(set2.Contains(Collision(1, 3)));
    EZ_TEST_BOOL(set2.Contains(Collision(1, 4)));
    EZ_TEST_BOOL(set2.Contains(Collision(0, 5)));

    set2.Insert(Collision(0, 6));
    set2.Insert(Collision(1, 7));

    EZ_TEST_BOOL(set2.Contains(Collision(0, 2)));
    EZ_TEST_BOOL(set2.Contains(Collision(1, 3)));
    EZ_TEST_BOOL(set2.Contains(Collision(1, 4)));
    EZ_TEST_BOOL(set2.Contains(Collision(0, 5)));
    EZ_TEST_BOOL(set2.Contains(Collision(0, 6)));
    EZ_TEST_BOOL(set2.Contains(Collision(1, 7)));

    EZ_TEST_BOOL(set2.Remove(Collision(1, 4)));
    EZ_TEST_BOOL(set2.Remove(Collision(0, 6)));

    EZ_TEST_BOOL(!set2.Contains(Collision(1, 4)));
    EZ_TEST_BOOL(!set2.Contains(Collision(0, 6)));
    EZ_TEST_BOOL(set2.Contains(Collision(0, 2)));
    EZ_TEST_BOOL(set2.Contains(Collision(1, 3)));
    EZ_TEST_BOOL(set2.Contains(Collision(0, 5)));
    EZ_TEST_BOOL(set2.Contains(Collision(1, 7)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Clear")
  {
    EZ_TEST_BOOL(st::HasAllDestructed());

    {
      ezHashSet<st> m1;
      m1.Insert(st(1));
      EZ_TEST_BOOL(st::HasDone(2, 1)); // for inserting new elements 1 temporary is created (and destroyed)

      m1.Insert(st(3));
      EZ_TEST_BOOL(st::HasDone(2, 1)); // for inserting new elements 2 temporary is created (and destroyed)

      m1.Insert(st(1));
      EZ_TEST_BOOL(st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      EZ_TEST_BOOL(st::HasDone(0, 2));
      EZ_TEST_BOOL(st::HasAllDestructed());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Insert")
  {
    ezHashSet<ezInt32> a1;

    for (ezInt32 i = 0; i < 10; ++i)
    {
      EZ_TEST_BOOL(!a1.Insert(i));
    }

    for (ezInt32 i = 0; i < 10; ++i)
    {
      EZ_TEST_BOOL(a1.Insert(i));
    }
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Move Insert")
  {
    OnlyMovable noCopyObject(42);

    ezHashSet<OnlyMovable> noCopyKey;
    // noCopyKey.Insert(noCopyObject); // Should not compile
    noCopyKey.Insert(std::move(noCopyObject));
    EZ_TEST_INT(noCopyObject.m_NumTimesMoved, 1);
    EZ_TEST_BOOL(noCopyKey.Contains(noCopyObject));
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove/Compact")
  {
    ezHashSet<ezInt32> a;

    EZ_TEST_BOOL(a.GetHeapMemoryUsage() == 0);

    for (ezInt32 i = 0; i < 1000; ++i)
    {
      a.Insert(i);
      EZ_TEST_INT(a.GetCount(), i + 1);
    }

    EZ_TEST_BOOL(a.GetHeapMemoryUsage() >= 1000 * (sizeof(ezInt32)));

    a.Compact();

    for (ezInt32 i = 0; i < 500; ++i)
    {
      EZ_TEST_BOOL(a.Remove(i));
    }

    a.Compact();

    for (ezInt32 i = 500; i < 1000; ++i)
    {
      EZ_TEST_BOOL(a.Contains(i));
    }

    a.Clear();
    a.Compact();

    EZ_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove (Iterator)")
  {
    ezHashSet<ezInt32> a;

    EZ_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
    for (ezInt32 i = 0; i < 1000; ++i)
      a.Insert(i);

    ezHashSet<ezInt32>::ConstIterator it = a.GetIterator();

    for (ezInt32 i = 0; i < 1000 - 1; ++i)
    {
      ezInt32 value = it.Key();
      it = a.Remove(it);
      EZ_TEST_BOOL(!a.Contains(value));
      EZ_TEST_BOOL(it.IsValid());
      EZ_TEST_INT(a.GetCount(), 1000 - 1 - i);
    }
    it = a.Remove(it);
    EZ_TEST_BOOL(!it.IsValid());
    EZ_TEST_BOOL(a.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Set Operations")
  {
    ezHashSet<ezUInt32> base;
    base.Insert(1);
    base.Insert(3);
    base.Insert(5);

    ezHashSet<ezUInt32> empty;

    ezHashSet<ezUInt32> disjunct;
    disjunct.Insert(2);
    disjunct.Insert(4);
    disjunct.Insert(6);

    ezHashSet<ezUInt32> subSet;
    subSet.Insert(1);
    subSet.Insert(5);

    ezHashSet<ezUInt32> superSet;
    superSet.Insert(1);
    superSet.Insert(3);
    superSet.Insert(5);
    superSet.Insert(7);

    ezHashSet<ezUInt32> nonDisjunctNonEmptySubSet;
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
      ezHashSet<ezUInt32> res;

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
      ezHashSet<ezUInt32> res;
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
      ezHashSet<ezUInt32> res;
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator==/!=")
  {
    ezStaticArray<ezInt32, 64> keys[2];

    for (ezUInt32 i = 0; i < 64; ++i)
    {
      keys[0].PushBack(rand());
    }

    keys[1] = keys[0];

    ezHashSet<ezInt32> t[2];

    for (ezUInt32 i = 0; i < 2; ++i)
    {
      while (!keys[i].IsEmpty())
      {
        const ezUInt32 uiIndex = rand() % keys[i].GetCount();
        const ezInt32 key = keys[i][uiIndex];
        t[i].Insert(key);

        keys[i].RemoveAtAndSwap(uiIndex);
      }
    }

    EZ_TEST_BOOL(t[0] == t[1]);

    t[0].Insert(32);
    EZ_TEST_BOOL(t[0] != t[1]);

    t[1].Insert(32);
    EZ_TEST_BOOL(t[0] == t[1]);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompatibleKeyType")
  {
    ezProxyAllocator testAllocator("Test", ezFoundation::GetDefaultAllocator());
    ezLocalAllocatorWrapper allocWrapper(&testAllocator);
    using TestString = ezHybridString<32, ezLocalAllocatorWrapper>;

    ezHashSet<TestString> stringSet;
    const char* szChar = "VeryLongStringDefinitelyMoreThan32Chars1111elf!!!!";
    const char* szString = "AnotherVeryLongStringThisTimeUsedForStringView!!!!";
    ezStringView sView(szString);
    ezStringBuilder sBuilder("BuilderAlsoNeedsToBeAVeryLongStringToTriggerAllocation");
    ezString sString("String");
    EZ_TEST_BOOL(!stringSet.Insert(szChar));
    EZ_TEST_BOOL(!stringSet.Insert(sView));
    EZ_TEST_BOOL(!stringSet.Insert(sBuilder));
    EZ_TEST_BOOL(!stringSet.Insert(sString));
    EZ_TEST_BOOL(stringSet.Insert(szString));

    ezUInt64 oldAllocCount = testAllocator.GetStats().m_uiNumAllocations;

    EZ_TEST_BOOL(stringSet.Contains(szChar));
    EZ_TEST_BOOL(stringSet.Contains(sView));
    EZ_TEST_BOOL(stringSet.Contains(sBuilder));
    EZ_TEST_BOOL(stringSet.Contains(sString));

    EZ_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

    EZ_TEST_BOOL(stringSet.Remove(szChar));
    EZ_TEST_BOOL(stringSet.Remove(sView));
    EZ_TEST_BOOL(stringSet.Remove(sBuilder));
    EZ_TEST_BOOL(stringSet.Remove(sString));

    EZ_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Swap")
  {
    ezStringBuilder tmp;
    ezHashSet<ezString> set1;
    ezHashSet<ezString> set2;

    for (ezUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      set1.Insert(tmp);

      tmp.SetFormat("{0}{0}{0}", i);
      set2.Insert(tmp);
    }

    set1.Swap(set2);

    for (ezUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      EZ_TEST_BOOL(set2.Contains(tmp));

      tmp.SetFormat("{0}{0}{0}", i);
      EZ_TEST_BOOL(set1.Contains(tmp));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "foreach")
  {
    ezStringBuilder tmp;
    ezHashSet<ezString> set;
    ezHashSet<ezString> set2;

    for (ezUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      set.Insert(tmp);
    }

    EZ_TEST_INT(set.GetCount(), 1000);

    set2 = set;
    EZ_TEST_INT(set2.GetCount(), set.GetCount());

    for (ezHashSet<ezString>::ConstIterator it = begin(set); it != end(set); ++it)
    {
      const ezString& k = it.Key();
      set2.Remove(k);
    }

    EZ_TEST_BOOL(set2.IsEmpty());
    set2 = set;

    for (auto key : set)
    {
      set2.Remove(key);
    }

    EZ_TEST_BOOL(set2.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Find")
  {
    ezStringBuilder tmp;
    ezHashSet<ezString> set;

    for (ezUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      set.Insert(tmp);
    }

    for (ezInt32 i = set.GetCount() - 1; i > 0; --i)
    {
      tmp.SetFormat("stuff{}bla", i);

      auto it = set.Find(tmp);

      EZ_TEST_STRING(it.Key(), tmp);

      int allowedIterations = set.GetCount();
      for (auto it2 = it; it2.IsValid(); ++it2)
      {
        // just test that iteration is possible and terminates correctly
        --allowedIterations;
        EZ_TEST_BOOL(allowedIterations >= 0);
      }

      set.Remove(it);
    }
  }
}
