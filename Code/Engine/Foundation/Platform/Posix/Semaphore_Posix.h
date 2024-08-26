#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Threading/Semaphore.h>

#include <Foundation/Strings/StringBuilder.h>

#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>

EZ_WARNING_PUSH()
// On OSX sem_destroy and sem_init are deprecated
EZ_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")

ezSemaphore::ezSemaphore() = default;

ezSemaphore::~ezSemaphore()
{
  if (m_hSemaphore.m_pNamedOrUnnamed != nullptr)
  {
    if (m_hSemaphore.m_pNamed != nullptr)
    {
      sem_close(m_hSemaphore.m_pNamed);
    }
    else
    {
      sem_destroy(&m_hSemaphore.m_Unnamed);
    }
  }
}

ezResult ezSemaphore::Create(ezUInt32 uiInitialTokenCount, ezStringView sSharedName /*= nullptr*/)
{
  if (sSharedName.IsEmpty())
  {
    // create an unnamed semaphore

    if (sem_init(&m_hSemaphore.m_Unnamed, 0, uiInitialTokenCount) != 0)
    {
      return EZ_FAILURE;
    }

    m_hSemaphore.m_pNamedOrUnnamed = &m_hSemaphore.m_Unnamed;
  }
  else
  {
    // create a named semaphore

    // documentation is unclear about access rights, just throwing everything at it for good measure
    ezStringBuilder tmp;
    m_hSemaphore.m_pNamed = sem_open(sSharedName.GetData(tmp), O_CREAT | O_EXCL, S_IRWXU | S_IRWXO | S_IRWXG, uiInitialTokenCount);

    if (m_hSemaphore.m_pNamed == nullptr)
    {
      return EZ_FAILURE;
    }

    m_hSemaphore.m_pNamedOrUnnamed = m_hSemaphore.m_pNamed;
  }

  return EZ_SUCCESS;
}

ezResult ezSemaphore::Open(ezStringView sSharedName)
{
  EZ_ASSERT_DEV(!sSharedName.IsEmpty(), "Name of semaphore to open mustn't be empty.");

  // open a named semaphore

  ezStringBuilder tmp;
  m_hSemaphore.m_pNamed = sem_open(sSharedName.GetData(tmp), 0);

  if (m_hSemaphore.m_pNamed == nullptr)
  {
    return EZ_FAILURE;
  }

  m_hSemaphore.m_pNamedOrUnnamed = m_hSemaphore.m_pNamed;

  return EZ_SUCCESS;
}

void ezSemaphore::AcquireToken()
{
  EZ_VERIFY(sem_wait(m_hSemaphore.m_pNamedOrUnnamed) == 0, "Semaphore token acquisition failed.");
}

void ezSemaphore::ReturnToken()
{
  EZ_VERIFY(sem_post(m_hSemaphore.m_pNamedOrUnnamed) == 0, "Returning a semaphore token failed, most likely due to a AcquireToken() / ReturnToken() mismatch.");
}

ezResult ezSemaphore::TryAcquireToken()
{
  // documentation is unclear whether one needs to check errno, or not
  // assuming that this will return 0 only when trywait got a token

  if (sem_trywait(m_hSemaphore.m_pNamedOrUnnamed) == 0)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

EZ_WARNING_POP()
