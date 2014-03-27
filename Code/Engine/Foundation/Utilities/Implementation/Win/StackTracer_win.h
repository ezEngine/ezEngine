
#ifdef EZ_STACKTRACER_WIN_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_STACKTRACER_WIN_INL_H_INCLUDED

#include <DbgHelp.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Math/Math.h>

// Deactivate Doxygen document generation for the following block.
/// \cond

namespace
{
  typedef WORD (__stdcall *CaptureStackBackTraceFunc)(DWORD FramesToSkip, DWORD FramesToCapture, 
    PVOID *BackTrace, PDWORD BackTraceHash);

  typedef BOOL (__stdcall *SymbolInitializeFunc)(HANDLE hProcess, PCWSTR UserSearchPath, 
    BOOL fInvadeProcess);

  typedef DWORD64 (__stdcall *SymbolLoadModuleFunc)(HANDLE hProcess, HANDLE hFile, PCWSTR ImageName,
    PCWSTR ModuleName, DWORD64 BaseOfDll, DWORD DllSize, PMODLOAD_DATA Data, DWORD Flags); 

  typedef BOOL (__stdcall *SymbolGetModuleInfoFunc)(HANDLE hProcess, DWORD64 qwAddr,
    PIMAGEHLP_MODULEW64 ModuleInfo);

  typedef BOOL (__stdcall *SymbolFromAddressFunc)(HANDLE hProcess, DWORD64 Address, 
    PDWORD64 Displacement, PSYMBOL_INFOW Symbol);

  typedef BOOL (__stdcall *LineFromAddressFunc)(HANDLE hProcess, DWORD64 Address,
    PDWORD64 Displacement, PIMAGEHLP_LINEW64 Line);

  struct StackTracerImplementation
  {
    HMODULE kernel32Dll;
    HMODULE dbgHelpDll;

    CaptureStackBackTraceFunc captureStackBackTrace;
    SymbolInitializeFunc symbolInitialize;
    SymbolLoadModuleFunc symbolLoadModule;
    SymbolGetModuleInfoFunc getModuleInfo;
    SymbolFromAddressFunc symbolFromAddress;
    LineFromAddressFunc lineFromAdress;

    StackTracerImplementation()
    {
      ezMemoryUtils::ZeroFill(this);

      kernel32Dll = LoadLibraryW(L"kernel32.dll");
      EZ_ASSERT(kernel32Dll != NULL, "StackTracer could not load kernel32.dll");
      if (kernel32Dll != NULL)
      {
        captureStackBackTrace = (CaptureStackBackTraceFunc)GetProcAddress(kernel32Dll, 
          "RtlCaptureStackBackTrace");
      }

      dbgHelpDll = LoadLibraryW(L"dbghelp.dll");
      EZ_ASSERT(dbgHelpDll != NULL, "StackTracer could not load dbghelp.dll");
      if (dbgHelpDll != NULL)
      {
        symbolInitialize = (SymbolInitializeFunc)GetProcAddress(dbgHelpDll, "SymInitializeW");
        symbolLoadModule = (SymbolLoadModuleFunc)GetProcAddress(dbgHelpDll, "SymLoadModuleExW");
        getModuleInfo = (SymbolGetModuleInfoFunc)GetProcAddress(dbgHelpDll, "SymGetModuleInfoW64");
        if (symbolInitialize == NULL || symbolLoadModule == NULL || getModuleInfo == NULL)
          return;

        if (!(*symbolInitialize)(GetCurrentProcess(), NULL, TRUE))
        {
          DWORD err = GetLastError();
          LPVOID lpMsgBuf = NULL;

          FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
            err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

          char errStr[1024];
          sprintf_s(errStr, "StackTracer could not initialize symbols. Error-Code %u (\"%s\")", err, lpMsgBuf);
          OutputDebugStringA(errStr);

          LocalFree(lpMsgBuf);
          return;
        }

        symbolFromAddress = (SymbolFromAddressFunc)GetProcAddress(dbgHelpDll, "SymFromAddrW");
        lineFromAdress = (LineFromAddressFunc)GetProcAddress(dbgHelpDll, "SymGetLineFromAddrW64");
      }
    }    
  };

  static StackTracerImplementation* s_pImplementation;

  static void OnPluginEvent(const ezPlugin::PluginEvent& e)
  {
    if (s_pImplementation->symbolLoadModule == NULL || s_pImplementation->getModuleInfo == NULL)
      return;

    if (e.m_EventType == ezPlugin::PluginEvent::AfterLoading)
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
      
      DWORD64 moduleAddress = (*s_pImplementation->symbolLoadModule)(currentProcess, NULL, szPluginPath, szPluginName, 0, 0, NULL, 0);
      if (moduleAddress == 0)
      {
        DWORD err = GetLastError();
        if (err != ERROR_SUCCESS)
        {
          LPVOID lpMsgBuf = NULL;

          FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
            err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

          char errStr[1024];
          sprintf_s(errStr, "StackTracer could not load symbols for '%s'. Error-Code %u (\"%s\")", e.m_szPluginFile, err, lpMsgBuf);
          OutputDebugStringA(errStr);

          LocalFree(lpMsgBuf);
        }

        return;
      }

      IMAGEHLP_MODULEW64 moduleInfo;
      ezMemoryUtils::ZeroFill(&moduleInfo);
      moduleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULEW64);

      if (!(*s_pImplementation->getModuleInfo)(currentProcess, moduleAddress, &moduleInfo))
      {
        DWORD err = GetLastError();
        LPVOID lpMsgBuf = NULL;

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
            err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

        char errStr[1024];
        sprintf_s(errStr, "StackTracer could not get module info for '%s'. Error-Code %u (\"%s\")", e.m_szPluginFile, err, lpMsgBuf);
        OutputDebugStringA(errStr);

        LocalFree(lpMsgBuf);
      }
    }
  }

  static void Initialize()
  {
    if (s_pImplementation == NULL)
    {
      static ezUInt8 ImplementationBuffer[sizeof(StackTracerImplementation)];
      s_pImplementation = new (ImplementationBuffer) StackTracerImplementation();
      EZ_ASSERT(s_pImplementation != NULL, "StackTracer initialization failed");

      ezPlugin::s_PluginEvents.AddEventHandler(OnPluginEvent);
    }
  }
}

//static
ezUInt32 ezStackTracer::GetStackTrace(ezArrayPtr<void*>& trace)
{
  Initialize();

  if (s_pImplementation->captureStackBackTrace != NULL)
  {
    const ezUInt32 uiSkip = 1;
    const ezUInt32 uiMaxNumTrace = ezMath::Min(62U, trace.GetCount() - uiSkip);
    ezInt32 iNumTraces = (*s_pImplementation->captureStackBackTrace)(uiSkip, uiMaxNumTrace, trace.GetPtr(), NULL);

    // skip the last three stackframes since they are useless
    return ezMath::Max(iNumTraces - 3, 0);
  }
  return 0;
}

//static
void ezStackTracer::ResolveStackTrace(const ezArrayPtr<void*>& trace, PrintFunc printFunc)
{
  Initialize();

  if (s_pImplementation->symbolFromAddress != NULL && s_pImplementation->lineFromAdress != NULL)
  {
    char buffer[1024];
    HANDLE currentProcess = GetCurrentProcess();

    const ezUInt32 uiNumTraceEntries = trace.GetCount();
    for (ezUInt32 i = 0; i < uiNumTraceEntries; i++)
    {
      DWORD64 pSymbolAddress = reinterpret_cast<UINT_PTR>(trace[i]);

      _SYMBOL_INFOW& symbolInfo = *(_SYMBOL_INFOW*)buffer;
      ezMemoryUtils::ZeroFill(&symbolInfo);
      symbolInfo.SizeOfStruct = sizeof(_SYMBOL_INFOW);
      symbolInfo.MaxNameLen = (EZ_ARRAY_SIZE(buffer) - symbolInfo.SizeOfStruct) / sizeof(WCHAR);

      DWORD64 displacement = 0;
      BOOL result = (*s_pImplementation->symbolFromAddress)(currentProcess, pSymbolAddress, 
        &displacement, &symbolInfo);
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

