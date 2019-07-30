#include <FoundationTestPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Time/Time.h>

namespace
{
  volatile ezInt32 g_iCrossThreadVariable = 0;
  const ezUInt32 g_uiIncrementSteps = 160000;

  class TestThread : public ezThread
  {
  public:
    TestThread()
        : ezThread("Test Thread")
    {
    }

    ezMutex* m_pWaitMutex = nullptr;
    ezMutex* m_pBlockedMutex = nullptr;

    virtual ezUInt32 Run()
    {
      // test TryAcquire on a locked mutex
      EZ_TEST_BOOL(m_pBlockedMutex->TryAcquire() == false);

      {
        // enter and leave the mutex once
        EZ_LOCK(*m_pWaitMutex);
      }

      EZ_PROFILE_SCOPE("Test Thread::Run");

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
    TestThread* pTestThread1 = nullptr;
    TestThread* pTestThread2 = nullptr;

    /// the try-catch is necessary to quiet the static code analysis
#if EZ_ENABLED(EZ_TESTFRAMEWORK_SUPPORT_EXCEPTIONS)
    try
#endif
    {
      pTestThread1 = new TestThread;
      pTestThread2 = new TestThread;
    }
#if EZ_ENABLED(EZ_TESTFRAMEWORK_SUPPORT_EXCEPTIONS)
    catch (...)
    {
    }
#endif

    EZ_TEST_BOOL(pTestThread1 != nullptr);
    EZ_TEST_BOOL(pTestThread2 != nullptr);

    ezMutex waitMutex, blockedMutex;
    pTestThread1->m_pWaitMutex = &waitMutex;
    pTestThread2->m_pWaitMutex = &waitMutex;

    pTestThread1->m_pBlockedMutex = &blockedMutex;
    pTestThread2->m_pBlockedMutex = &blockedMutex;

    // no one holds these mutexes yet, must succeed
    EZ_TEST_BOOL(blockedMutex.TryAcquire() == true);
    EZ_TEST_BOOL(waitMutex.TryAcquire() == true);

    // Both thread will increment the global variable via atomic operations
    pTestThread1->Start();
    pTestThread2->Start();

    // give the threads a bit of time to start
    ezThreadUtils::Sleep(ezTime::Milliseconds(50));

    // allow the threads to run now
    waitMutex.Release();

    // Main thread will also increment the test variable
    ezAtomicUtils::Increment(g_iCrossThreadVariable);

    // Join with both threads
    pTestThread1->Join();
    pTestThread2->Join();

    // we are holding the mutex, another TryAcquire should work
    EZ_TEST_BOOL(blockedMutex.TryAcquire() == true);

    // The threads should have finished, no one holds the lock
    EZ_TEST_BOOL(waitMutex.TryAcquire() == true);

    // Test deletion
    delete pTestThread1;
    delete pTestThread2;

    EZ_TEST_INT(g_iCrossThreadVariable, g_uiIncrementSteps * 2 + 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Thread Sleeping")
  {
    const ezTime start = ezTime::Now();

    ezTime sleepTime(ezTime::Seconds(0.3));

    ezThreadUtils::Sleep(sleepTime);

    const ezTime stop = ezTime::Now();

    const ezTime duration = stop - start;

    // We test for 0.25 - 0.35 since the threading functions are a bit varying in their precision
    EZ_TEST_BOOL(duration.GetSeconds() > 0.25);
    EZ_TEST_BOOL_MSG(duration.GetSeconds() < 1.0,
                     "This test can fail when the machine is under too much load and blocks the process for too long.");
  }
}
