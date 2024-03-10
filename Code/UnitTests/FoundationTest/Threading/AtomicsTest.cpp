#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/Thread.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/VariantType.h>

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

  ezAtomicInteger32 g_iSetVariable = 0;

  ezAtomicInteger32 g_iTestAndSetVariable = 0;
  ezInt32 g_iTestAndSetCounter = 0;

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

  ezAtomicInteger64 g_iTestAndSetVariable64 = 0;
  ezInt32 g_iTestAndSetCounter64 = 0;

  ezAtomicInteger32 g_iCompareAndSwapVariable32 = 0;
  ezInt32 g_iCompareAndSwapCounter32 = 0;

  ezAtomicInteger64 g_iCompareAndSwapVariable64 = 0;
  ezInt32 g_iCompareAndSwapCounter64 = 0;

  void* g_pTestAndSetPointer = nullptr;
  ezInt32 g_iTestAndSetPointerCounter = 0;

  ezAtomicInteger<ezVariantType::Enum> g_EnumSet = ezVariantType::Bool;

  ezAtomicInteger<ezVariantType::Enum> g_EnumCompareAndSwap = ezVariantType::Bool;
  ezInt32 g_iCompareAndSwapCounterEnum = 0;

  ezAtomicInteger<ezVariantType::Enum> g_EnumTestAndSetVariable = ezVariantType::Bool;
  ezInt32 g_iTestAndSetCounterEnum = 0;

  class AtomicsTestThread : public ezThread
  {
  public:
    AtomicsTestThread(ezInt32 iIndex)
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

      if (g_iTestAndSetVariable.TestAndSet(0, m_iIndex))
      {
        ++g_iTestAndSetCounter;
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

      if (g_iTestAndSetVariable64.TestAndSet(0, m_iIndex))
      {
        ++g_iTestAndSetCounter64;
      }

      if (ezAtomicUtils::TestAndSet(&g_pTestAndSetPointer, nullptr, this))
      {
        ++g_iTestAndSetPointerCounter;
      }

      if (g_iCompareAndSwapVariable32.CompareAndSwap(0, m_iIndex) == 0)
      {
        ++g_iCompareAndSwapCounter32;
      }

      if (g_iCompareAndSwapVariable64.CompareAndSwap(0, m_iIndex) == 0)
      {
        ++g_iCompareAndSwapCounter64;
      }

      const ezVariantType::Enum targetEnum = m_iIndex == 1 ? ezVariantType::Float : ezVariantType::Color;
      g_EnumSet.Set(targetEnum);

      if (g_EnumTestAndSetVariable.TestAndSet(ezVariantType::Bool, targetEnum))
      {
        ++g_iTestAndSetCounterEnum;
      }

      if (g_EnumCompareAndSwap.CompareAndSwap(ezVariantType::Bool, targetEnum) == ezVariantType::Bool)
      {
        ++g_iCompareAndSwapCounterEnum;
      }

      return 0;
    }

  private:
    ezInt32 m_iIndex;
  };


  ezAtomicInteger32 g_iPostIncVariable32 = 0;
  ezAtomicInteger32 g_iPostDecVariable32 = 0;
  ezAtomicInteger64 g_iPostIncVariable64 = 0;
  ezAtomicInteger64 g_iPostDecVariable64 = 0;
  ezDynamicArray<ezAtomicInteger32> g_PostIncValues32;
  ezDynamicArray<ezAtomicInteger32> g_PostIncValues64;
  ezDynamicArray<ezAtomicInteger32> g_PostDecValues32;
  ezDynamicArray<ezAtomicInteger32> g_PostDecValues64;
  ezDynamicArray<ezUniquePtr<ezThread>> g_PostIncDecThreads;

  class PostIncDecThread : public ezThread
  {
  public:
    PostIncDecThread(ezInt32 iIndex)
      : ezThread("Test Thread")
      , m_iIndex(iIndex)
    {
    }

    virtual ezUInt32 Run()
    {
      g_PostIncValues32[g_iPostIncVariable32.PostIncrement()].Increment();
      g_PostIncValues64[static_cast<ezInt32>(g_iPostIncVariable64.PostIncrement())].Increment();
      g_PostDecValues32[g_iPostDecVariable32.PostDecrement()].Increment();
      g_PostDecValues64[static_cast<ezInt32>(g_iPostDecVariable64.PostDecrement())].Increment();
      return 0;
    }

  private:
    ezInt32 m_iIndex;
  };
} // namespace

EZ_CREATE_SIMPLE_TEST(Threading, Atomics)
{
  // Initialization
  {
    g_iIncVariable = 0;
    g_iDecVariable = 0;

    g_iAddVariable = 0;
    g_iSubVariable = 0;

    g_iAndVariable = 0xFF;
    g_iOrVariable = 1;
    g_iXorVariable = 3;

    g_iMinVariable = 100;
    g_iMaxVariable = -100;

    g_iSetVariable = 0;

    g_iTestAndSetVariable = 0;
    g_iTestAndSetCounter = 0;

    g_iIncVariable64 = 0;
    g_iDecVariable64 = 0;

    g_iAddVariable64 = 0;
    g_iSubVariable64 = 0;

    g_iAndVariable64 = 0xFF;
    g_iOrVariable64 = 1;
    g_iXorVariable64 = 3;

    g_iMinVariable64 = 100;
    g_iMaxVariable64 = -100;

    g_iSetVariable64 = 0;

    g_iTestAndSetVariable64 = 0;
    g_iTestAndSetCounter64 = 0;

    g_iCompareAndSwapVariable32 = 0;
    g_iCompareAndSwapCounter32 = 0;

    g_iCompareAndSwapVariable64 = 0;
    g_iCompareAndSwapCounter64 = 0;

    g_pTestAndSetPointer = nullptr;
    g_iTestAndSetPointerCounter = 0;

    g_EnumSet = ezVariantType::Bool;
    g_EnumTestAndSetVariable = ezVariantType::Bool;
    g_EnumCompareAndSwap = ezVariantType::Bool;
    g_iTestAndSetCounterEnum = 0;
    g_iCompareAndSwapCounterEnum = 0;
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Thread")
  {
    AtomicsTestThread* pTestThread = nullptr;
    AtomicsTestThread* pTestThread2 = nullptr;

    /// the try-catch is necessary to quiet the static code analysis
    try
    {
      pTestThread = new AtomicsTestThread(1);
      pTestThread2 = new AtomicsTestThread(2);
    }
    catch (...)
    {
    }

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

    EZ_TEST_INT(g_iIncVariable, 2);
    EZ_TEST_INT(g_iDecVariable, -2);

    EZ_TEST_INT(g_iAddVariable, 3);
    EZ_TEST_INT(g_iSubVariable, -3);

    EZ_TEST_INT(g_iAndVariable, 0);
    EZ_TEST_INT(g_iOrVariable, 7);
    EZ_TEST_INT(g_iXorVariable, 5);

    EZ_TEST_INT(g_iMinVariable, 1);
    EZ_TEST_INT(g_iMaxVariable, 2);

    EZ_TEST_BOOL(g_iSetVariable > 0);

    EZ_TEST_BOOL(g_iTestAndSetVariable > 0);
    EZ_TEST_INT(g_iTestAndSetCounter, 1); // only one thread should have set the variable

    EZ_TEST_INT(g_iIncVariable64, 2);
    EZ_TEST_INT(g_iDecVariable64, -2);

    EZ_TEST_INT(g_iAddVariable64, 3);
    EZ_TEST_INT(g_iSubVariable64, -3);

    EZ_TEST_INT(g_iAndVariable64, 0);
    EZ_TEST_INT(g_iOrVariable64, 7);
    EZ_TEST_INT(g_iXorVariable64, 5);

    EZ_TEST_INT(g_iMinVariable64, 1);
    EZ_TEST_INT(g_iMaxVariable64, 2);

    EZ_TEST_BOOL(g_iSetVariable64 > 0);

    EZ_TEST_BOOL(g_iTestAndSetVariable64 > 0);
    EZ_TEST_INT(g_iTestAndSetCounter64, 1);      // only one thread should have set the variable

    EZ_TEST_BOOL(g_pTestAndSetPointer != nullptr);
    EZ_TEST_INT(g_iTestAndSetPointerCounter, 1); // only one thread should have set the variable

    EZ_TEST_BOOL(g_iCompareAndSwapVariable32 > 0);
    EZ_TEST_INT(g_iCompareAndSwapCounter32, 1);  // only one thread should have set the variable

    EZ_TEST_BOOL(g_iCompareAndSwapVariable64 > 0);
    EZ_TEST_INT(g_iCompareAndSwapCounter64, 1);  // only one thread should have set the variable

    g_iDecVariable = 0;
    EZ_TEST_INT(g_iDecVariable.Decrement(), -1);

    g_iDecVariable64 = 0;
    EZ_TEST_INT(g_iDecVariable64.Decrement(), -1);

    EZ_TEST_BOOL(g_EnumSet == ezVariantType::Float || g_EnumSet == ezVariantType::Color);
    EZ_TEST_BOOL(g_EnumTestAndSetVariable == ezVariantType::Float || g_EnumTestAndSetVariable == ezVariantType::Color);
    EZ_TEST_INT(g_iTestAndSetCounterEnum, 1);
    EZ_TEST_BOOL(g_EnumCompareAndSwap == ezVariantType::Float || g_EnumCompareAndSwap == ezVariantType::Color);
    EZ_TEST_INT(g_iCompareAndSwapCounterEnum, 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Post Increment Atomics (basics)")
  {
    g_iPostIncVariable32 = 0;
    EZ_TEST_INT(g_iPostIncVariable32.PostIncrement(), 0);
    EZ_TEST_INT(g_iPostIncVariable32, 1);

    g_iPostIncVariable64 = 0;
    EZ_TEST_INT(g_iPostIncVariable64.PostIncrement(), 0);
    EZ_TEST_INT(g_iPostIncVariable64, 1);

    g_iPostDecVariable32 = 0;
    EZ_TEST_INT(g_iPostDecVariable32.PostDecrement(), 0);
    EZ_TEST_INT(g_iPostDecVariable32, -1);

    g_iPostDecVariable64 = 0;
    EZ_TEST_INT(g_iPostDecVariable64.PostDecrement(), 0);
    EZ_TEST_INT(g_iPostDecVariable64, -1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Post Increment Atomics")
  {

    const ezUInt32 numThreads = 64;

    // used to check that every integer value in range [0; numThreads] is returned exactly once
    g_PostIncValues32.SetCount(numThreads);
    g_PostIncValues64.SetCount(numThreads);
    g_PostDecValues32.SetCount(numThreads);
    g_PostDecValues64.SetCount(numThreads);
    g_PostIncDecThreads.SetCount(numThreads);

    // count up
    g_iPostIncVariable32 = 0;
    g_iPostIncVariable64 = 0;

    // count down
    g_iPostDecVariable32 = numThreads - 1;
    g_iPostDecVariable64 = numThreads - 1;

    for (ezUInt32 t = 0; t < numThreads; ++t)
    {
      g_PostIncDecThreads[t] = EZ_DEFAULT_NEW(PostIncDecThread, t);
    }

    for (ezUInt32 t = 0; t < numThreads; ++t)
    {
      g_PostIncDecThreads[t]->Start();
    }

    for (ezUInt32 t = 0; t < numThreads; ++t)
    {
      g_PostIncDecThreads[t]->Join();
    }

    // check that every value was returned exactly once
    for (ezUInt32 t = 0; t < numThreads; ++t)
    {
      EZ_TEST_INT(g_PostIncValues32[t], 1);
      EZ_TEST_INT(g_PostIncValues64[t], 1);
      EZ_TEST_INT(g_PostDecValues32[t], 1);
      EZ_TEST_INT(g_PostDecValues64[t], 1);
    }

    g_PostIncDecThreads.Clear();
    g_PostIncValues32.Clear();
    g_PostIncValues64.Clear();
    g_PostDecValues32.Clear();
    g_PostDecValues64.Clear();
  }
}
