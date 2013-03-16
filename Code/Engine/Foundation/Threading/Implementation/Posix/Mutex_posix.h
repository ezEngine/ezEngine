
EZ_FORCE_INLINE ezMutex::ezMutex()
{
  // May need attribute parameter for recursive locking?
  pthread_mutex_init(&m_Handle, NULL);
}

EZ_FORCE_INLINE ezMutex::~ezMutex()
{
  pthread_mutex_destroy(&m_Handle);
}

EZ_FORCE_INLINE void ezMutex::Acquire()
{
  pthread_mutex_lock(&m_Handle);
}

EZ_FORCE_INLINE void ezMutex::Release()
{
  pthread_mutex_unlock(&m_Handle);
}
