#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Stopwatch.h>

EZ_CREATE_SIMPLE_TEST(Containers, ArrayMap)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Iterator")
  {
    ezArrayMap<ezUInt32, ezUInt32> m;
    for (ezUInt32 i = 0; i < 1000; ++i)
      m[i] = i + 1;

    // element with the given key (and such, value "key + 1")
    auto findable = m.Find(499u);

    // non-const
    {
      // findable
      auto itfound = std::find_if(begin(m), end(m), [](const ezArrayMap<ezUInt32, ezUInt32>::Pair& val)
        { return val.value == 500; });
      EZ_TEST_BOOL(findable == itfound->key);

      // unfindable
      itfound = std::find_if(begin(m), end(m), [](const ezArrayMap<ezUInt32, ezUInt32>::Pair& val)
        { return val.value == 1001; });
      EZ_TEST_BOOL(end(m) == itfound);
    }

    // const
    {
      // findable
      auto itfound = std::find_if(cbegin(m), cend(m), [](const ezArrayMap<ezUInt32, ezUInt32>::Pair& val)
        { return val.value == 500; });
      EZ_TEST_BOOL(findable == itfound->key);

      // unfindable
      itfound = std::find_if(cbegin(m), cend(m), [](const ezArrayMap<ezUInt32, ezUInt32>::Pair& val)
        { return val.value == 1001; });
      EZ_TEST_BOOL(cend(m) == itfound);
    }

    // non-const reverse
    {
      // findable
      auto itfound = std::find_if(rbegin(m), rend(m), [](const ezArrayMap<ezUInt32, ezUInt32>::Pair& val)
        { return val.value == 500; });
      EZ_TEST_BOOL(findable == itfound->key);

      // unfindable
      itfound = std::find_if(rbegin(m), rend(m), [](const ezArrayMap<ezUInt32, ezUInt32>::Pair& val)
        { return val.value == 1001; });
      EZ_TEST_BOOL(rend(m) == itfound);
    }

    // const reverse
    {
      // findable
      auto itfound = std::find_if(crbegin(m), crend(m), [](const ezArrayMap<ezUInt32, ezUInt32>::Pair& val)
        { return val.value == 500; });
      EZ_TEST_BOOL(findable == itfound->key);

      // unfindable
      itfound = std::find_if(crbegin(m), crend(m), [](const ezArrayMap<ezUInt32, ezUInt32>::Pair& val)
        { return val.value == 1001; });
      EZ_TEST_BOOL(crend(m) == itfound);
    }

    // forward
    ezUInt32 prev = begin(m)->key;
    for (const auto& elem : m)
    {
      EZ_TEST_BOOL(elem.value == prev + 1);
      prev = elem.value;
    }

    EZ_TEST_BOOL(prev == 1000);

    // backward
    prev = (rbegin(m))->value + 1;
    for (auto it = rbegin(m); it < rend(m); ++it)
    {
      EZ_TEST_BOOL(it->value == prev - 1);
      prev = it->value;
    }

    EZ_TEST_BOOL(prev == 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetData with Iterators")
  {
    ezArrayMap<ezUInt32, ezUInt32> m;
    for (ezUInt32 i = 0; i < 1000; ++i)
      m[i] = i + 1;

    // element with the given key (and such, value "key + 1")
    auto findable = m.Find(499u);

    // check if modification of the keys via direct data access
    // keeps iterability and access via keys intact

    // modify
    auto& data = m.GetData();
    for (auto& p : data)
    {
      p.key += 1000;
    }

    // ...and test with new key
    EZ_TEST_BOOL(m[findable + 1000] == 500);

    // and index...
    EZ_TEST_BOOL(m.GetValue(499u) == 500);

    // and old key.
    EZ_TEST_BOOL(m.Find(499u) == ezInvalidIndex);

    // findable
    auto itfound = std::find_if(begin(m), end(m), [](const ezArrayMap<ezUInt32, ezUInt32>::Pair& val)
      { return val.value == 500; });
    EZ_TEST_BOOL((findable + 1000) == itfound->key);

    // unfindable
    itfound = std::find_if(begin(m), end(m), [](const ezArrayMap<ezUInt32, ezUInt32>::Pair& val)
      { return val.value == 1001; });
    EZ_TEST_BOOL(end(m) == itfound);

    // forward
    ezUInt32 prev = 0;
    for (const auto& elem : m)
    {
      EZ_TEST_BOOL(elem.value == prev + 1);
      prev = elem.value;
    }

    EZ_TEST_BOOL(prev == 1000);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Insert / Find / Reserve / Clear / IsEmpty / Compact / GetCount")
  {
    ezArrayMap<ezString, ezInt32> sa;

    EZ_TEST_BOOL(sa.GetHeapMemoryUsage() == 0);

    EZ_TEST_INT(sa.GetCount(), 0);
    EZ_TEST_BOOL(sa.IsEmpty());

    sa.Reserve(10);

    EZ_TEST_BOOL(sa.GetHeapMemoryUsage() >= 10 * (sizeof(ezString) + sizeof(ezInt32)));

    sa.Insert("z", 0);
    sa.Insert("y", 1);
    sa.Insert("x", 2);
    sa.Insert("c", 3);
    sa.Insert("b", 4);
    sa.Insert("a", 5);

    EZ_TEST_INT(sa.GetCount(), 6);
    EZ_TEST_BOOL(!sa.IsEmpty());

    EZ_TEST_INT(sa.Find("a"), 0);
    EZ_TEST_INT(sa.Find("b"), 1);
    EZ_TEST_INT(sa.Find("c"), 2);
    EZ_TEST_INT(sa.Find("x"), 3);
    EZ_TEST_INT(sa.Find("y"), 4);
    EZ_TEST_INT(sa.Find("z"), 5);

    EZ_TEST_INT(sa.GetPair(sa.Find("a")).value, 5);
    EZ_TEST_INT(sa.GetPair(sa.Find("b")).value, 4);
    EZ_TEST_INT(sa.GetPair(sa.Find("c")).value, 3);
    EZ_TEST_INT(sa.GetPair(sa.Find("x")).value, 2);
    EZ_TEST_INT(sa.GetPair(sa.Find("y")).value, 1);
    EZ_TEST_INT(sa.GetPair(sa.Find("z")).value, 0);

    sa.Clear();
    EZ_TEST_BOOL(sa.IsEmpty());

    EZ_TEST_BOOL(sa.GetHeapMemoryUsage() > 0);
    sa.Compact();
    EZ_TEST_BOOL(sa.GetHeapMemoryUsage() == 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Insert / Find / = / == / != ")
  {
    ezArrayMap<ezInt32, ezInt32> sa, sa2;

    sa.Insert(20, 0);
    sa.Insert(19, 1);
    sa.Insert(18, 2);
    sa.Insert(12, 3);
    sa.Insert(11, 4);

    sa2 = sa;

    EZ_TEST_BOOL(sa == sa2);

    sa.Insert(10, 5);

    EZ_TEST_BOOL(sa != sa2);

    EZ_TEST_INT(sa.Find(10), 0);
    EZ_TEST_INT(sa.Find(11), 1);
    EZ_TEST_INT(sa.Find(12), 2);
    EZ_TEST_INT(sa.Find(18), 3);
    EZ_TEST_INT(sa.Find(19), 4);
    EZ_TEST_INT(sa.Find(20), 5);

    sa2.Insert(10, 5);

    EZ_TEST_BOOL(sa == sa2);

    EZ_TEST_INT(sa.GetValue(sa.Find(10)), 5);
    EZ_TEST_INT(sa.GetValue(sa.Find(11)), 4);
    EZ_TEST_INT(sa.GetValue(sa.Find(12)), 3);
    EZ_TEST_INT(sa.GetValue(sa.Find(18)), 2);
    EZ_TEST_INT(sa.GetValue(sa.Find(19)), 1);
    EZ_TEST_INT(sa.GetValue(sa.Find(20)), 0);

    EZ_TEST_BOOL(sa == sa2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains")
  {
    ezArrayMap<ezString, ezInt32> sa;

    EZ_TEST_BOOL(!sa.Contains("a"));
    EZ_TEST_BOOL(!sa.Contains("z"));

    sa.Insert("z", 0);
    sa.Insert("y", 1);
    sa.Insert("x", 2);

    EZ_TEST_BOOL(!sa.Contains("a"));
    EZ_TEST_BOOL(sa.Contains("z"));

    sa.Insert("c", 3);
    sa.Insert("b", 4);
    sa.Insert("a", 5);

    EZ_TEST_BOOL(sa.Contains("a"));
    EZ_TEST_BOOL(sa.Contains("z"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains")
  {
    ezArrayMap<ezString, ezInt32> sa;

    EZ_TEST_BOOL(!sa.Contains("a", 0));
    EZ_TEST_BOOL(!sa.Contains("z", 0));

    sa.Insert("z", 0);
    sa.Insert("y", 1);
    sa.Insert("x", 2);

    EZ_TEST_BOOL(!sa.Contains("a", 0));
    EZ_TEST_BOOL(sa.Contains("z", 0));
    EZ_TEST_BOOL(sa.Contains("y", 1));
    EZ_TEST_BOOL(sa.Contains("x", 2));

    sa.Insert("c", 3);
    sa.Insert("b", 4);
    sa.Insert("a", 5);

    EZ_TEST_BOOL(sa.Contains("a", 5));
    EZ_TEST_BOOL(sa.Contains("b", 4));
    EZ_TEST_BOOL(sa.Contains("c", 3));
    EZ_TEST_BOOL(sa.Contains("z", 0));
    EZ_TEST_BOOL(sa.Contains("y", 1));
    EZ_TEST_BOOL(sa.Contains("x", 2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetValue / GetKey / Copy Constructor")
  {
    ezArrayMap<ezString, ezInt32> sa;

    sa.Insert("z", 1);
    sa.Insert("y", 3);
    sa.Insert("x", 5);
    sa.Insert("c", 7);
    sa.Insert("b", 9);
    sa.Insert("a", 11);

    sa.Sort();

    const ezArrayMap<ezString, ezInt32> sa2(sa);

    EZ_TEST_INT(sa.GetValue(0), 11);
    EZ_TEST_INT(sa.GetValue(2), 7);

    EZ_TEST_INT(sa2.GetValue(0), 11);
    EZ_TEST_INT(sa2.GetValue(2), 7);

    EZ_TEST_STRING(sa.GetKey(1), "b");
    EZ_TEST_STRING(sa.GetKey(3), "x");

    EZ_TEST_INT(sa["b"], 9);
    EZ_TEST_INT(sa["y"], 3);

    EZ_TEST_INT(sa.GetPair(2).value, 7);
    EZ_TEST_STRING(sa.GetPair(4).key, "y");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove")
  {
    ezArrayMap<ezString, ezInt32> sa;

    bool bExisted = true;

    sa.FindOrAdd("a", &bExisted) = 2;
    EZ_TEST_BOOL(!bExisted);

    sa.FindOrAdd("b", &bExisted) = 4;
    EZ_TEST_BOOL(!bExisted);

    sa.FindOrAdd("c", &bExisted) = 6;
    EZ_TEST_BOOL(!bExisted);

    sa.FindOrAdd("b", &bExisted) = 5;
    EZ_TEST_BOOL(bExisted);

    EZ_TEST_INT(sa.GetCount(), 3);

    EZ_TEST_INT(sa.Find("a"), 0);
    EZ_TEST_INT(sa.Find("c"), 2);

    sa.RemoveAndCopy("b");
    EZ_TEST_INT(sa.GetCount(), 2);

    EZ_TEST_INT(sa.Find("b"), ezInvalidIndex);

    EZ_TEST_INT(sa.Find("a"), 0);
    EZ_TEST_INT(sa.Find("c"), 1);

    sa.RemoveAtAndCopy(1);
    EZ_TEST_INT(sa.GetCount(), 1);

    EZ_TEST_INT(sa.Find("a"), 0);
    EZ_TEST_INT(sa.Find("c"), ezInvalidIndex);

    sa.RemoveAtAndCopy(0);
    EZ_TEST_INT(sa.GetCount(), 0);

    EZ_TEST_INT(sa.Find("a"), ezInvalidIndex);
    EZ_TEST_INT(sa.Find("c"), ezInvalidIndex);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Stresstest")
  {
    // Interestingly the map is not really slower than the sorted array, at least not in debug builds

    ezStopwatch s;
    ezArrayMap<ezInt32, ezInt32> sa;
    ezMap<ezInt32, ezInt32> map;

    const ezInt32 uiElements = 100000;

    // const ezTime t0 = s.Checkpoint();

    {
      sa.Reserve(uiElements);

      for (ezUInt32 i = 0; i < uiElements; ++i)
      {
        sa.Insert(uiElements - i, i * 2);
      }

      sa.Sort();
    }

    // const ezTime t1 = s.Checkpoint();

    {
      for (ezInt32 i = 0; i < uiElements; ++i)
      {
        EZ_TEST_INT(sa.GetValue(sa.Find(uiElements - i)), i * 2);
      }
    }

    // const ezTime t2 = s.Checkpoint();

    {
      for (ezUInt32 i = 0; i < uiElements; ++i)
      {
        map.Insert(uiElements - i, i * 2);
      }
    }

    // const ezTime t3 = s.Checkpoint();

    {
      for (ezUInt32 i = 0; i < uiElements; ++i)
      {
        EZ_TEST_INT(map[uiElements - i], i * 2);
      }
    }

    // const ezTime t4 = s.Checkpoint();

    // int breakpoint = 0;
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Lower Bound / Upper Bound")
  {
    ezArrayMap<ezInt32, ezInt32> sa;
    sa[1] = 23;
    sa[3] = 23;
    sa[4] = 23;
    sa[6] = 23;
    sa[7] = 23;
    sa[9] = 23;
    sa[11] = 23;
    sa[14] = 23;
    sa[17] = 23;

    EZ_TEST_INT(sa.LowerBound(0), 0);
    EZ_TEST_INT(sa.LowerBound(1), 0);
    EZ_TEST_INT(sa.LowerBound(2), 1);
    EZ_TEST_INT(sa.LowerBound(3), 1);
    EZ_TEST_INT(sa.LowerBound(4), 2);
    EZ_TEST_INT(sa.LowerBound(5), 3);
    EZ_TEST_INT(sa.LowerBound(6), 3);
    EZ_TEST_INT(sa.LowerBound(7), 4);
    EZ_TEST_INT(sa.LowerBound(8), 5);
    EZ_TEST_INT(sa.LowerBound(9), 5);
    EZ_TEST_INT(sa.LowerBound(10), 6);
    EZ_TEST_INT(sa.LowerBound(11), 6);
    EZ_TEST_INT(sa.LowerBound(12), 7);
    EZ_TEST_INT(sa.LowerBound(13), 7);
    EZ_TEST_INT(sa.LowerBound(14), 7);
    EZ_TEST_INT(sa.LowerBound(15), 8);
    EZ_TEST_INT(sa.LowerBound(16), 8);
    EZ_TEST_INT(sa.LowerBound(17), 8);
    EZ_TEST_INT(sa.LowerBound(18), ezInvalidIndex);
    EZ_TEST_INT(sa.LowerBound(19), ezInvalidIndex);
    EZ_TEST_INT(sa.LowerBound(20), ezInvalidIndex);

    EZ_TEST_INT(sa.UpperBound(0), 0);
    EZ_TEST_INT(sa.UpperBound(1), 1);
    EZ_TEST_INT(sa.UpperBound(2), 1);
    EZ_TEST_INT(sa.UpperBound(3), 2);
    EZ_TEST_INT(sa.UpperBound(4), 3);
    EZ_TEST_INT(sa.UpperBound(5), 3);
    EZ_TEST_INT(sa.UpperBound(6), 4);
    EZ_TEST_INT(sa.UpperBound(7), 5);
    EZ_TEST_INT(sa.UpperBound(8), 5);
    EZ_TEST_INT(sa.UpperBound(9), 6);
    EZ_TEST_INT(sa.UpperBound(10), 6);
    EZ_TEST_INT(sa.UpperBound(11), 7);
    EZ_TEST_INT(sa.UpperBound(12), 7);
    EZ_TEST_INT(sa.UpperBound(13), 7);
    EZ_TEST_INT(sa.UpperBound(14), 8);
    EZ_TEST_INT(sa.UpperBound(15), 8);
    EZ_TEST_INT(sa.UpperBound(16), 8);
    EZ_TEST_INT(sa.UpperBound(17), ezInvalidIndex);
    EZ_TEST_INT(sa.UpperBound(18), ezInvalidIndex);
    EZ_TEST_INT(sa.UpperBound(19), ezInvalidIndex);
    EZ_TEST_INT(sa.UpperBound(20), ezInvalidIndex);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Duplicate Keys")
  {
    ezArrayMap<ezInt32, ezInt32> sa;

    sa.Insert(32, 1);
    sa.Insert(31, 1);
    sa.Insert(33, 1);

    sa.Insert(40, 1);
    sa.Insert(44, 1);
    sa.Insert(46, 1);

    sa.Insert(11, 1);
    sa.Insert(15, 1);
    sa.Insert(19, 1);

    sa.Insert(11, 2);
    sa.Insert(15, 2);
    sa.Insert(31, 2);
    sa.Insert(44, 2);

    sa.Insert(11, 3);
    sa.Insert(15, 3);
    sa.Insert(44, 3);

    sa.Insert(60, 1);
    sa.Insert(60, 2);
    sa.Insert(60, 3);
    sa.Insert(60, 4);
    sa.Insert(60, 5);
    sa.Insert(60, 6);
    sa.Insert(60, 7);
    sa.Insert(60, 8);
    sa.Insert(60, 9);
    sa.Insert(60, 10);

    sa.Sort();

    EZ_TEST_INT(sa.LowerBound(11), 0);
    EZ_TEST_INT(sa.LowerBound(15), 3);
    EZ_TEST_INT(sa.LowerBound(19), 6);

    EZ_TEST_INT(sa.LowerBound(31), 7);
    EZ_TEST_INT(sa.LowerBound(32), 9);
    EZ_TEST_INT(sa.LowerBound(33), 10);

    EZ_TEST_INT(sa.LowerBound(40), 11);
    EZ_TEST_INT(sa.LowerBound(44), 12);
    EZ_TEST_INT(sa.LowerBound(46), 15);

    EZ_TEST_INT(sa.LowerBound(60), 16);


    EZ_TEST_INT(sa.UpperBound(11), 3);
    EZ_TEST_INT(sa.UpperBound(15), 6);
    EZ_TEST_INT(sa.UpperBound(19), 7);

    EZ_TEST_INT(sa.UpperBound(31), 9);
    EZ_TEST_INT(sa.UpperBound(32), 10);
    EZ_TEST_INT(sa.UpperBound(33), 11);

    EZ_TEST_INT(sa.UpperBound(40), 12);
    EZ_TEST_INT(sa.UpperBound(44), 15);
    EZ_TEST_INT(sa.UpperBound(46), 16);

    EZ_TEST_INT(sa.UpperBound(60), ezInvalidIndex);
  }
}
