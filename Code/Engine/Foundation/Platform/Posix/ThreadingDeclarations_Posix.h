#pragma once

// Deactivate Doxygen document generation for the following block.
/// \cond

#include <pthread.h>
#include <semaphore.h>

using ezThreadHandle = pthread_t;
using ezThreadID = pthread_t;
using ezMutexHandle = pthread_mutex_t;
using ezOSThreadEntryPoint = void* (*)(void* pThreadParameter);

struct ezSemaphoreHandle
{
  sem_t* m_pNamedOrUnnamed = nullptr;
  sem_t* m_pNamed = nullptr;
  sem_t m_Unnamed;
};

#define EZ_THREAD_CLASS_ENTRY_POINT void* ezThreadClassEntryPoint(void* pThreadParameter);

struct ezConditionVariableData
{
  pthread_cond_t m_ConditionVariable;
};


/// \endcond
