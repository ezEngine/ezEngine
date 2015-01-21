
#ifdef EZ_THREADLOCALSTORAGE_WIN_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_THREADLOCALSTORAGE_WIN_INL_H_INCLUDED

// Thread local storage implementation for Windows

void ezThreadLocalStorage::Initialize()
{
  if (s_bInitialized)
    return;

  s_bInitialized = true;

  // Get a TLS index from the OS, since we manage a table of our own one TLS index suffices
  g_ThreadLocalManagmentTableKey = TlsAlloc();
  EZ_ASSERT_ALWAYS(g_ThreadLocalManagmentTableKey != TLS_OUT_OF_INDEXES, "Unable to allocate key for thread local management table!");

  // And set the table for the main thread
  SetPerThreadPointerTable(&g_MainThreadLocalPointerTable);
}

void ezThreadLocalStorage::Shutdown()
{
  if (!s_bInitialized)
    return;

  s_bInitialized = false;

  TlsFree(g_ThreadLocalManagmentTableKey);
  g_ThreadLocalManagmentTableKey = TLS_OUT_OF_INDEXES;
}

void ezThreadLocalStorage::SetPerThreadPointerTable(ezThreadLocalPointerTable* pPerThreadPointerTable)
{
  // Initialize the table with NULL pointers
  if (pPerThreadPointerTable != NULL)
  {
    pPerThreadPointerTable->SetCount(EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT);
  }

  TlsSetValue(g_ThreadLocalManagmentTableKey, pPerThreadPointerTable);
}

ezThreadLocalPointerTable* ezThreadLocalStorage::GetPerThreadPointerTable()
{
  EZ_ASSERT_RELEASE(g_ThreadLocalManagmentTableKey != TLS_OUT_OF_INDEXES, "Invalid internal TLS index");

  return static_cast<ezThreadLocalPointerTable*>(TlsGetValue(g_ThreadLocalManagmentTableKey));
}

