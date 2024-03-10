#include <FoundationTest/FoundationTestPCH.h>

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

    using Event = ezEvent<ezUInt32>;
    Event m_Event;
    ezUInt32 m_uiRecursionCount;
  };
} // namespace

EZ_CREATE_SIMPLE_TEST(Communication, Event)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Basics")
  {
    using TestEvent = ezEvent<ezInt32*>;
    TestEvent e;

    Test test1;
    test1.m_iData = 3;

    Test test2;
    test2.m_iData = 5;

    ezInt32 iResult = 0;

    e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test1));
    EZ_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

    iResult = 0;
    e.Broadcast(&iResult);

    EZ_TEST_INT(iResult, 3);

    e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test2));
    EZ_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));

    iResult = 0;
    e.Broadcast(&iResult);

    EZ_TEST_INT(iResult, 8);

    e.RemoveEventHandler(TestEvent::Handler(&Test::DoStuff, &test1));
    EZ_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

    iResult = 0;
    e.Broadcast(&iResult);

    EZ_TEST_INT(iResult, 5);

    e.RemoveEventHandler(TestEvent::Handler(&Test::DoStuff, &test2));
    EZ_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));

    iResult = 0;
    e.Broadcast(&iResult);

    EZ_TEST_INT(iResult, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Unsubscribing via ID")
  {
    using TestEvent = ezEvent<ezInt32*>;
    TestEvent e;

    Test test1;
    Test test2;

    auto subId1 = e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test1));
    EZ_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

    auto subId2 = e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test2));
    EZ_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));

    e.RemoveEventHandler(subId1);
    EZ_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

    e.RemoveEventHandler(subId2);
    EZ_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Unsubscribing via Unsubscriber")
  {
    using TestEvent = ezEvent<ezInt32*>;
    TestEvent e;

    Test test1;
    Test test2;

    {
      TestEvent::Unsubscriber unsub1;

      {
        TestEvent::Unsubscriber unsub2;

        e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test1), unsub1);
        EZ_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

        e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test2), unsub2);
        EZ_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));
      }

      EZ_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));
    }

    EZ_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove while iterate")
  {
    using TestEvent = ezEvent<int, ezMutex, ezDefaultAllocatorWrapper, ezEventType::CopyOnBroadcast>;
    TestEvent e;

    ezUInt32 callMap = 0;

    ezEventSubscriptionID subscriptions[4] = {};

    subscriptions[0] = e.AddEventHandler(TestEvent::Handler([&](int i)
      { callMap |= EZ_BIT(0); }));

    subscriptions[1] = e.AddEventHandler(TestEvent::Handler([&](int i)
      {
      callMap |= EZ_BIT(1);
      e.RemoveEventHandler(subscriptions[1]); }));

    subscriptions[2] = e.AddEventHandler(TestEvent::Handler([&](int i)
      {
      callMap |= EZ_BIT(2);
      e.RemoveEventHandler(subscriptions[2]);
      e.RemoveEventHandler(subscriptions[3]); }));

    subscriptions[3] = e.AddEventHandler(TestEvent::Handler([&](int i)
      { callMap |= EZ_BIT(3); }));

    e.Broadcast(0);

    EZ_TEST_BOOL(callMap == (EZ_BIT(0) | EZ_BIT(1) | EZ_BIT(2) | EZ_BIT(3)));

    callMap = 0;
    e.Broadcast(0);
    EZ_TEST_BOOL(callMap == EZ_BIT(0));

    e.RemoveEventHandler(subscriptions[0]);
  }
}
