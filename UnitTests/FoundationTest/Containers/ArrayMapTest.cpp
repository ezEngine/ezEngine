#include <PCH.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Stopwatch.h>

EZ_CREATE_SIMPLE_TEST(Containers, ArrayMap)
{
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

    EZ_TEST_INT(sa[sa.Find("a")].value, 5);
    EZ_TEST_INT(sa[sa.Find("b")].value, 4);
    EZ_TEST_INT(sa[sa.Find("c")].value, 3);
    EZ_TEST_INT(sa[sa.Find("x")].value, 2);
    EZ_TEST_INT(sa[sa.Find("y")].value, 1);
    EZ_TEST_INT(sa[sa.Find("z")].value, 0);

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

    EZ_TEST_INT(sa[2].value, 7);
    EZ_TEST_STRING(sa[4].key, "y");
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

    sa.Remove("b");
    EZ_TEST_INT(sa.GetCount(), 2);

    EZ_TEST_INT(sa.Find("b"), ezInvalidIndex);

    EZ_TEST_INT(sa.Find("a"), 0);
    EZ_TEST_INT(sa.Find("c"), 1);

    sa.RemoveAt(1);
    EZ_TEST_INT(sa.GetCount(), 1);

    EZ_TEST_INT(sa.Find("a"), 0);
    EZ_TEST_INT(sa.Find("c"), ezInvalidIndex);

    sa.RemoveAt(0);
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

    const ezUInt32 uiElements = 100000;

    const ezTime t0 = s.Checkpoint();

    {
      sa.Reserve(uiElements);

      for (ezUInt32 i = 0; i < uiElements; ++i)
      {
        sa.Insert(uiElements - i, i*2);
      }

      sa.Sort();
    }

    const ezTime t1 = s.Checkpoint();

    {
      for (ezUInt32 i = 0; i < uiElements; ++i)
      {
        EZ_TEST_INT(sa.GetValue(sa.Find(uiElements - i)), i * 2);
      }
    }

    const ezTime t2 = s.Checkpoint();

    {
      for (ezUInt32 i = 0; i < uiElements; ++i)
      {
        map.Insert(uiElements - i, i*2);
      }
    }

    const ezTime t3 = s.Checkpoint();

    {
      for (ezUInt32 i = 0; i < uiElements; ++i)
      {
        EZ_TEST_INT(map[uiElements - i], i * 2);
      }
    }

    const ezTime t4 = s.Checkpoint();

    int breakpoint = 0;
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Lower Bound / Upper Bound")
  {
    ezArrayMap<ezInt32, ezInt32> sa;
    sa[1 ] = 23;
    sa[3 ] = 23;
    sa[4 ] = 23;
    sa[6 ] = 23;
    sa[7 ] = 23;
    sa[9 ] = 23;
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
}

