#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Strings/StringConversion.h>

ezAtomicInteger32 ezOSThread::s_iThreadCount;

// Deactivate Doxygen document generation for the following block.
/// \cond

// Exception used to set a thread name
// See: http://blogs.msdn.com/b/stevejs/archive/2005/12/19/505815.aspx for more details
const DWORD MS_VC_EXCEPTION = 0x406D1388;

// This structure describes a thread name set exception
#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO
{
  DWORD dwType;     // Must be 0x1000.
  LPCSTR szName;    // Pointer to name (in user addr space).
  DWORD dwThreadID; // Thread ID (-1=caller thread).
  DWORD dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

#define EZ_MSVC_WARNING_NUMBER 6312
#include <Foundation/Basics/Compiler/MSVC/DisableWarning_MSVC.h>

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)

// See https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreaddescription
// this is the new way to set thread names which are also stored with crash dumps and work in more tools (like Pix etc.)
// however it needs to be loaded dynamically from the kernel DLL.
typedef HRESULT(WINAPI* pfnSetThreadDescription)(HANDLE, PCWSTR);


// According to the docs the thread description function lives in kernel32.dll,
// however on StackOverflow you can find that it seems to be in KernelBase.dll
// (https://stackoverflow.com/questions/62243162/how-to-access-setthreaddescription-in-windows-2016-server-version-1607)
// Thus we try to load it from both places just in case things change
pfnSetThreadDescription GetSetThreadDescriptionProcAddr()
{
  pfnSetThreadDescription retVal = nullptr;

  {
    HMODULE kernel32 = GetModuleHandleA("Kernel32.dll");
    if (kernel32 != nullptr)
    {
      retVal = reinterpret_cast<pfnSetThreadDescription>(GetProcAddress(kernel32, "SetThreadDescription"));
    }

    if (retVal != nullptr)
    {
      return retVal;
    }
  }

  {
    HMODULE kernelBase = GetModuleHandleA("KernelBase.dll");
    if (kernelBase != nullptr)
    {
      retVal = reinterpret_cast<pfnSetThreadDescription>(GetProcAddress(kernelBase, "SetThreadDescription"));
    }
  }

  return retVal;
}

#endif

void SetThreadNameViaException(HANDLE hThread, LPCSTR szThreadName)
{
  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = szThreadName;
  info.dwThreadID = GetThreadId(hThread);
  info.dwFlags = 0;

  __try
  {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), (ULONG_PTR*)&info);
  }
  __except (EXCEPTION_CONTINUE_EXECUTION)
  {
    return; // makes the static code analysis happy
  }
}

void SetThreadName(HANDLE hThread, LPCSTR szThreadName)
{
#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)
  static pfnSetThreadDescription s_pSetThreadDescriptionFnPtr = GetSetThreadDescriptionProcAddr();

  if (s_pSetThreadDescriptionFnPtr)
  {
    ezStringWChar threadName(szThreadName);
    s_pSetThreadDescriptionFnPtr(hThread, threadName.GetData());
  }
  else
  {
    SetThreadNameViaException(hThread, szThreadName);
  }
#else
  SetThreadNameViaException(hThread, szThreadName);
#endif
}

#include <Foundation/Basics/Compiler/MSVC/RestoreWarning_MSVC.h>

/// \endcond


// Windows specific implementation of the thread class

ezOSThread::ezOSThread(
  ezOSThreadEntryPoint pThreadEntryPoint, void* pUserData /*= nullptr*/, const char* szName /*= "ezThread"*/, ezUInt32 uiStackSize /*= 128 * 1024*/)
{
  s_iThreadCount.Increment();

  EZ_ASSERT_ALWAYS(pThreadEntryPoint != nullptr, "Thread entry point is invalid.");

  m_Handle = CreateThread(nullptr, uiStackSize, pThreadEntryPoint, pUserData, CREATE_SUSPENDED, nullptr);
  EZ_ASSERT_RELEASE(m_Handle != INVALID_HANDLE_VALUE, "Thread creation failed!");
  EZ_ASSERT_RELEASE(m_Handle != nullptr, "Thread creation failed!"); // makes the static code analysis happy

  m_ThreadID = GetThreadId(m_Handle);

  m_EntryPoint = pThreadEntryPoint;
  m_pUserData = pUserData;
  m_szName = szName;
  m_uiStackSize = uiStackSize;

  // If a name is given, assign it here
  if (szName != nullptr)
  {
    SetThreadName(m_Handle, szName);
  }
}

ezOSThread::~ezOSThread()
{
  CloseHandle(m_Handle);

  s_iThreadCount.Decrement();
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
