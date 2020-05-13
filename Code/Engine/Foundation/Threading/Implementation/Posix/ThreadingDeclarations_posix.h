#pragma once

// Deactivate Doxygen document generation for the following block.
/// \cond

#include <pthread.h>
#include <semaphore.h>

typedef pthread_t ezThreadHandle;
typedef pthread_t ezThreadID;
typedef pthread_mutex_t ezMutexHandle;
typedef void* (*ezOSThreadEntryPoint)(void* pThreadParameter);

#define EZ_THREAD_CLASS_ENTRY_POINT void* ezThreadClassEntryPoint(void* pThreadParameter);

struct ezConditionVariableData
{
  pthread_cond_t m_ConditionVariable;
};


/// \endcond
