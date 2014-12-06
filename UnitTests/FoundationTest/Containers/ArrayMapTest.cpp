#include <PCH.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/String.h>

EZ_CREATE_SIMPLE_TEST(Containers, ArrayMap)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Insert / Find")
  {
    ezArrayMap<ezString, ezInt32> sa;

    sa.Insert("z", 0);
    sa.Insert("y", 1);
    sa.Insert("x", 2);
    sa.Insert("c", 3);
    sa.Insert("b", 4);
    sa.Insert("a", 5);

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
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Insert / Find")
  {
    ezArrayMap<ezInt32, ezInt32> sa;

    sa.Insert(20, 0);
    sa.Insert(19, 1);
    sa.Insert(18, 2);
    sa.Insert(12, 3);
    sa.Insert(11, 4);
    sa.Insert(10, 5);

    EZ_TEST_INT(sa.Find(10), 0);
    EZ_TEST_INT(sa.Find(11), 1);
    EZ_TEST_INT(sa.Find(12), 2);
    EZ_TEST_INT(sa.Find(18), 3);
    EZ_TEST_INT(sa.Find(19), 4);
    EZ_TEST_INT(sa.Find(20), 5);

    EZ_TEST_INT(sa.GetValue(sa.Find(10)), 5);
    EZ_TEST_INT(sa.GetValue(sa.Find(11)), 4);
    EZ_TEST_INT(sa.GetValue(sa.Find(12)), 3);
    EZ_TEST_INT(sa.GetValue(sa.Find(18)), 2);
    EZ_TEST_INT(sa.GetValue(sa.Find(19)), 1);
    EZ_TEST_INT(sa.GetValue(sa.Find(20)), 0);
  }
}

