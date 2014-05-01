#include <PCH.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>

namespace
{
  typedef ezGenericId<32, 16> Id;
  typedef ezConstructionCounter st;

  struct TestObject
  {
    int x;
    ezString s;
  };
}

EZ_CREATE_SIMPLE_TEST(Containers, IdTable)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezIdTable<Id, ezInt32> table;

    EZ_TEST_BOOL(table.GetCount() == 0);
    EZ_TEST_BOOL(table.IsEmpty());

    ezUInt32 counter = 0;
    for (ezIdTable<Id, ezInt32>::ConstIterator it = table.GetIterator(); it.IsValid(); ++it)
    {
      ++counter;
    }
    EZ_TEST_INT(counter, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy Constructor/Assignment/Iterator")
  {
    EZ_TEST_BOOL(st::HasAllDestructed());
    {
      ezIdTable<Id, st> table1;

      for (ezInt32 i = 0; i < 200; ++i)
      {
        table1.Insert(st(i));
      }

      EZ_TEST_BOOL(table1.Remove(Id(0, 0)));

      for (ezInt32 i = 0; i < 99; ++i)
      {
        Id id;
      
        do
        {
          id.m_InstanceIndex = rand() % 200;
        }
        while (!table1.Contains(id));

        EZ_TEST_BOOL(table1.Remove(id));
      }

      ezIdTable<Id, st> table2;
      table2 = table1;
      ezIdTable<Id, st> table3(table1);

      EZ_TEST_BOOL(table2.IsFreelistValid());
      EZ_TEST_BOOL(table3.IsFreelistValid());

      EZ_TEST_INT(table1.GetCount(), 100);
      EZ_TEST_INT(table2.GetCount(), 100);
      EZ_TEST_INT(table3.GetCount(), 100);

      ezUInt32 uiCounter = 0;
      for (ezIdTable<Id, st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
      {
        st value;

        EZ_TEST_BOOL(table2.TryGetValue(it.Id(), value));
        EZ_TEST_BOOL(it.Value() == value);

        EZ_TEST_BOOL(table3.TryGetValue(it.Id(), value));
        EZ_TEST_BOOL(it.Value() == value);

        ++uiCounter;
      }
      EZ_TEST_INT(uiCounter, table1.GetCount());

      for (ezIdTable<Id, st>::Iterator it = table1.GetIterator(); it.IsValid(); ++it)
      {
        it.Value() = st(42);
      }

      for (ezIdTable<Id, st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
      {
        st value;

        EZ_TEST_BOOL(table1.TryGetValue(it.Id(), value));
        EZ_TEST_BOOL(it.Value() == value);
        EZ_TEST_BOOL(value.m_iData == 42);
      }
    }
    EZ_TEST_BOOL(st::HasAllDestructed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Insert/Remove")
  {
    ezIdTable<Id, TestObject> table;

    for (int i = 0; i < 100; i++)
    {
      TestObject x = { rand(), "Test" };
      Id id = table.Insert(x);
      EZ_TEST_INT(id.m_InstanceIndex, i);
      EZ_TEST_INT(id.m_Generation, 0);

      EZ_TEST_BOOL(table.Contains(id));

      TestObject y = table[id];
      EZ_TEST_INT(x.x, y.x);
      EZ_TEST_BOOL(x.s == y.s);
    }
    EZ_TEST_INT(table.GetCount(), 100);

    Id ids[10] = { Id(13, 0), Id(0, 0), Id(16, 0), Id(34, 0), Id(56, 0),
      Id(57, 0), Id(79, 0), Id(85, 0), Id(91, 0), Id(97, 0) };


    for (int i = 0; i < 10; i++)
    {
      bool res = table.Remove(ids[i]);
      EZ_TEST_BOOL(res);
      EZ_TEST_BOOL(!table.Contains(ids[i]));
    }
    EZ_TEST_INT(table.GetCount(), 90);

    for (int i = 0; i < 40; i++)
    {
      TestObject x = { 1000, "Bla. This is a very long string which does not fit into 32 byte and will cause memory allocations." };
      Id newId = table.Insert(x);

      EZ_TEST_BOOL(table.Contains(newId));

      TestObject y = table[newId];
      EZ_TEST_INT(x.x, y.x);
      EZ_TEST_BOOL(x.s == y.s);

      TestObject* pObj;
      EZ_TEST_BOOL(table.TryGetValue(newId, pObj));
      EZ_TEST_BOOL(pObj->s == x.s);
    }
    EZ_TEST_INT(table.GetCount(), 130);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Crash test")
  {
    ezIdTable<Id, TestObject> table;
    ezDynamicArray<Id> ids;
    
    for (ezUInt32 i = 0; i < 100000; ++i)
    {
      int action = rand() % 2;
      if (action == 0)
      {
        TestObject x = { rand(), "Test" };
        ids.PushBack(table.Insert(x));
      }
      else
      {
        if (ids.GetCount() > 0)
        {
          ezUInt32 index = rand() % ids.GetCount();        
          EZ_TEST_BOOL(table.Remove(ids[index]));
          ids.RemoveAtSwap(index);
        }
      }

      EZ_TEST_BOOL(table.IsFreelistValid());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Clear")
  {
    EZ_TEST_BOOL(st::HasAllDestructed());

    ezIdTable<Id, st> m1;
    Id id0 = m1.Insert(st(1));
    EZ_TEST_BOOL(st::HasDone(2, 1)); // for inserting new elements 1 temporary is created (and destroyed)

    Id id1 = m1.Insert(st(3));
    EZ_TEST_BOOL(st::HasDone(2, 1)); // for inserting new elements 1 temporary is created (and destroyed)

    m1[id0] = st(2);
    EZ_TEST_BOOL(st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

    m1.Clear();
    EZ_TEST_BOOL(st::HasDone(0, 2));
    EZ_TEST_BOOL(st::HasAllDestructed());

    EZ_TEST_BOOL(!m1.Contains(id0));
    EZ_TEST_BOOL(!m1.Contains(id1));
    EZ_TEST_BOOL(m1.IsFreelistValid());
  }

  /*EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove/Compact")
  {
    ezIdTable<Id, st> a;

    for (ezInt32 i = 0; i < 1000; ++i)
    {
      a.Insert(i);
      EZ_TEST_INT(a.GetCount(), i + 1);
    }

    a.Compact();
    EZ_TEST_BOOL(a.IsFreelistValid());

    {
      ezUInt32 i = 0;
      for (ezIdTable<Id, st>::Iterator it = a.GetIterator(); it.IsValid(); ++it)
      {
        EZ_TEST_INT(a[it.Id()].m_iData, i);
        ++i;
      }
    }

    for (ezInt32 i = 500; i < 1000; ++i)
    {
      st oldValue;
      EZ_TEST_BOOL(a.Remove(Id(i, 0), &oldValue));
      EZ_TEST_INT(oldValue.m_iData, i);
    }
    
    a.Compact();
    EZ_TEST_BOOL(a.IsFreelistValid());

    {
      ezUInt32 i = 0;
      for (ezIdTable<Id, st>::Iterator it = a.GetIterator(); it.IsValid(); ++it)
      {
        EZ_TEST_INT(a[it.Id()].m_iData, i);
        ++i;
      }
    }
  }*/
}
