#pragma once

#ifdef EZ_THREADSIGNAL_POSIX_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_THREADSIGNAL_POSIX_INL_H_INCLUDED

ezThreadSignal::ezThreadSignal()
{
  m_Data.m_bSignalIsRaised = false;

  pthread_mutexattr_t MutexAttributes;
  pthread_mutexattr_init(&MutexAttributes);
  pthread_mutexattr_settype(&MutexAttributes, PTHREAD_MUTEX_RECURSIVE);

  EZ_VERIFY(pthread_mutex_init(&m_Data.m_Mutex, &MutexAttributes) == 0, "Failed to initialize mutex for thread signal.");

  EZ_VERIFY(pthread_cond_init(&m_Data.m_ConditionVariable, nullptr) == 0, "Failed to initialize the condition variable for thread signal.");
}

ezThreadSignal::~ezThreadSignal()
{
  pthread_cond_destroy(&m_Data.m_ConditionVariable);
  pthread_mutex_destroy(&m_Data.m_Mutex);
}

void ezThreadSignal::WaitForSignal()
{
  pthread_mutex_lock(&m_Data.m_Mutex);

  // if no one was waiting for the signal, so far, this thread will get the signal immediately
  while (!m_Data.m_bSignalIsRaised)
    pthread_cond_wait(&m_Data.m_ConditionVariable, &m_Data.m_Mutex);

  // this thread got the signal, so reset the state of the signal, to prevent other threads from running as well
  m_Data.m_bSignalIsRaised = false;
  pthread_mutex_unlock(&m_Data.m_Mutex);
}

void ezThreadSignal::RaiseSignal()
{
  pthread_mutex_lock(&m_Data.m_Mutex);

  // set the signal to be raised and wake up the waiting threads
  m_Data.m_bSignalIsRaised = true;
  pthread_cond_signal(&m_Data.m_ConditionVariable);

  pthread_mutex_unlock(&m_Data.m_Mutex);
}


