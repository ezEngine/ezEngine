#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <Foundation/Platform/Win/Utils/MinWindows.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/Threading/Semaphore.h>

ezSemaphore::ezSemaphore()
{
  m_hSemaphore = nullptr;
}

ezSemaphore::~ezSemaphore()
{
  if (m_hSemaphore != nullptr)
  {
    CloseHandle(m_hSemaphore);
    m_hSemaphore = nullptr;
  }
}

ezResult ezSemaphore::Create(ezUInt32 uiInitialTokenCount, ezStringView sSharedName /*= nullptr*/)
{
  EZ_ASSERT_DEV(m_hSemaphore == nullptr, "Semaphore can't be recreated.");

  LPSECURITY_ATTRIBUTES secAttr = nullptr; // default
  const DWORD flags = 0;                   // reserved but unused
  const DWORD access = STANDARD_RIGHTS_ALL | SEMAPHORE_MODIFY_STATE /* needed for ReleaseSemaphore */;

  if (sSharedName.IsEmpty())
  {
    // create an unnamed semaphore

    m_hSemaphore = CreateSemaphoreExW(secAttr, uiInitialTokenCount, ezMath::MaxValue<ezInt32>(), nullptr, flags, access);
  }
  else
  {
    // create a named semaphore in the 'Local' namespace
    // these are visible session wide, ie. all processes by the same user account can see these, but not across users

    const ezStringBuilder semaphoreName("Local\\", sSharedName);

    m_hSemaphore = CreateSemaphoreExW(secAttr, uiInitialTokenCount, ezMath::MaxValue<ezInt32>(), ezStringWChar(semaphoreName).GetData(), flags, access);
  }

  if (m_hSemaphore == nullptr)
  {
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezSemaphore::Open(ezStringView sSharedName)
{
  EZ_ASSERT_DEV(m_hSemaphore == nullptr, "Semaphore can't be recreated.");

  const DWORD access = SYNCHRONIZE /* needed for WaitForSingleObject */ | SEMAPHORE_MODIFY_STATE /* needed for ReleaseSemaphore */;
  const BOOL inheriteHandle = FALSE;

  EZ_ASSERT_DEV(!sSharedName.IsEmpty(), "Name of semaphore to open mustn't be empty.");

  const ezStringBuilder semaphoreName("Local\\", sSharedName);

  m_hSemaphore = OpenSemaphoreW(access, inheriteHandle, ezStringWChar(semaphoreName).GetData());

  if (m_hSemaphore == nullptr)
  {
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezSemaphore::AcquireToken()
{
  EZ_ASSERT_DEV(m_hSemaphore != nullptr, "Invalid semaphore.");
  EZ_VERIFY(WaitForSingleObject(m_hSemaphore, INFINITE) == WAIT_OBJECT_0, "Semaphore token acquisition failed.");
}

void ezSemaphore::ReturnToken()
{
  EZ_ASSERT_DEV(m_hSemaphore != nullptr, "Invalid semaphore.");
  EZ_VERIFY(ReleaseSemaphore(m_hSemaphore, 1, nullptr) != 0, "Returning a semaphore token failed, most likely due to a AcquireToken() / ReturnToken() mismatch.");
}

ezResult ezSemaphore::TryAcquireToken()
{
  EZ_ASSERT_DEV(m_hSemaphore != nullptr, "Invalid semaphore.");

  const ezUInt32 res = WaitForSingleObject(m_hSemaphore, 0 /* timeout of zero milliseconds */);

  if (res == WAIT_OBJECT_0)
  {
    return EZ_SUCCESS;
  }

  EZ_ASSERT_DEV(res == WAIT_OBJECT_0 || res == WAIT_TIMEOUT, "Semaphore TryAcquireToken (WaitForSingleObject) failed with error code {}.", res);

  return EZ_FAILURE;
}

#endif
