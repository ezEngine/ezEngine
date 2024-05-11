#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/ConditionVariable.h>

#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Types/UniquePtr.h>

namespace
{
  class TestThread : public ezThread
  {
  public:
    TestThread()
      : ezThread("Test Thread")
    {
    }

    ezConditionVariable* m_pCV = nullptr;
    ezAtomicInteger32* m_pCounter = nullptr;

    virtual ezUInt32 Run()
    {
      EZ_LOCK(*m_pCV);

      m_pCounter->Decrement();

      m_pCV->UnlockWaitForSignalAndLock();

      m_pCounter->Increment();
      return 0;
    }
  };

  class TestThreadTimeout : public ezThread
  {
  public:
    TestThreadTimeout()
      : ezThread("Test Thread Timeout")
    {
    }

    ezConditionVariable* m_pCV = nullptr;
    ezConditionVariable* m_pCVTimeout = nullptr;
    ezAtomicInteger32* m_pCounter = nullptr;

    virtual ezUInt32 Run()
    {
      // make sure all threads are put to sleep first
      {
        EZ_LOCK(*m_pCV);
        m_pCounter->Decrement();
        m_pCV->UnlockWaitForSignalAndLock();
      }

      // this condition will never be met during the test
      // it should always run into the timeout
      EZ_LOCK(*m_pCVTimeout);
      m_pCVTimeout->UnlockWaitForSignalAndLock(ezTime::MakeFromSeconds(0.5));

      m_pCounter->Increment();
      return 0;
    }
  };
} // namespace

EZ_CREATE_SIMPLE_TEST(Threading, ConditionalVariable)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Wait No Timeout")
  {
    constexpr ezUInt32 uiNumThreads = 32;

    ezUniquePtr<TestThread> pTestThreads[uiNumThreads];
    ezAtomicInteger32 iCounter = uiNumThreads;
    ezConditionVariable cv;

    for (ezUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThreads[i] = EZ_DEFAULT_NEW(TestThread);
      pTestThreads[i]->m_pCounter = &iCounter;
      pTestThreads[i]->m_pCV = &cv;
      pTestThreads[i]->Start();
    }

    // wait until all threads are in waiting state
    while (true)
    {
      // We need to lock here as otherwise we could signal
      // while a thread hasn't reached the wait yet.
      EZ_LOCK(cv);
      if (iCounter == 0)
        break;

      ezThreadUtils::YieldTimeSlice();
    }

    for (ezUInt32 t = 0; t < uiNumThreads / 2; ++t)
    {
      const ezInt32 iExpected = iCounter + 1;

      cv.SignalOne();

      for (ezUInt32 a = 0; a < 1000; ++a)
      {
        ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(1));

        if (iCounter >= iExpected)
          break;
      }

      // Theoretically this could fail, if the OS doesn't wake up any other thread in time but with 1000 tries that is very unlikely.
      // On some platforms like posix it is not guaranteed that exactly one thread is woken up, so we check that at least one thread was woken up.
      EZ_TEST_BOOL(iCounter >= iExpected);
    }

    // wake up the rest
    {
      cv.SignalAll();

      for (ezUInt32 a = 0; a < 1000; ++a)
      {
        ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(1));

        if (iCounter >= (ezInt32)uiNumThreads)
          break;
      }

      // theoretically this could fail, if the OS doesn't wake up any other thread in time
      // but with 1000 tries that is very unlikely
      EZ_TEST_INT(iCounter, (ezInt32)uiNumThreads);
      EZ_TEST_BOOL(iCounter <= (ezInt32)uiNumThreads); // THIS test must never fail!
    }

    for (ezUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThreads[i]->Join();
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Wait With timeout")
  {
    constexpr ezUInt32 uiNumThreads = 16;

    ezUniquePtr<TestThreadTimeout> pTestThreads[uiNumThreads];
    ezAtomicInteger32 iCounter = uiNumThreads;
    ezConditionVariable cv;
    ezConditionVariable cvt;

    for (ezUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThreads[i] = EZ_DEFAULT_NEW(TestThreadTimeout);
      pTestThreads[i]->m_pCounter = &iCounter;
      pTestThreads[i]->m_pCV = &cv;
      pTestThreads[i]->m_pCVTimeout = &cvt;
      pTestThreads[i]->Start();
    }

    // wait until all threads are in waiting state
    while (true)
    {
      // We need to lock here as otherwise we could signal
      // while a thread hasn't reached the wait yet.
      EZ_LOCK(cv);
      if (iCounter == 0)
        break;

      ezThreadUtils::YieldTimeSlice();
    }

    // open the flood gates
    cv.SignalAll();

    // all threads should run into their timeout now
    for (ezUInt32 a = 0; a < 100; ++a)
    {
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(50));

      if (iCounter >= (ezInt32)uiNumThreads)
        break;
    }

    // theoretically this could fail, if the OS doesn't wake up any other thread in time
    // but with 100 tries that is very unlikely
    EZ_TEST_INT(iCounter, (ezInt32)uiNumThreads);
    EZ_TEST_BOOL(iCounter <= (ezInt32)uiNumThreads); // THIS test must never fail!

    for (ezUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThreads[i]->Join();
    }
  }
}
