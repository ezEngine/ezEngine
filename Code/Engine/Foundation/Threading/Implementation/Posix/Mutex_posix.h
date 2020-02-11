
EZ_ALWAYS_INLINE ezMutex::ezMutex()
{
  pthread_mutexattr_t mutexAttributes;
  pthread_mutexattr_init(&mutexAttributes);
  pthread_mutexattr_settype(&mutexAttributes, PTHREAD_MUTEX_RECURSIVE);

  pthread_mutex_init(&m_Handle, &mutexAttributes);

  pthread_mutexattr_destroy(&mutexAttributes);
}

EZ_ALWAYS_INLINE ezMutex::~ezMutex()
{
  pthread_mutex_destroy(&m_Handle);
}

EZ_ALWAYS_INLINE void ezMutex::Acquire()
{
  pthread_mutex_lock(&m_Handle);
  ++m_iLockCount;
}

EZ_ALWAYS_INLINE bool ezMutex::TryAcquire()
{
  if (pthread_mutex_trylock(&m_Handle) == 0)
  {
    ++m_iLockCount;
    return true;
  }

  return false;
}
EZ_ALWAYS_INLINE void ezMutex::Release()
{
  --m_iLockCount;
  pthread_mutex_unlock(&m_Handle);
}
