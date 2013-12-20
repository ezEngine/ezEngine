
#include <Foundation/PCH.h>
#include <Foundation/Threading/ThreadLocalStorage.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Configuration/Startup.h>

bool ezThreadLocalStorage::s_bInitialized = false;
static ezThreadLocalPointerTable g_MainThreadLocalPointerTable;


static ezThreadLocalStorageKey g_ThreadLocalManagmentTableKey;

bool g_bThreadLocalStorageAllocationTable[EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT] = {false};

static ezMutex g_TableManagementMutex;

ezUInt32 ezThreadLocalStorage::AllocateSlot()
{
  ezLock<ezMutex> Lock(g_TableManagementMutex);

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
  EZ_ASSERT(uiSlotIndex < EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT, "Invalid slot index passed!");

  ezLock<ezMutex> Lock(g_TableManagementMutex);

  g_bThreadLocalStorageAllocationTable[uiSlotIndex] = false;
}

void ezThreadLocalStorage::SetValueForSlot(ezUInt32 uiSlotIndex, void* pValue)
{
  EZ_ASSERT(uiSlotIndex < EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT, "Invalid slot index passed!");

  ezThreadLocalPointerTable* pThreadLocalPointerTable = GetPerThreadPointerTable();
  EZ_ASSERT(pThreadLocalPointerTable != NULL, "Per thread local storage pointer table hasn't been set. The table for ezThread derived classes is set when calling Start().");

  (*pThreadLocalPointerTable)[uiSlotIndex] = pValue;
}

void* ezThreadLocalStorage::GetValueForSlot(ezUInt32 uiSlotIndex)
{
  EZ_ASSERT(uiSlotIndex < EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT, "Invalid slot index passed!");

  ezThreadLocalPointerTable* pThreadLocalPointerTable = GetPerThreadPointerTable();
  EZ_ASSERT(pThreadLocalPointerTable != NULL, "Per thread local storage pointer table hasn't been set. The table for ezThread derived classes is set when calling Start().");

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


EZ_STATICLINK_REFPOINT(Foundation_Threading_Implementation_ThreadLocalStorage);

