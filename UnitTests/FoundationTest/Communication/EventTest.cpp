#include <PCH.h>

#include <Foundation/Communication/Event.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Communication);

namespace
{
  struct Test
  {
    void DoStuff(ezInt32* pEventData) { *pEventData += m_iData; }

    ezInt32 m_iData;
  };

  struct TestRecursion
  {
    TestRecursion() { m_uiRecursionCount = 0; }
    void DoStuff(ezUInt32 uiRecursions)
    {
      if (m_uiRecursionCount < uiRecursions)
      {
        m_uiRecursionCount++;
        m_Event.Broadcast(uiRecursions, 10);
      }
    }

    typedef ezEvent<ezUInt32> Event;
    Event m_Event;
    ezUInt32 m_uiRecursionCount;
  };
}

EZ_CREATE_SIMPLE_TEST(Communication, Event)
{
  {
    typedef ezEvent<ezInt32*> TestEvent;
    TestEvent e;

    Test test1;
    test1.m_iData = 3;

    Test test2;
    test2.m_iData = 5;

    ezInt32 iResult = 0;

    e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test1));

    iResult = 0;
    e.Broadcast(&iResult);

    EZ_TEST_INT(iResult, 3);

    e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test2));

    iResult = 0;
    e.Broadcast(&iResult);

    EZ_TEST_INT(iResult, 8);

    e.RemoveEventHandler(TestEvent::Handler(&Test::DoStuff, &test1));

    iResult = 0;
    e.Broadcast(&iResult);

    EZ_TEST_INT(iResult, 5);

    e.RemoveEventHandler(TestEvent::Handler(&Test::DoStuff, &test2));

    iResult = 0;
    e.Broadcast(&iResult);

    EZ_TEST_INT(iResult, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Recursion")
  {
    for (ezUInt32 i = 0; i < 10; i++)
    {
      TestRecursion test;
      test.m_Event.AddEventHandler(TestRecursion::Event::Handler(&TestRecursion::DoStuff, &test));
      test.m_Event.Broadcast(i, 10);
      EZ_TEST_INT(test.m_uiRecursionCount, i);
      test.m_Event.RemoveEventHandler(TestRecursion::Event::Handler(&TestRecursion::DoStuff, &test));
    }
  }
}
