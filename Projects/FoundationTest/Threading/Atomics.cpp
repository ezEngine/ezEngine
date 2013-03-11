#include <TestFramework/Framework/TestFramework.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/AtomicInteger.h>

namespace
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

  ezAtomicInteger32 g_iHasWork = 0;

  ezAtomicInteger64 g_iIncVariable64 = 0;
  ezAtomicInteger64 g_iDecVariable64 = 0;

  ezAtomicInteger64 g_iAddVariable64 = 0;
  ezAtomicInteger64 g_iSubVariable64 = 0;

  ezAtomicInteger64 g_iAndVariable64 = 0xFF;
  ezAtomicInteger64 g_iOrVariable64 = 1;
  ezAtomicInteger64 g_iXorVariable64 = 3;

  ezAtomicInteger64 g_iMinVariable64 = 100;
  ezAtomicInteger64 g_iMaxVariable64 = -100;

  class TestThread : public ezThread
  {
  public:

    TestThread(ezInt32 iIndex) : ezThread("Test Thread"), m_iIndex(iIndex)
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

      if (m_iIndex == 1)
      {
        g_iHasWork.Swap(1);
      }
      else
      {
        while (g_iHasWork.Swap(0) == 1)
        {          
        }
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

      return 0;
    }

  private:
    ezInt32 m_iIndex;
  };
}

EZ_CREATE_SIMPLE_TEST(Threading, Atomics)
{
  EZ_TEST_BLOCK(true, "Thread")
  {
    TestThread* pTestThread = NULL;
    TestThread* pTestThread2 = NULL;

    // the try-catch is necessary to quite the static code analysis
    // TODO (Marc): The thread internally throws an exception, maybe we can convince the code analysis there, that this is not a bug.
    try
    {
      pTestThread = new TestThread(1);
      pTestThread2 = new TestThread(2);
    }
    catch(...)
    {
    }

    EZ_TEST(pTestThread != NULL);
    EZ_TEST(pTestThread2 != NULL);

    // Both thread will increment via atomic operations the global variable
    pTestThread->Start();
    pTestThread2->Start();

    // Join with both threads
    pTestThread->Join();
    pTestThread2->Join();

    // Test deletion
    delete pTestThread;
    delete pTestThread2;

    EZ_TEST_INT(g_iIncVariable, 2);
    EZ_TEST_INT(g_iDecVariable, -2);

    EZ_TEST_INT(g_iAddVariable, 3);
    EZ_TEST_INT(g_iSubVariable, -3);

    EZ_TEST_INT(g_iAndVariable, 0);
    EZ_TEST_INT(g_iOrVariable, 7);
    EZ_TEST_INT(g_iXorVariable, 5);

    EZ_TEST_INT(g_iMinVariable, 1);
    EZ_TEST_INT(g_iMaxVariable, 2);


    EZ_TEST_INT(g_iIncVariable64, 2);
    EZ_TEST_INT(g_iDecVariable64, -2);

    EZ_TEST_INT(g_iAddVariable64, 3);
    EZ_TEST_INT(g_iSubVariable64, -3);

    EZ_TEST_INT(g_iAndVariable64, 0);
    EZ_TEST_INT(g_iOrVariable64, 7);
    EZ_TEST_INT(g_iXorVariable64, 5);

    EZ_TEST_INT(g_iMinVariable64, 1);
    EZ_TEST_INT(g_iMaxVariable64, 2);
  }
}

