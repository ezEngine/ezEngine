
#include <Foundation/PCH.h>
#include <Foundation/Threading/ThreadLocalStorage.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/ConditionalLock.h>
#include <Foundation/Configuration/Startup.h>

bool ezThreadLocalStorage::s_bInitialized = false;
static ezThreadLocalPointerTable g_MainThreadLocalPointerTable;


static ezThreadLocalStorageKey g_ThreadLocalManagmentTableKey;

bool g_bThreadLocalStorageAllocationTable[EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT] = {false};

static ezMutex g_TableManagementMutex;

ezUInt32 ezThreadLocalStorage::AllocateSlot()
{
  ezConditionalLock<ezMutex> Lock(g_TableManagementMutex, s_bInitialized);

  Initialize();

  // Find a free slot
  for (ezUInt32 i = 0; i < EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT; ++i)
  {
    if (!g_bThreadLocalStorageAllocationTable[i])
    {
      g_bThreadLocalStorageAllocationTable[i] = true;
      return i;
    }
  }

  // We assert here since failing to allocate a TLS slot is probably very bad for the
  // calling code anyhow
  EZ_REPORT_FAILURE("Couldn't allocate free TLS slot!");


  return EZ_THREAD_LOCAL_STORAGE_INVALID_SLOT;
}

void ezThreadLocalStorage::FreeSlot(ezUInt32 uiSlotIndex)
{
  EZ_ASSERT_DEBUG(uiSlotIndex < EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT, "Invalid slot index passed!");

  ezConditionalLock<ezMutex> Lock(g_TableManagementMutex, s_bInitialized);

  // Special handling for the main thread pointer table since it may live longer
  // than other thread pointer tables - e.g. the foundation tests don't reinitialize the TLS
  // But only when the system is still initialized, otherwise the static destruction order may have cleaned the table
  // already.
  if (s_bInitialized)
    g_MainThreadLocalPointerTable[uiSlotIndex] = nullptr;

  g_bThreadLocalStorageAllocationTable[uiSlotIndex] = false;
}

void ezThreadLocalStorage::SetValueForSlot(ezUInt32 uiSlotIndex, void* pValue)
{
  EZ_ASSERT_DEBUG(uiSlotIndex < EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT, "Invalid slot index passed!");

  ezThreadLocalPointerTable* pThreadLocalPointerTable = GetPerThreadPointerTable();
  EZ_ASSERT_DEBUG(pThreadLocalPointerTable != NULL, "Per thread local storage pointer table hasn't been set. The table for ezThread derived classes is set when calling Start().");

  (*pThreadLocalPointerTable)[uiSlotIndex] = pValue;
}

void* ezThreadLocalStorage::GetValueForSlot(ezUInt32 uiSlotIndex)
{
  EZ_ASSERT_DEBUG(uiSlotIndex < EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT, "Invalid slot index passed!");

  ezThreadLocalPointerTable* pThreadLocalPointerTable = GetPerThreadPointerTable();
  EZ_ASSERT_DEBUG(pThreadLocalPointerTable != NULL, "Per thread local storage pointer table hasn't been set. The table for ezThread derived classes is set when calling Start().");

  return (*pThreadLocalPointerTable)[uiSlotIndex];
}


// Include inline file containing the platform specific functions
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/Threading/Implementation/Win/ThreadLocalStorage_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
  #include <Foundation/Threading/Implementation/Posix/ThreadLocalStorage_posix.h>
#else
  #error "Thread local storage functions are not implemented on current platform"
#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_ThreadLocalStorage);

