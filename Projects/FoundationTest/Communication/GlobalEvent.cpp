#include <PCH.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>

static ezInt32 iTestData1 = 0;
static ezInt32 iTestData2 = 0;

// The following event handlers are automatically registered, nothing else needs to be done here

EZ_ON_GLOBAL_EVENT(TestGlobalEvent1)
{
  iTestData1 += param0.GetInt();
}

EZ_ON_GLOBAL_EVENT(TestGlobalEvent2)
{
  iTestData2 += param0.GetInt();
}

EZ_ON_GLOBAL_EVENT_ONCE(TestGlobalEvent3)
{
  // this handler will be executed only once, even if the event is broadcasted multiple times
  iTestData2 += 42;
}

EZ_CREATE_SIMPLE_TEST(Communication, GlobalEvent)
{
  iTestData1 = 0;
  iTestData2 = 0;

  EZ_TEST_INT(iTestData1, 0);
  EZ_TEST_INT(iTestData2, 0);

  ezGlobalEvent::Broadcast("TestGlobalEvent1", 1);

  EZ_TEST_INT(iTestData1, 1);
  EZ_TEST_INT(iTestData2, 0);

  ezGlobalEvent::Broadcast("TestGlobalEvent1", 2);

  EZ_TEST_INT(iTestData1, 3);
  EZ_TEST_INT(iTestData2, 0);

  ezGlobalEvent::Broadcast("TestGlobalEvent1", 3);

  EZ_TEST_INT(iTestData1, 6);
  EZ_TEST_INT(iTestData2, 0);

  ezGlobalEvent::Broadcast("TestGlobalEvent2", 4);

  EZ_TEST_INT(iTestData1, 6);
  EZ_TEST_INT(iTestData2, 4);

  ezGlobalEvent::Broadcast("TestGlobalEvent3", 4);

  EZ_TEST_INT(iTestData1, 6);
  EZ_TEST_INT(iTestData2, 46);
  
  ezGlobalEvent::Broadcast("TestGlobalEvent2", 5);

  EZ_TEST_INT(iTestData1, 6);
  EZ_TEST_INT(iTestData2, 51);

  ezGlobalEvent::Broadcast("TestGlobalEvent3", 4);

  EZ_TEST_INT(iTestData1, 6);
  EZ_TEST_INT(iTestData2, 51);

  ezGlobalEvent::Broadcast("TestGlobalEvent2", 6);

  EZ_TEST_INT(iTestData1, 6);
  EZ_TEST_INT(iTestData2, 57);

  ezGlobalEvent::Broadcast("TestGlobalEvent3", 4);

  EZ_TEST_INT(iTestData1, 6);
  EZ_TEST_INT(iTestData2, 57);

  ezLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);

  ezGlobalEvent::PrintGlobalEventStatistics();

  ezLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
}
