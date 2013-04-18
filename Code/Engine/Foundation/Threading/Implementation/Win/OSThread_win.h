
#ifdef EZ_OSTHREAD_WIN_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_OSTHREAD_WIN_INL_H_INCLUDED

// Exception used to set a thread name
// See: http://blogs.msdn.com/b/stevejs/archive/2005/12/19/505815.aspx for more details
const DWORD MS_VC_EXCEPTION = 0x406D1388;

// This structure describes a thread name set exception
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
  DWORD dwType; // Must be 0x1000.
  LPCSTR szName; // Pointer to name (in user addr space).
  DWORD dwThreadID; // Thread ID (-1=caller thread).
  DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)


#define EZ_MSVC_WARNING_NUMBER 6312
#include <Foundation/Basics/Compiler/DisableWarning.h>

void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName)
{
  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = szThreadName;
  info.dwThreadID = dwThreadID;
  info.dwFlags = 0;

  __try
  {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(DWORD), (ULONG_PTR*)&info);
  }
  __except(EXCEPTION_CONTINUE_EXECUTION)
  {
    return; // makes the static code analysis happy
  }
}

#include <Foundation/Basics/Compiler/RestoreWarning.h>


// Windows specific implementation of the thread class

ezOSThread::ezOSThread(ezOSThreadEntryPoint pThreadEntryPoint, void* pUserData /*= NULL*/, const char* pName /*= "ezThread"*/, ezUInt32 uiStackSize /*= 128 * 1024*/)
{
  m_Handle = CreateThread(NULL, uiStackSize, pThreadEntryPoint, pUserData, CREATE_SUSPENDED, NULL);
  EZ_ASSERT(m_Handle != INVALID_HANDLE_VALUE, "Thread creation failed!");
  EZ_ASSERT(m_Handle != NULL, "Thread creation failed!"); // makes the static code analysis happy
  m_EntryPoint = pThreadEntryPoint;

  // If a name is given, assign it here
  if(pName != NULL)
  {
    SetThreadName(GetThreadId(m_Handle), pName);
  }

}

ezOSThread::~ezOSThread()
{
  CloseHandle(m_Handle);
}

/// Attempts to acquire an exclusive lock for this mutex object
void ezOSThread::Start()
{
  ResumeThread(m_Handle);
}

/// Releases a lock that has been previously acquired
void ezOSThread::Join()
{
  WaitForSingleObject(m_Handle, INFINITE);
}

