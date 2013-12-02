#include <PCH.h>
#include <Foundation/Communication/Event.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Communication);

namespace
{
  struct Test
  {
    void DoStuff(ezInt32* pEventData)
    {
      *pEventData += m_iData;
    }

    ezInt32 m_iData;
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
}
