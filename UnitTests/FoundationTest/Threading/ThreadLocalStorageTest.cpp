#include <PCH.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadLocalStorage.h>
#include <Foundation/Threading/ThreadLocalPointer.h>

namespace 
{
  ezThreadLocalPointer<ezUInt32> g_TlsValue;

  class TestThread : public ezThread
  {
  public:


    TestThread()
      : ezThread("Test Thread"), m_bTestsCheckedOut(false)
    {
    }

    void SetThreadLocalVariable(ezUInt32 uiThreadLocalVariable)
    {
      m_uiThreadLocalVariable = uiThreadLocalVariable;
    }

    virtual ezUInt32 Run()
    {
      ezUInt32 uiCopyOfValue = m_uiThreadLocalVariable;

      // Assign the pointer of this threads uint32 to the shared TLS variable
      g_TlsValue = &m_uiThreadLocalVariable;
      
      ezUInt32* pValueStoredInTLSVar = g_TlsValue;

      if (pValueStoredInTLSVar == nullptr)
        return 0;

      if (pValueStoredInTLSVar != &m_uiThreadLocalVariable)
        return 0;

      if (*pValueStoredInTLSVar != uiCopyOfValue)
        return 0;

      m_bTestsCheckedOut = true;
      return 0;
    }

    inline bool DidTestsCheckOut() const
    {
      return m_bTestsCheckedOut;
    }

  private:

    ezUInt32 m_uiThreadLocalVariable;

    bool m_bTestsCheckedOut;
  };

}


EZ_CREATE_SIMPLE_TEST(Threading, ThreadLocalStorage)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Thread Local Storage")
  {
    // Test simply accessing the main thread values first

    ezUInt32 uiVarIndex = ezThreadLocalStorage::AllocateSlot();
    EZ_TEST_BOOL(ezThreadLocalStorage::IsValidSlot(uiVarIndex));

    // The initial value should be nullptr
    EZ_TEST_BOOL(ezThreadLocalStorage::GetValueForSlot(uiVarIndex) == nullptr);

    // Place a pointer there
    void* pBogusPointer = reinterpret_cast<void*>(0xBAADF00D);
    ezThreadLocalStorage::SetValueForSlot(uiVarIndex, pBogusPointer);

    EZ_TEST_BOOL(ezThreadLocalStorage::GetValueForSlot(uiVarIndex) == pBogusPointer);

    // Now allocate another variable (should have another index)
    ezUInt32 uiOtherVarIndex = ezThreadLocalStorage::AllocateSlot();
    EZ_TEST_BOOL(ezThreadLocalStorage::IsValidSlot(uiVarIndex));

    EZ_TEST_BOOL(uiOtherVarIndex != uiVarIndex);
    
    // And free the slots
    ezThreadLocalStorage::FreeSlot(uiVarIndex);
    ezThreadLocalStorage::FreeSlot(uiOtherVarIndex);
  }

  

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Thread Local Variable Class")
  {
    ezUInt32 uiMainThreadValue = 42;
    ezUInt32 uiCopyValue = uiMainThreadValue;

    // First assign the TLS variable the address of the main thread
    g_TlsValue = &uiMainThreadValue;

    ezUInt32* pValueStoredInTLSVar = g_TlsValue;
    EZ_TEST_BOOL(pValueStoredInTLSVar != nullptr);
    EZ_TEST_BOOL(pValueStoredInTLSVar == &uiMainThreadValue);

    EZ_TEST_BOOL(*pValueStoredInTLSVar == uiCopyValue);

    ezStaticArray<TestThread*, EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT * 2> TestThreads;

    // Now spawn additional threads which also assign values to the TLS variable
    // We spawn more than available TLS slots to make sure nothing is allocated per thread
    // and not shared like intended
    for (ezUInt32 i = 0; i < EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT * 2; ++i)
    {
      TestThreads.PushBack(EZ_DEFAULT_NEW(TestThread));
      TestThreads[i]->SetThreadLocalVariable(i);
      TestThreads[i]->Start();
    }

    for (ezUInt32 i = 0; i < EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT * 2; ++i)
    {
      TestThreads[i]->Join();
    }

    for (ezUInt32 i = 0; i < EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT * 2; ++i)
    {
      EZ_TEST_BOOL(TestThreads[i]->DidTestsCheckOut());
      EZ_DEFAULT_DELETE(TestThreads[i]);
    }
  }
}