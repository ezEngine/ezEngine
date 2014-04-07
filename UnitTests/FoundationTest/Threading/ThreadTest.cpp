#include <PCH.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Time/Time.h>

namespace 
{
  volatile ezInt32 g_iCrossThreadVariable = 0;
  const ezUInt32 g_uiIncrementSteps = 160000;

  ezProfilingId g_TestThreadProfilingId = ezProfilingSystem::CreateId("Test Thread::Run");

  class TestThread : public ezThread
  {
  public:

    TestThread()
      : ezThread("Test Thread")
    {
    }

    virtual ezUInt32 Run()
    {
      EZ_PROFILE(g_TestThreadProfilingId);

      for (ezUInt32 i = 0; i < g_uiIncrementSteps; i++)
      {
        ezAtomicUtils::Increment(g_iCrossThreadVariable);

        ezTime::Now();
        ezThreadUtils::YieldTimeSlice();
        ezTime::Now();
      }

      return 0;
    }
  };
}

EZ_CREATE_SIMPLE_TEST_GROUP(Threading);

EZ_CREATE_SIMPLE_TEST(Threading, Thread)
{
  g_iCrossThreadVariable = 0;


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Thread")
  {
    TestThread* pTestThread = nullptr;
    TestThread* pTestThread2 = nullptr;

    /// the try-catch is necessary to quiet the static code analysis
    try
    {
      pTestThread = new TestThread;
      pTestThread2 = new TestThread;
    }
    catch(...)
    {
    }

    EZ_TEST_BOOL(pTestThread != nullptr);
    EZ_TEST_BOOL(pTestThread2 != nullptr);

    // Both thread will increment the global variable via atomic operations
    pTestThread->Start();
    pTestThread2->Start();

    // Main thread will also increment the test variable
    ezAtomicUtils::Increment(g_iCrossThreadVariable);

    // Join with both threads
    pTestThread->Join();
    pTestThread2->Join();

    // Test deletion
    delete pTestThread;
    delete pTestThread2;

    EZ_TEST_INT(g_iCrossThreadVariable, g_uiIncrementSteps * 2 + 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Thread Sleeping")
  {
    const ezTime start = ezTime::Now();

    ezTime sleepTime(ezTime::Seconds(0.3));

    ezThreadUtils::Sleep((ezUInt32)sleepTime.GetMilliseconds());

    const ezTime stop = ezTime::Now();

    const ezTime duration = stop - start;

    // We test for 0.25 - 0.35 since the threading functions are a bit varying in their precision
    EZ_TEST_BOOL(duration.GetSeconds() > 0.25);
    EZ_TEST_BOOL_MSG(duration.GetSeconds() < 1.0, "This test can fail when the machine is under too much load and blocks the process for too long.");
  }
}
