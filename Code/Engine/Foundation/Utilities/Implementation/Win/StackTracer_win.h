#ifdef EZ_STACKTRACER_WIN_INL_H_INCLUDED
#error "This file must not be included twice."
#endif

#define EZ_STACKTRACER_WIN_INL_H_INCLUDED

#define EZ_MSVC_WARNING_NUMBER 4091
#include <Foundation/Basics/Compiler/DisableWarning.h>

#include <DbgHelp.h>

#include <Foundation/Basics/Compiler/RestoreWarning.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>

// Deactivate Doxygen document generation for the following block.
/// \cond

namespace
{
  typedef WORD(__stdcall* CaptureStackBackTraceFunc)(DWORD FramesToSkip, DWORD FramesToCapture, PVOID* BackTrace, PDWORD BackTraceHash);

  typedef BOOL(__stdcall* SymbolInitializeFunc)(HANDLE hProcess, PCWSTR UserSearchPath, BOOL fInvadeProcess);

  typedef DWORD64(__stdcall* SymbolLoadModuleFunc)(HANDLE hProcess, HANDLE hFile, PCWSTR ImageName, PCWSTR ModuleName, DWORD64 BaseOfDll,
                                                   DWORD DllSize, PMODLOAD_DATA Data, DWORD Flags);

  typedef BOOL(__stdcall* SymbolGetModuleInfoFunc)(HANDLE hProcess, DWORD64 qwAddr, PIMAGEHLP_MODULEW64 ModuleInfo);

  typedef PVOID(__stdcall* SymbolFunctionTableAccess)(HANDLE hProcess, DWORD64 AddrBase);

  typedef DWORD64(__stdcall* SymbolGetModuleBaseFunc)(HANDLE hProcess, DWORD64 qwAddr);

  typedef BOOL(__stdcall* StackWalk)(DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME64 StackFrame, PVOID ContextRecord,
                                     PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
                                     PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
                                     PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);

  typedef BOOL(__stdcall* SymbolFromAddressFunc)(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFOW Symbol);

  typedef BOOL(__stdcall* LineFromAddressFunc)(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PIMAGEHLP_LINEW64 Line);

  struct StackTracerImplementation
  {
    HMODULE kernel32Dll;
    HMODULE dbgHelpDll;

    CaptureStackBackTraceFunc captureStackBackTrace;
    SymbolInitializeFunc symbolInitialize;
    SymbolLoadModuleFunc symbolLoadModule;
    SymbolGetModuleInfoFunc getModuleInfo;
    SymbolFunctionTableAccess getFunctionTableAccess;
    SymbolGetModuleBaseFunc getModuleBase;
    StackWalk stackWalk;
    SymbolFromAddressFunc symbolFromAddress;
    LineFromAddressFunc lineFromAdress;
    bool m_bInitDbgHelp = false;

    StackTracerImplementation()
    {
      ezMemoryUtils::ZeroFill(this, 1);

      kernel32Dll = LoadLibraryW(L"kernel32.dll");
      EZ_ASSERT_DEV(kernel32Dll != nullptr, "StackTracer could not load kernel32.dll");
      if (kernel32Dll != nullptr)
      {
        captureStackBackTrace = (CaptureStackBackTraceFunc)GetProcAddress(kernel32Dll, "RtlCaptureStackBackTrace");
      }

      dbgHelpDll = LoadLibraryW(L"dbghelp.dll");
      EZ_ASSERT_DEV(dbgHelpDll != nullptr, "StackTracer could not load dbghelp.dll");
      if (dbgHelpDll != nullptr)
      {
        symbolInitialize = (SymbolInitializeFunc)GetProcAddress(dbgHelpDll, "SymInitializeW");
        symbolLoadModule = (SymbolLoadModuleFunc)GetProcAddress(dbgHelpDll, "SymLoadModuleExW");
        getModuleInfo = (SymbolGetModuleInfoFunc)GetProcAddress(dbgHelpDll, "SymGetModuleInfoW64");
        getFunctionTableAccess = (SymbolFunctionTableAccess)GetProcAddress(dbgHelpDll, "SymFunctionTableAccess64");
        getModuleBase = (SymbolGetModuleBaseFunc)GetProcAddress(dbgHelpDll, "SymGetModuleBase64");
        stackWalk = (StackWalk)GetProcAddress(dbgHelpDll, "StackWalk64");
        if (symbolInitialize == nullptr || symbolLoadModule == nullptr || getModuleInfo == nullptr || getFunctionTableAccess == nullptr ||
            getModuleBase == nullptr || stackWalk == nullptr)
          return;

        symbolFromAddress = (SymbolFromAddressFunc)GetProcAddress(dbgHelpDll, "SymFromAddrW");
        lineFromAdress = (LineFromAddressFunc)GetProcAddress(dbgHelpDll, "SymGetLineFromAddrW64");
      }
    }
  };

  static StackTracerImplementation* s_pImplementation;

  static void Initialize()
  {
    if (s_pImplementation == nullptr)
    {
      static ezUInt8 ImplementationBuffer[sizeof(StackTracerImplementation)];
      s_pImplementation = new (ImplementationBuffer) StackTracerImplementation();
      EZ_ASSERT_DEV(s_pImplementation != nullptr, "StackTracer initialization failed");
    }
  }

  static void SymbolInitialize()
  {
    if (!s_pImplementation->m_bInitDbgHelp)
    {
      s_pImplementation->m_bInitDbgHelp = true;
      if (!(*s_pImplementation->symbolInitialize)(GetCurrentProcess(), nullptr, TRUE))
      {
        ezLog::Error("StackTracer could not initialize symbols. Error-Code {0}", ezArgErrorCode(::GetLastError()));
      }
    }
  }
}

void ezStackTracer::OnPluginEvent(const ezPlugin::PluginEvent& e)
{
  Initialize();

  if (s_pImplementation->symbolLoadModule == nullptr || s_pImplementation->getModuleInfo == nullptr)
    return;

  // Can't get dbghelp functions to work correctly. SymLoadModuleEx will fail on every dll after the first call.
  // However, SymInitialize works to load dynamic dlls if we postpone it until all dlls are loaded.
  // So we defer init until the first DLL is un-loaded or the first callstack is to be resolved.
  if (e.m_EventType == ezPlugin::PluginEvent::BeforeUnloading)
  {
    SymbolInitialize();
  }

  if (false) // e.m_EventType == ezPlugin::PluginEvent::AfterLoading)
  {
    char buffer[1024];
    strcpy_s(buffer, ezOSFile::GetApplicationDirectory());
    strcat_s(buffer, e.m_szPluginFile);
    strcat_s(buffer, ".dll");

    wchar_t szPluginPath[1024];
    mbstowcs(szPluginPath, buffer, EZ_ARRAY_SIZE(szPluginPath));

    wchar_t szPluginName[256];
    mbstowcs(szPluginName, e.m_szPluginFile, EZ_ARRAY_SIZE(szPluginName));

    HANDLE currentProcess = GetCurrentProcess();

    DWORD64 moduleAddress = (*s_pImplementation->symbolLoadModule)(currentProcess, nullptr, szPluginPath, szPluginName, 0, 0, nullptr, 0);
    if (moduleAddress == 0)
    {
      DWORD err = GetLastError();
      if (err != ERROR_SUCCESS)
      {
        ezLog::Error("StackTracer could not load symbols for '{0}'. Error-Code {1}", e.m_szPluginFile, ezArgErrorCode(err));
      }

      return;
    }

    IMAGEHLP_MODULEW64 moduleInfo;
    ezMemoryUtils::ZeroFill(&moduleInfo, 1);
    moduleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULEW64);

    if (!(*s_pImplementation->getModuleInfo)(currentProcess, moduleAddress, &moduleInfo))
    {
      DWORD err = GetLastError();
      LPVOID lpMsgBuf = nullptr;

      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, err,
                    MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPTSTR)&lpMsgBuf, 0, nullptr);

      char errStr[1024];
      sprintf_s(errStr, "StackTracer could not get module info for '%s'. Error-Code %u (\"%s\")\n", e.m_szPluginFile, err,
                static_cast<char*>(lpMsgBuf));
      OutputDebugStringA(errStr);

      LocalFree(lpMsgBuf);
    }
  }
}

// static
ezUInt32 ezStackTracer::GetStackTrace(ezArrayPtr<void*>& trace, void* pContext)
{
  Initialize();

  if (pContext && s_pImplementation->stackWalk)
  {
    CONTEXT& context = *static_cast<PCONTEXT>(pContext);

    DWORD machine_type;
    STACKFRAME64 frame;
    ZeroMemory(&frame, sizeof(frame));
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;
#ifdef _M_X64
    frame.AddrPC.Offset = context.Rip;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrStack.Offset = context.Rsp;
    machine_type = IMAGE_FILE_MACHINE_AMD64;
#else
    frame.AddrPC.Offset = context.Eip;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrStack.Offset = context.Esp;
    machine_type = IMAGE_FILE_MACHINE_I386;
#endif
    for (ezInt32 i = 0; i < (ezInt32)trace.GetCount(); i++)
    {
      if (s_pImplementation->stackWalk(machine_type, GetCurrentProcess(), GetCurrentThread(), &frame, &context, NULL,
                                       s_pImplementation->getFunctionTableAccess, s_pImplementation->getModuleBase, NULL))
      {
        trace[i] = reinterpret_cast<void*>(frame.AddrPC.Offset);
      }
      else
      {
        // skip the last three stack-frames since they are useless
        return ezMath::Max(i - 4, 0);
      }
    }
  }
  else if (s_pImplementation->captureStackBackTrace != nullptr)
  {
    const ezUInt32 uiSkip = 1;
    const ezUInt32 uiMaxNumTrace = ezMath::Min(62U, trace.GetCount());
    ezInt32 iNumTraces = (*s_pImplementation->captureStackBackTrace)(uiSkip, uiMaxNumTrace, trace.GetPtr(), nullptr);

    // skip the last three stack-frames since they are useless
    return ezMath::Max(iNumTraces - 3, 0);
  }

  return 0;
}

// static
void ezStackTracer::ResolveStackTrace(const ezArrayPtr<void*>& trace, PrintFunc printFunc)
{
  Initialize();
  SymbolInitialize();

  if (s_pImplementation->symbolFromAddress != nullptr && s_pImplementation->lineFromAdress != nullptr)
  {
    char buffer[1024];
    HANDLE currentProcess = GetCurrentProcess();

    const ezUInt32 uiNumTraceEntries = trace.GetCount();
    for (ezUInt32 i = 0; i < uiNumTraceEntries; i++)
    {
      DWORD64 pSymbolAddress = reinterpret_cast<UINT_PTR>(trace[i]);

      _SYMBOL_INFOW& symbolInfo = *(_SYMBOL_INFOW*)buffer;
      ezMemoryUtils::ZeroFill(&symbolInfo, 1);
      symbolInfo.SizeOfStruct = sizeof(_SYMBOL_INFOW);
      symbolInfo.MaxNameLen = (EZ_ARRAY_SIZE(buffer) - symbolInfo.SizeOfStruct) / sizeof(WCHAR);

      DWORD64 displacement = 0;
      BOOL result = (*s_pImplementation->symbolFromAddress)(currentProcess, pSymbolAddress, &displacement, &symbolInfo);
      if (!result)
      {
        wcscpy_s(symbolInfo.Name, symbolInfo.MaxNameLen, L"<Unknown>");
      }

      IMAGEHLP_LINEW64 lineInfo;
      memset(&lineInfo, 0, sizeof(lineInfo));
      lineInfo.SizeOfStruct = sizeof(lineInfo);
      (*s_pImplementation->lineFromAdress)(currentProcess, pSymbolAddress, &displacement, &lineInfo);

      wchar_t str[1024];
      swprintf_s(str, L"%s(%u):'%s'\n", lineInfo.FileName, lineInfo.LineNumber, symbolInfo.Name);

      char finalStr[1024];
      wcstombs(finalStr, str, EZ_ARRAY_SIZE(finalStr));

      (*printFunc)(finalStr);
    }
  }
}


/// \endcond
