
EZ_FORCE_INLINE ezMutex::ezMutex()
{
  pthread_mutexattr_t mutexAttributes;
  pthread_mutexattr_init(&mutexAttributes);
  pthread_mutexattr_settype(&mutexAttributes, PTHREAD_MUTEX_RECURSIVE);
  
  pthread_mutex_init(&m_Handle, &mutexAttributes);
  
  pthread_mutexattr_destroy(&mutexAttributes);
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

