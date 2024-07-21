#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/System/StackTracer.h>

EZ_WARNING_PUSH()
EZ_WARNING_DISABLE_MSVC(4091)

#  include <DbgHelp.h>

EZ_WARNING_POP()

#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Math/Math.h>

#  include <memory>

// Deactivate Doxygen document generation for the following block.
/// \cond

namespace
{
  using CaptureStackBackTraceFunc = WORD(__stdcall*)(DWORD FramesToSkip, DWORD FramesToCapture, PVOID* BackTrace, PDWORD BackTraceHash);
  using SymbolInitializeFunc = BOOL(__stdcall*)(HANDLE hProcess, PCWSTR UserSearchPath, BOOL fInvadeProcess);
  using SymCleanupFunc = BOOL(__stdcall*)(HANDLE hProcess);
  using SymbolLoadModuleFunc = DWORD64(__stdcall*)(HANDLE hProcess, HANDLE hFile, PCWSTR ImageName, PCWSTR ModuleName, DWORD64 BaseOfDll, DWORD DllSize, PMODLOAD_DATA Data, DWORD Flags);
  using SymRefreshModuleListFunc = BOOL(__stdcall*)(HANDLE hProcess);
  using SymbolGetModuleInfoFunc = BOOL(__stdcall*)(HANDLE hProcess, DWORD64 qwAddr, PIMAGEHLP_MODULEW64 ModuleInfo);
  using SymbolFunctionTableAccess = PVOID(__stdcall*)(HANDLE hProcess, DWORD64 AddrBase);
  using SymbolGetModuleBaseFunc = DWORD64(__stdcall*)(HANDLE hProcess, DWORD64 qwAddr);
  using StackWalk = BOOL(__stdcall*)(DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME64 StackFrame, PVOID ContextRecord, PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine, PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine, PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);
  using SymbolFromAddressFunc = BOOL(__stdcall*)(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFOW Symbol);
  using LineFromAddressFunc = BOOL(__stdcall*)(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PIMAGEHLP_LINEW64 Line);
  using SymSetSearchPathFunc = BOOL(__stdcall*)(HANDLE hProcess, PCWSTR SearchPath);

  struct StackTracerImplementation
  {
    HMODULE kernel32Dll;
    HMODULE dbgHelpDll;

    CaptureStackBackTraceFunc captureStackBackTrace;
    SymbolInitializeFunc symbolInitialize;
    SymCleanupFunc symCleanup;
    SymRefreshModuleListFunc symRefreshModuleList;
    SymbolLoadModuleFunc symbolLoadModule;
    SymbolGetModuleInfoFunc getModuleInfo;
    SymbolFunctionTableAccess getFunctionTableAccess;
    SymbolGetModuleBaseFunc getModuleBase;
    StackWalk stackWalk;
    SymbolFromAddressFunc symbolFromAddress;
    LineFromAddressFunc lineFromAdress;
    SymSetSearchPathFunc symSetSearchPath;
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
        symSetSearchPath = (SymSetSearchPathFunc)GetProcAddress(dbgHelpDll, "SymSetSearchPathW");
        symbolInitialize = (SymbolInitializeFunc)GetProcAddress(dbgHelpDll, "SymInitializeW");
        symCleanup = (SymCleanupFunc)GetProcAddress(dbgHelpDll, "SymCleanup");
        symRefreshModuleList = (SymRefreshModuleListFunc)GetProcAddress(dbgHelpDll, "SymRefreshModuleList");
        symbolLoadModule = (SymbolLoadModuleFunc)GetProcAddress(dbgHelpDll, "SymLoadModuleExW");
        getModuleInfo = (SymbolGetModuleInfoFunc)GetProcAddress(dbgHelpDll, "SymGetModuleInfoW64");
        getFunctionTableAccess = (SymbolFunctionTableAccess)GetProcAddress(dbgHelpDll, "SymFunctionTableAccess64");
        getModuleBase = (SymbolGetModuleBaseFunc)GetProcAddress(dbgHelpDll, "SymGetModuleBase64");
        stackWalk = (StackWalk)GetProcAddress(dbgHelpDll, "StackWalk64");

        if (symbolInitialize == nullptr ||
            symCleanup == nullptr ||
            symbolLoadModule == nullptr ||
            symRefreshModuleList == nullptr ||
            getModuleInfo == nullptr ||
            getFunctionTableAccess == nullptr ||
            getModuleBase == nullptr ||
            stackWalk == nullptr ||
            symSetSearchPath == nullptr)
        {
          return;
        }

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
      alignas(EZ_ALIGNMENT_OF(StackTracerImplementation)) static ezUInt8 ImplementationBuffer[sizeof(StackTracerImplementation)];
      s_pImplementation = new (ImplementationBuffer) StackTracerImplementation();
      EZ_ASSERT_DEV(s_pImplementation != nullptr, "StackTracer initialization failed");
    }
  }

  static bool SymbolInitializeImpl()
  {
    // initializing symbol loading easily fails if any other code already did this before
    // so when using EZ together with for example a third party library that also records stack traces, just calling SymInitialize will fail
    // and we don't get any callstacks
    // the multi-step approach below has worked in known problematic scenarios, but no guarantee that there isn't a better "right way" to do it

    if(s_pImplementation->symbolInitialize == nullptr || s_pImplementation->symRefreshModuleList == nullptr || s_pImplementation->symCleanup == nullptr)
    {
      return false;
    }

    // try SymInitialize first
    if ((*s_pImplementation->symbolInitialize)(GetCurrentProcess(), nullptr, TRUE))
      return true;

    // try SymRefreshModuleList next
    if ((*s_pImplementation->symRefreshModuleList)(GetCurrentProcess()))
      return true;

    // otherwise cleanup and try again
    if (!(*s_pImplementation->symCleanup)(GetCurrentProcess()))
      return false;

    if ((*s_pImplementation->symbolInitialize)(GetCurrentProcess(), nullptr, TRUE))
      return true;

    return false;
  }

  static void SymbolInitialize()
  {
    if (s_pImplementation->m_bInitDbgHelp)
      return;

    s_pImplementation->m_bInitDbgHelp = true;

    if (!SymbolInitializeImpl())
    {
      ezLog::Error("StackTracer could not initialize symbols. Error-Code {0}", ezArgErrorCode(::GetLastError()));
      return;
    }

    // we want to search for the PDBs in the same directory where the EXE is located, no matter what the current working directory is
    if (!(*s_pImplementation->symSetSearchPath)(GetCurrentProcess(), ezStringWChar(ezOSFile::GetApplicationDirectory())))
    {
      ezLog::Error("StackTracer could not set symbol search path. Error-Code {0}", ezArgErrorCode(::GetLastError()));
      return;
    }
  }
} // namespace

void ezStackTracer::OnPluginEvent(const ezPluginEvent& e)
{
  Initialize();

  if (s_pImplementation->symbolLoadModule == nullptr || s_pImplementation->getModuleInfo == nullptr)
    return;

  // Can't get dbghelp functions to work correctly. SymLoadModuleEx will fail on every dll after the first call.
  // However, SymInitialize works to load dynamic dlls if we postpone it until all dlls are loaded.
  // So we defer init until the first DLL is un-loaded or the first callstack is to be resolved.
  if (e.m_EventType == ezPluginEvent::BeforeUnloading)
  {
    SymbolInitialize();
  }

  if (false) // e.m_EventType == ezPluginEvent::AfterLoading)
  {
    ezStringBuilder tmp;

    char buffer[1024];
    strcpy_s(buffer, ezOSFile::GetApplicationDirectory().GetStartPointer());
    strcat_s(buffer, e.m_sPluginBinary.GetData(tmp));
    strcat_s(buffer, ".dll");

    wchar_t szPluginPath[1024];
    mbstowcs(szPluginPath, buffer, EZ_ARRAY_SIZE(szPluginPath));

    wchar_t szPluginName[256];
    mbstowcs(szPluginName, e.m_sPluginBinary.GetData(tmp), EZ_ARRAY_SIZE(szPluginName));

    HANDLE currentProcess = GetCurrentProcess();

    DWORD64 moduleAddress = (*s_pImplementation->symbolLoadModule)(currentProcess, nullptr, szPluginPath, szPluginName, 0, 0, nullptr, 0);
    if (moduleAddress == 0)
    {
      DWORD err = GetLastError();
      if (err != ERROR_SUCCESS)
      {
        ezLog::Error("StackTracer could not load symbols for '{0}'. Error-Code {1}", e.m_sPluginBinary, ezArgErrorCode(err));
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
      ezStringUtils::snprintf(errStr, 1024, "StackTracer could not get module info for '%s'. Error-Code %u (\"%s\")\n", e.m_sPluginBinary.GetData(tmp), err, static_cast<char*>(lpMsgBuf));
      ezLog::Print(errStr);

      LocalFree(lpMsgBuf);
    }
  }
}

// static
ezUInt32 ezStackTracer::GetStackTrace(ezArrayPtr<void*>& ref_trace, void* pContext)
{
  Initialize();

  if (pContext && s_pImplementation->stackWalk)
  {
    // We need dbghelp init for stackWalk call.
    SymbolInitialize();
    // in order not to destroy the pContext handed in we need to make a copy of it
    // see StackWalk/StackWalk64 docs https://docs.microsoft.com/windows/win32/api/dbghelp/nf-dbghelp-stackwalk
    PCONTEXT originalContext = static_cast<PCONTEXT>(pContext);
    PCONTEXT copiedContext = nullptr;

    DWORD contextSize = 0;
    // get size needed for buffer and allocate buffer of that size
    InitializeContext(nullptr, originalContext->ContextFlags, &copiedContext, &contextSize);
    unsigned char* rawBuffer = new (std::nothrow) unsigned char[contextSize];
    if (rawBuffer == nullptr)
    {
      return 0;
    }
    auto pBuffer = std::unique_ptr<unsigned char[]>(rawBuffer);

    BOOL contextInitalized = InitializeContext(static_cast<void*>(pBuffer.get()), originalContext->ContextFlags, &copiedContext, &contextSize);
    if (!contextInitalized)
    {
      return 0;
    }

    BOOL contextCopied = CopyContext(copiedContext, originalContext->ContextFlags, originalContext);
    if (!contextCopied)
    {
      return 0;
    }

    CONTEXT& context = *static_cast<PCONTEXT>(copiedContext);
    DWORD machine_type;
    STACKFRAME64 frame;
    ZeroMemory(&frame, sizeof(frame));
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;
#  if defined(_M_X64)
    frame.AddrPC.Offset = context.Rip;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrStack.Offset = context.Rsp;
    machine_type = IMAGE_FILE_MACHINE_AMD64;
#  elif defined(_ARM64_)
    frame.AddrPC.Offset = context.Pc;
    frame.AddrFrame.Offset = context.Fp;
    frame.AddrStack.Offset = context.Sp;
    machine_type = IMAGE_FILE_MACHINE_ARM64;
#  elif defined(_X86_)
    frame.AddrPC.Offset = context.Eip;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrStack.Offset = context.Esp;
    machine_type = IMAGE_FILE_MACHINE_I386;
#  else
#    error Unsupported platform
#  endif

    for (ezInt32 i = 0; i < (ezInt32)ref_trace.GetCount(); i++)
    {
      if (s_pImplementation->stackWalk(machine_type, GetCurrentProcess(), GetCurrentThread(), &frame, &context, NULL,
            s_pImplementation->getFunctionTableAccess, s_pImplementation->getModuleBase, NULL))
      {
        ref_trace[i] = reinterpret_cast<void*>(frame.AddrPC.Offset);
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
    const ezUInt32 uiMaxNumTrace = ezMath::Min(62U, ref_trace.GetCount());
    ezInt32 iNumTraces = (*s_pImplementation->captureStackBackTrace)(uiSkip, uiMaxNumTrace, ref_trace.GetPtr(), nullptr);

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
    alignas(_SYMBOL_INFOW) char buffer[1024];
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

      printFunc(finalStr);
    }
  }
}

/// \endcond

#endif
