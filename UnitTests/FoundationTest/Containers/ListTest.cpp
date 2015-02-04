#include <PCH.h>
#include <Foundation/Containers/List.h>

typedef ezConstructionCounter st;

EZ_CREATE_SIMPLE_TEST(Containers, List)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezList<ezInt32> l;
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PushBack() / PeekBack")
  {
    ezList<ezInt32> l;
    l.PushBack();

    EZ_TEST_INT(l.GetCount(), 1);
    EZ_TEST_INT(l.PeekBack(), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PushBack(i) / GetCount")
  {
    ezList<ezInt32> l;
    EZ_TEST_BOOL(l.GetHeapMemoryUsage() == 0);

    for (ezUInt32 i = 0; i < 1000; ++i)
    {
      l.PushBack(i);

      EZ_TEST_INT(l.GetCount(), i + 1);
      EZ_TEST_INT(l.PeekBack(), i);
    }

    EZ_TEST_BOOL(l.GetHeapMemoryUsage() >= sizeof(ezInt32) * 1000);

    ezUInt32 i = 0;
    for (ezList<ezInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      EZ_TEST_INT(*it, i);
      ++i;
    }

    EZ_TEST_INT(i, 1000);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PopBack()")
  {
    ezList<ezInt32> l;

    ezInt32 i = 0;
    for (; i < 1000; ++i)
      l.PushBack(i);

    while (!l.IsEmpty())
    {
      --i;
      EZ_TEST_INT(l.PeekBack(), i);
      l.PopBack();
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PushFront() / PeekFront")
  {
    ezList<ezInt32> l;
    l.PushFront();

    EZ_TEST_INT(l.GetCount(), 1);
    EZ_TEST_INT(l.PeekFront(), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PushFront(i) / PeekFront")
  {
    ezList<ezInt32> l;

    for (ezUInt32 i = 0; i < 1000; ++i)
    {
      l.PushFront(i);

      EZ_TEST_INT(l.GetCount(), i + 1);
      EZ_TEST_INT(l.PeekFront(), i);
    }

    ezUInt32 i2 = 1000;
    for (ezList<ezInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      --i2;
      EZ_TEST_INT(*it, i2);
    }

    EZ_TEST_INT(i2, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PopFront()")
  {
    ezList<ezInt32> l;

    ezInt32 i = 0;
    for (; i < 1000; ++i)
      l.PushFront(i);

    while (!l.IsEmpty())
    {
      --i;
      EZ_TEST_INT(l.PeekFront(), i);
      l.PopFront();
    }
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Clear / IsEmpty")
  {
    ezList<ezInt32> l;

    EZ_TEST_BOOL(l.IsEmpty());

    for (ezUInt32 i = 0; i < 1000; ++i)
      l.PushBack(i);

    EZ_TEST_BOOL(!l.IsEmpty());

    l.Clear();
    EZ_TEST_BOOL(l.IsEmpty());

    for (ezUInt32 i = 0; i < 1000; ++i)
    {
      l.PushBack(i);
      EZ_TEST_BOOL(!l.IsEmpty());

      l.Clear();
      EZ_TEST_BOOL(l.IsEmpty());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator=")
  {
    ezList<ezInt32> l, l2;

    for (ezUInt32 i = 0; i < 1000; ++i)
      l.PushBack(i);

    l2 = l;

    ezUInt32 i = 0;
    for (ezList<ezInt32>::Iterator it = l2.GetIterator(); it != l2.GetEndIterator(); ++it)
    {
      EZ_TEST_INT(*it, i);
      ++i;
    }

    EZ_TEST_INT(i, 1000);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy Constructor")
  {
    ezList<ezInt32> l;

    for (ezUInt32 i = 0; i < 1000; ++i)
      l.PushBack(i);

    ezList<ezInt32> l2(l);

    ezUInt32 i = 0;
    for (ezList<ezInt32>::Iterator it = l2.GetIterator(); it != l2.GetEndIterator(); ++it)
    {
      EZ_TEST_INT(*it, i);
      ++i;
    }

    EZ_TEST_INT(i, 1000);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetCount")
  {
    ezList<ezInt32> l;
    l.SetCount(1000);
    EZ_TEST_INT(l.GetCount(), 1000);

    ezInt32 i = 1;
    for (ezList<ezInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      EZ_TEST_INT(*it, 0);
      *it = i;
      ++i;
    }

    l.SetCount(2000);
    i = 1;
    for (ezList<ezInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      if (i > 1000)
        EZ_TEST_INT(*it, 0)
      else
      EZ_TEST_INT(*it, i);

      ++i;
    }

    l.SetCount(500);
    i = 1;
    for (ezList<ezInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      EZ_TEST_INT(*it, i);
      ++i;
    }

    EZ_TEST_INT(i, 501);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Insert(item)")
  {
    ezList<ezInt32> l;

    for (ezUInt32 i = 1; i < 1000; ++i)
      l.PushBack(i);

    // create an interleaved array of values of i and i+10000
    for (ezList<ezInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      // insert before this element
      l.Insert(it, *it + 10000);
    }

    ezInt32 i = 1;
    for (ezList<ezInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      EZ_TEST_INT(*it, i + 10000);
      ++it;

      EZ_TEST_BOOL(it.IsValid());
      EZ_TEST_INT(*it, i);

      ++i;
    }

    EZ_TEST_INT(i, 1000);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove(item)")
  {
    ezList<ezInt32> l;

    ezUInt32 i = 1;
    for (; i < 1000; ++i)
      l.PushBack(i);

    // create an interleaved array of values of i and i+10000
    for (ezList<ezInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      // insert before this element
      l.Insert(it, *it + 10000);
    }

    // now remove every second element and only keep the larger values
    for (ezList<ezInt32>::Iterator it = l.GetLastIterator(); it.IsValid(); --it)
    {
      it = l.Remove(it);
      --it;
      --i;
      EZ_TEST_INT(*it, i + 10000);
    }

    i = 1;
    for (ezList<ezInt32>::Iterator it = l.GetIterator(); it.IsValid(); ++it)
    {
      EZ_TEST_INT(*it, i + 10000);
      ++i;
    }

    EZ_TEST_INT(i, 1000);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Iterator::IsValid")
  {
    ezList<ezInt32> l;

    for (ezUInt32 i = 0; i < 1000; ++i)
      l.PushBack(i);

    ezUInt32 i = 0;
    for (ezList<ezInt32>::Iterator it = l.GetIterator(); it.IsValid(); ++it)
    {
      EZ_TEST_INT(*it, i);
      ++i;
    }

    EZ_TEST_BOOL(!l.GetEndIterator().IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Element Constructions / Destructions")
  {
    EZ_TEST_BOOL(st::HasAllDestructed());

    ezList<st> l;

    EZ_TEST_BOOL(st::HasAllDestructed());

    l.PushBack();
    EZ_TEST_BOOL(st::HasDone(2, 1));

    l.PushBack(st(1));
    EZ_TEST_BOOL(st::HasDone(2, 1));

    l.SetCount(4);
    EZ_TEST_BOOL(st::HasDone(4, 2));

    l.Clear();
    EZ_TEST_BOOL(st::HasDone(0, 4));

    EZ_TEST_BOOL(st::HasAllDestructed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator == / !=")
  {
    ezList<ezInt32> l, l2;

    EZ_TEST_BOOL(l == l2);

    ezInt32 i = 0;
    for (; i < 1000; ++i)
      l.PushBack(i);

    EZ_TEST_BOOL(l != l2);

    l2 = l;

    EZ_TEST_BOOL(l == l2);
  }
}

