
#ifdef EZ_THREADLOCALSTORAGE_POSIX_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_THREADLOCALSTORAGE_POSIX_INL_H_INCLUDED

// Thread local storage implementation for Posix API enabled platforms

void ezThreadLocalStorage::Initialize()
{
  // Get a TLS index from the OS, since we manage a table of our own one TLS index suffices
  int iReturnCode = pthread_key_create(&g_ThreadLocalManagmentTableKey, NULL);
  EZ_ASSERT_ALWAYS(iReturnCode == 0, "Unable to allocate key for thread local management table!");

  // And set the table for the main thread
  SetPerThreadPointerTable(&g_MainThreadLocalPointerTable);
}

void ezThreadLocalStorage::Shutdown()
{
  pthread_key_delete(g_ThreadLocalManagmentTableKey);
  g_ThreadLocalManagmentTableKey = (pthread_key_t)0;
}

void ezThreadLocalStorage::SetPerThreadPointerTable(ezThreadLocalPointerTable* pPerThreadPointerTable)
{
  // Initialize the table with NULL pointers
  if (pPerThreadPointerTable != NULL)
  {
    pPerThreadPointerTable->SetCount(EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT);
  }

  pthread_setspecific(g_ThreadLocalManagmentTableKey, pPerThreadPointerTable);
}

ezThreadLocalPointerTable* ezThreadLocalStorage::GetPerThreadPointerTable()
{
  EZ_ASSERT_RELEASE(g_ThreadLocalManagmentTableKey != (pthread_key_t)0, "Invalid internal TLS index");
  
  return static_cast<ezThreadLocalPointerTable*>(pthread_getspecific(g_ThreadLocalManagmentTableKey));
}

