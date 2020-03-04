#include <FoundationTestPCH.h>

#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/UniquePtr.h>

namespace
{
  class TestThread2 : public ezThread
  {
  public:
    TestThread2()
      : ezThread("Test Thread")
    {
    }

    ezThreadSignal* m_pSignalAuto = nullptr;
    ezThreadSignal* m_pSignalManual = nullptr;
    ezAtomicInteger32* m_pCounter = nullptr;
    bool m_bTimeout = false;

    virtual ezUInt32 Run()
    {
      m_pCounter->Decrement();

      m_pSignalAuto->WaitForSignal();

      m_pCounter->Increment();

      if (m_bTimeout)
      {
        m_pSignalManual->WaitForSignal(ezTime::Seconds(0.5));
      }
      else
      {
        m_pSignalManual->WaitForSignal();
      }

      m_pCounter->Increment();

      return 0;
    }
  };
} // namespace

EZ_CREATE_SIMPLE_TEST(Threading, ThreadSignal)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Wait No Timeout")
  {
    constexpr ezUInt32 uiNumThreads = 32;

    ezUniquePtr<TestThread2> pTestThread2s[uiNumThreads];
    ezAtomicInteger32 iCounter = uiNumThreads;
    ezThreadSignal sigAuto(ezThreadSignal::Mode::AutoReset);
    ezThreadSignal sigManual(ezThreadSignal::Mode::ManualReset);

    for (ezUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThread2s[i] = EZ_DEFAULT_NEW(TestThread2);
      pTestThread2s[i]->m_pCounter = &iCounter;
      pTestThread2s[i]->m_pSignalAuto = &sigAuto;
      pTestThread2s[i]->m_pSignalManual = &sigManual;
      pTestThread2s[i]->Start();
    }

    // wait until all threads are in waiting state
    while (iCounter > 0)
    {
      ezThreadUtils::YieldTimeSlice();
    }

    for (ezUInt32 t = 0; t < uiNumThreads; ++t)
    {
      const ezInt32 iExpected = t + 1;

      sigAuto.RaiseSignal();

      for (ezUInt32 a = 0; a < 1000; ++a)
      {
        ezThreadUtils::Sleep(ezTime::Milliseconds(1));

        if (iCounter >= iExpected)
          break;
      }

      // theoretically this could fail, if the OS doesn't wake up any other thread in time
      // but with 1000 tries that is very unlikely
      EZ_TEST_INT(iCounter, iExpected);
      EZ_TEST_BOOL(iCounter <= iExpected); // THIS test must never fail!
    }

    // wake up the rest
    {
      sigManual.RaiseSignal();

      for (ezUInt32 a = 0; a < 1000; ++a)
      {
        ezThreadUtils::Sleep(ezTime::Milliseconds(1));

        if (iCounter >= (ezInt32)uiNumThreads * 2)
          break;
      }

      // theoretically this could fail, if the OS doesn't wake up any other thread in time
      // but with 1000 tries that is very unlikely
      EZ_TEST_INT(iCounter, (ezInt32)uiNumThreads * 2);
      EZ_TEST_BOOL(iCounter <= (ezInt32)uiNumThreads * 2); // THIS test must never fail!
    }

    for (ezUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThread2s[i]->Join();
    }
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Wait With Timeout")
  {
    constexpr ezUInt32 uiNumThreads = 16;

    ezUniquePtr<TestThread2> pTestThread2s[uiNumThreads];
    ezAtomicInteger32 iCounter = uiNumThreads;
    ezThreadSignal sigAuto(ezThreadSignal::Mode::AutoReset);
    ezThreadSignal sigManual(ezThreadSignal::Mode::ManualReset);

    for (ezUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThread2s[i] = EZ_DEFAULT_NEW(TestThread2);
      pTestThread2s[i]->m_pCounter = &iCounter;
      pTestThread2s[i]->m_pSignalAuto = &sigAuto;
      pTestThread2s[i]->m_pSignalManual = &sigManual;
      pTestThread2s[i]->m_bTimeout = true;
      pTestThread2s[i]->Start();
    }

    // wait until all threads are in waiting state
    while (iCounter > 0)
    {
      ezThreadUtils::YieldTimeSlice();
    }

    // raise the signal N times
    for (ezUInt32 t = 0; t < uiNumThreads; ++t)
    {
      sigAuto.RaiseSignal();

      for (ezUInt32 a = 0; a < 1000; ++a)
      {
        ezThreadUtils::Sleep(ezTime::Milliseconds(1));

        if (iCounter >= (ezInt32)t + 1)
          break;
      }
    }

    // due to the wait timeout in the thread, testing this exact value here would be unreliable
    //EZ_TEST_INT(iCounter, (ezInt32)uiNumThreads);

    // just wait for the rest
    {
      for (ezUInt32 a = 0; a < 100; ++a)
      {
        ezThreadUtils::Sleep(ezTime::Milliseconds(50));

        if (iCounter >= (ezInt32)uiNumThreads * 2)
          break;
      }

      // theoretically this could fail, if the OS doesn't wake up any other thread in time
      // but with 1000 tries that is very unlikely
      EZ_TEST_INT(iCounter, (ezInt32)uiNumThreads * 2);
      EZ_TEST_BOOL(iCounter <= (ezInt32)uiNumThreads * 2); // THIS test must never fail!
    }

    for (ezUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThread2s[i]->Join();
    }
  }
}
