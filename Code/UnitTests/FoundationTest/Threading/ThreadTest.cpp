#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Time/Time.h>

namespace
{
  ezInt32 g_iCrossThreadVariable = 0;
  const ezUInt32 g_uiIncrementSteps = 160000;

  class TestThread3 : public ezThread
  {
  public:
    TestThread3()
      : ezThread("Test Thread")
    {
    }

    ezMutex* m_pWaitMutex = nullptr;
    ezMutex* m_pBlockedMutex = nullptr;

    virtual ezUInt32 Run()
    {
      // test TryLock on a locked mutex
      EZ_TEST_BOOL(m_pBlockedMutex->TryLock().Failed());

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
} // namespace

EZ_CREATE_SIMPLE_TEST_GROUP(Threading);

EZ_CREATE_SIMPLE_TEST(Threading, Thread)
{
  g_iCrossThreadVariable = 0;


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Thread")
  {
    TestThread3* pTestThread31 = nullptr;
    TestThread3* pTestThread32 = nullptr;

    /// the try-catch is necessary to quiet the static code analysis
    try
    {
      pTestThread31 = new TestThread3;
      pTestThread32 = new TestThread3;
    }
    catch (...)
    {
    }

    EZ_TEST_BOOL(pTestThread31 != nullptr);
    EZ_TEST_BOOL(pTestThread32 != nullptr);

    ezMutex waitMutex, blockedMutex;
    pTestThread31->m_pWaitMutex = &waitMutex;
    pTestThread32->m_pWaitMutex = &waitMutex;

    pTestThread31->m_pBlockedMutex = &blockedMutex;
    pTestThread32->m_pBlockedMutex = &blockedMutex;

    // no one holds these mutexes yet, must succeed
    EZ_TEST_BOOL(blockedMutex.TryLock().Succeeded());
    EZ_TEST_BOOL(waitMutex.TryLock().Succeeded());

    // Both thread will increment the global variable via atomic operations
    pTestThread31->Start();
    pTestThread32->Start();

    // give the threads a bit of time to start
    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(50));

    // allow the threads to run now
    waitMutex.Unlock();

    // Main thread will also increment the test variable
    ezAtomicUtils::Increment(g_iCrossThreadVariable);

    // Join with both threads
    pTestThread31->Join();
    pTestThread32->Join();

    // we are holding the mutex, another TryLock should work
    EZ_TEST_BOOL(blockedMutex.TryLock().Succeeded());

    // The threads should have finished, no one holds the lock
    EZ_TEST_BOOL(waitMutex.TryLock().Succeeded());

    // Test deletion
    delete pTestThread31;
    delete pTestThread32;

    EZ_TEST_INT(g_iCrossThreadVariable, g_uiIncrementSteps * 2 + 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Thread Sleeping")
  {
    const ezTime start = ezTime::Now();

    ezTime sleepTime(ezTime::MakeFromSeconds(0.3));

    ezThreadUtils::Sleep(sleepTime);

    const ezTime stop = ezTime::Now();

    const ezTime duration = stop - start;

    // We test for 0.25 - 0.35 since the threading functions are a bit varying in their precision
    EZ_TEST_BOOL(duration.GetSeconds() > 0.25);
    EZ_TEST_BOOL_MSG(duration.GetSeconds() < 1.0, "This test can fail when the machine is under too much load and blocks the process for too long.");
  }
}
