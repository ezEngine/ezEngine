
EZ_ALWAYS_INLINE ezMutex::ezMutex()
{
  pthread_mutexattr_t mutexAttributes;
  pthread_mutexattr_init(&mutexAttributes);
  pthread_mutexattr_settype(&mutexAttributes, PTHREAD_MUTEX_RECURSIVE);

  pthread_mutex_init(&m_hHandle, &mutexAttributes);

  pthread_mutexattr_destroy(&mutexAttributes);
}

EZ_ALWAYS_INLINE ezMutex::~ezMutex()
{
  pthread_mutex_destroy(&m_hHandle);
}

EZ_ALWAYS_INLINE void ezMutex::Lock()
{
  pthread_mutex_lock(&m_hHandle);
  ++m_iLockCount;
}

EZ_ALWAYS_INLINE ezResult ezMutex::TryLock()
{
  if (pthread_mutex_trylock(&m_hHandle) == 0)
  {
    ++m_iLockCount;
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}
EZ_ALWAYS_INLINE void ezMutex::Unlock()
{
  --m_iLockCount;
  pthread_mutex_unlock(&m_hHandle);
}
