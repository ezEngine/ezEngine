#include <PCH.h>

#include <Foundation/Threading/Thread.h>

namespace AtomicsTestDetail
{
  ezAtomicInteger32 g_iIncVariable = 0;
  ezAtomicInteger32 g_iDecVariable = 0;

  ezAtomicInteger32 g_iAddVariable = 0;
  ezAtomicInteger32 g_iSubVariable = 0;

  ezAtomicInteger32 g_iAndVariable = 0xFF;
  ezAtomicInteger32 g_iOrVariable = 1;
  ezAtomicInteger32 g_iXorVariable = 3;

  ezAtomicInteger32 g_iMinVariable = 100;
  ezAtomicInteger32 g_iMaxVariable = -100;

  ezAtomicInteger32 g_iSetVariable = 0;

  ezAtomicInteger32 g_iCompSwapVariable = 0;
  ezInt32 g_iCompSwapCounter = 0;

  ezAtomicInteger64 g_iIncVariable64 = 0;
  ezAtomicInteger64 g_iDecVariable64 = 0;

  ezAtomicInteger64 g_iAddVariable64 = 0;
  ezAtomicInteger64 g_iSubVariable64 = 0;

  ezAtomicInteger64 g_iAndVariable64 = 0xFF;
  ezAtomicInteger64 g_iOrVariable64 = 1;
  ezAtomicInteger64 g_iXorVariable64 = 3;

  ezAtomicInteger64 g_iMinVariable64 = 100;
  ezAtomicInteger64 g_iMaxVariable64 = -100;

  ezAtomicInteger64 g_iSetVariable64 = 0;

  ezAtomicInteger64 g_iCompSwapVariable64 = 0;
  ezInt32 g_iCompSwapCounter64 = 0;

  void* g_pCompSwapPointer = nullptr;
  ezInt32 g_iCompSwapPointerCounter = 0;


  class TestThread : public ezThread
  {
  public:
    TestThread(ezInt32 iIndex)
        : ezThread("Test Thread")
        , m_iIndex(iIndex)
    {
    }

    virtual ezUInt32 Run()
    {
      g_iIncVariable.Increment();
      g_iDecVariable.Decrement();

      g_iAddVariable.Add(m_iIndex);
      g_iSubVariable.Subtract(m_iIndex);

      const ezInt32 iBit = 1 << m_iIndex;
      g_iAndVariable.And(iBit);
      g_iOrVariable.Or(iBit);
      g_iXorVariable.Xor(iBit);

      g_iMinVariable.Min(m_iIndex);
      g_iMaxVariable.Max(m_iIndex);

      g_iSetVariable.Set(m_iIndex);

      if (g_iCompSwapVariable.TestAndSet(0, m_iIndex))
      {
        ++g_iCompSwapCounter;
      }

      g_iIncVariable64.Increment();
      g_iDecVariable64.Decrement();

      g_iAddVariable64.Add(m_iIndex);
      g_iSubVariable64.Subtract(m_iIndex);

      g_iAndVariable64.And(iBit);
      g_iOrVariable64.Or(iBit);
      g_iXorVariable64.Xor(iBit);

      g_iMinVariable64.Min(m_iIndex);
      g_iMaxVariable64.Max(m_iIndex);

      g_iSetVariable64.Set(m_iIndex);

      if (g_iCompSwapVariable64.TestAndSet(0, m_iIndex))
      {
        ++g_iCompSwapCounter64;
      }

      if (ezAtomicUtils::TestAndSet(&g_pCompSwapPointer, nullptr, this))
      {
        ++g_iCompSwapPointerCounter;
      }

      return 0;
    }

  private:
    ezInt32 m_iIndex;
  };
}

EZ_CREATE_SIMPLE_TEST(Threading, Atomics)
{
  // Initialization
  {
    AtomicsTestDetail::g_iIncVariable = 0;
    AtomicsTestDetail::g_iDecVariable = 0;

    AtomicsTestDetail::g_iAddVariable = 0;
    AtomicsTestDetail::g_iSubVariable = 0;

    AtomicsTestDetail::g_iAndVariable = 0xFF;
    AtomicsTestDetail::g_iOrVariable = 1;
    AtomicsTestDetail::g_iXorVariable = 3;

    AtomicsTestDetail::g_iMinVariable = 100;
    AtomicsTestDetail::g_iMaxVariable = -100;

    AtomicsTestDetail::g_iSetVariable = 0;

    AtomicsTestDetail::g_iCompSwapVariable = 0;
    AtomicsTestDetail::g_iCompSwapCounter = 0;

    AtomicsTestDetail::g_iIncVariable64 = 0;
    AtomicsTestDetail::g_iDecVariable64 = 0;

    AtomicsTestDetail::g_iAddVariable64 = 0;
    AtomicsTestDetail::g_iSubVariable64 = 0;

    AtomicsTestDetail::g_iAndVariable64 = 0xFF;
    AtomicsTestDetail::g_iOrVariable64 = 1;
    AtomicsTestDetail::g_iXorVariable64 = 3;

    AtomicsTestDetail::g_iMinVariable64 = 100;
    AtomicsTestDetail::g_iMaxVariable64 = -100;

    AtomicsTestDetail::g_iSetVariable64 = 0;

    AtomicsTestDetail::g_iCompSwapVariable64 = 0;
    AtomicsTestDetail::g_iCompSwapCounter64 = 0;

    AtomicsTestDetail::g_pCompSwapPointer = nullptr;
    AtomicsTestDetail::g_iCompSwapPointerCounter = 0;
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Thread")
  {
    AtomicsTestDetail::TestThread* pTestThread = nullptr;
    AtomicsTestDetail::TestThread* pTestThread2 = nullptr;

    /// the try-catch is necessary to quiet the static code analysis
#if EZ_ENABLED(EZ_TESTFRAMEWORK_SUPPORT_EXCEPTIONS)
    try
#endif
    {
      pTestThread = new AtomicsTestDetail::TestThread(1);
      pTestThread2 = new AtomicsTestDetail::TestThread(2);
    }
#if EZ_ENABLED(EZ_TESTFRAMEWORK_SUPPORT_EXCEPTIONS)
    catch (...)
    {
    }
#endif

    EZ_TEST_BOOL(pTestThread != nullptr);
    EZ_TEST_BOOL(pTestThread2 != nullptr);

    // Both thread will increment via atomic operations the global variable
    pTestThread->Start();
    pTestThread2->Start();

    // Join with both threads
    pTestThread->Join();
    pTestThread2->Join();

    // Test deletion
    delete pTestThread;
    delete pTestThread2;

    EZ_TEST_INT(AtomicsTestDetail::g_iIncVariable, 2);
    EZ_TEST_INT(AtomicsTestDetail::g_iDecVariable, -2);

    EZ_TEST_INT(AtomicsTestDetail::g_iAddVariable, 3);
    EZ_TEST_INT(AtomicsTestDetail::g_iSubVariable, -3);

    EZ_TEST_INT(AtomicsTestDetail::g_iAndVariable, 0);
    EZ_TEST_INT(AtomicsTestDetail::g_iOrVariable, 7);
    EZ_TEST_INT(AtomicsTestDetail::g_iXorVariable, 5);

    EZ_TEST_INT(AtomicsTestDetail::g_iMinVariable, 1);
    EZ_TEST_INT(AtomicsTestDetail::g_iMaxVariable, 2);

    EZ_TEST_BOOL(AtomicsTestDetail::g_iSetVariable > 0);

    EZ_TEST_BOOL(AtomicsTestDetail::g_iCompSwapVariable > 0);
    EZ_TEST_INT(AtomicsTestDetail::g_iCompSwapCounter, 1); // only one thread should have set the variable

    EZ_TEST_INT(AtomicsTestDetail::g_iIncVariable64, 2);
    EZ_TEST_INT(AtomicsTestDetail::g_iDecVariable64, -2);

    EZ_TEST_INT(AtomicsTestDetail::g_iAddVariable64, 3);
    EZ_TEST_INT(AtomicsTestDetail::g_iSubVariable64, -3);

    EZ_TEST_INT(AtomicsTestDetail::g_iAndVariable64, 0);
    EZ_TEST_INT(AtomicsTestDetail::g_iOrVariable64, 7);
    EZ_TEST_INT(AtomicsTestDetail::g_iXorVariable64, 5);

    EZ_TEST_INT(AtomicsTestDetail::g_iMinVariable64, 1);
    EZ_TEST_INT(AtomicsTestDetail::g_iMaxVariable64, 2);

    EZ_TEST_BOOL(AtomicsTestDetail::g_iSetVariable64 > 0);

    EZ_TEST_BOOL(AtomicsTestDetail::g_iCompSwapVariable64 > 0);
    EZ_TEST_INT(AtomicsTestDetail::g_iCompSwapCounter64, 1); // only one thread should have set the variable

    EZ_TEST_BOOL(AtomicsTestDetail::g_pCompSwapPointer != nullptr);
    EZ_TEST_INT(AtomicsTestDetail::g_iCompSwapPointerCounter, 1); // only one thread should have set the variable
  }
}
