#pragma once

// Deactivate Doxygen document generation for the following block.
/// \cond

typedef pthread_t ezThreadHandle;
typedef pthread_mutex_t ezMutexHandle;
typedef void* (*ezOSThreadEntryPoint)(void* pThreadParameter);
typedef pthread_key_t ezThreadLocalStorageKey;

#define EZ_THREAD_CLASS_ENTRY_POINT void* ezThreadClassEntryPoint(void* pThreadParameter);


/// \endcond