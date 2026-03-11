#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  include <Foundation/Threading/Implementation/OSThread.h>
#  include <Foundation/Threading/Thread.h>

#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <Foundation/Strings/StringConversion.h>

ezAtomicInteger32 ezOSThread::s_iThreadCount;

// Deactivate Doxygen document generation for the following block.
/// \cond

// Exception used to set a thread name
// See: http://blogs.msdn.com/b/stevejs/archive/2005/12/19/505815.aspx for more details
const DWORD MS_VC_EXCEPTION = 0x406D1388;

// This structure describes a thread name set exception
#  pragma pack(push, 8)
using THREADNAME_INFO = struct tagTHREADNAME_INFO
{
  DWORD dwType;     // Must be 0x1000.
  LPCSTR szName;    // Pointer to name (in user addr space).
  DWORD dwThreadID; // Thread ID (-1=caller thread).
  DWORD dwFlags;    // Reserved for future use, must be zero.
};
#  pragma pack(pop)

EZ_WARNING_PUSH()
EZ_WARNING_DISABLE_MSVC(6312)

// See https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreaddescription
// this is the new way to set thread names which are also stored with crash dumps and work in more tools (like Pix etc.)
// however it needs to be loaded dynamically from the kernel DLL.
using pfnSetThreadDescription = HRESULT(WINAPI*)(HANDLE, PCWSTR);


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

void SetThreadNameViaException(HANDLE hThread, LPCSTR pThreadName)
{
  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = pThreadName;
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

void SetThreadName(HANDLE hThread, LPCSTR pThreadName)
{
  static pfnSetThreadDescription s_pSetThreadDescriptionFnPtr = GetSetThreadDescriptionProcAddr();

  if (s_pSetThreadDescriptionFnPtr)
  {
    ezStringWChar threadName(pThreadName);
    s_pSetThreadDescriptionFnPtr(hThread, threadName.GetData());
  }
  else
  {
    SetThreadNameViaException(hThread, pThreadName);
  }
}

EZ_WARNING_POP()

/// \endcond


// Windows specific implementation of the thread class

ezOSThread::ezOSThread(
  ezOSThreadEntryPoint threadEntryPoint, void* pUserData /*= nullptr*/, ezStringView sName /*= "ezThread"*/, ezUInt32 uiStackSize /*= 128 * 1024*/)
{
  s_iThreadCount.Increment();

  EZ_ASSERT_ALWAYS(threadEntryPoint != nullptr, "Thread entry point is invalid.");

  m_hHandle = CreateThread(nullptr, uiStackSize, threadEntryPoint, pUserData, CREATE_SUSPENDED, nullptr);
  EZ_ASSERT_RELEASE(m_hHandle != INVALID_HANDLE_VALUE, "Thread creation failed!");
  EZ_ASSERT_RELEASE(m_hHandle != nullptr, "Thread creation failed!"); // makes the static code analysis happy

  m_ThreadID = GetThreadId(m_hHandle);

  m_EntryPoint = threadEntryPoint;
  m_pUserData = pUserData;
  m_sName = sName;
  m_uiStackSize = uiStackSize;

  // If a name is given, assign it here
  if (!m_sName.IsEmpty())
  {
    SetThreadName(m_hHandle, m_sName.GetData());
  }
}

ezOSThread::~ezOSThread()
{
  CloseHandle(m_hHandle);

  s_iThreadCount.Decrement();
}

/// Attempts to acquire an exclusive lock for this mutex object
void ezOSThread::Start()
{
  ResumeThread(m_hHandle);
}

/// Releases a lock that has been previously acquired
void ezOSThread::Join()
{
  WaitForSingleObject(m_hHandle, INFINITE);
}

#endif
