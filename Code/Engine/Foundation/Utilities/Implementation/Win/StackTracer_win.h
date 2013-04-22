
#ifdef EZ_STACKTRACER_WIN_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_STACKTRACER_WIN_INL_H_INCLUDED

#include <Foundation/Math/Math.h>

// Deactivate Doxygen document generation for the following block.
/// \cond

typedef WORD (__stdcall *CaptureStackBackTraceFunc)(DWORD FramesToSkip, DWORD FramesToCapture, 
  PVOID *BackTrace, PDWORD BackTraceHash);

typedef BOOL (__stdcall *SymbolInitializeFunc)(HANDLE hProcess, PCWSTR UserSearchPath, 
  BOOL fInvadeProcess);

typedef BOOL (__stdcall *SymbolFromAddressFunc)(HANDLE hProcess, DWORD64 Address, 
  PDWORD64 Displacement, PSYMBOL_INFOW Symbol);

typedef BOOL (__stdcall *LineFromAddressFunc)(HANDLE hProcess, DWORD64 Address, 
  PDWORD64 Displacement, PIMAGEHLP_LINEW64 Line);

struct StackTracerImplementation
{
  bool isInitialized;

  HMODULE kernel32Dll;
  HMODULE dbgHelpDll;

  CaptureStackBackTraceFunc captureStackBackTrace;
  SymbolInitializeFunc symbolInitialize;
  SymbolFromAddressFunc symbolFromAddress;
  LineFromAddressFunc lineFromAdress;

  EZ_FORCE_INLINE StackTracerImplementation()
  {
    ezMemoryUtils::ZeroFill(this);
  }

  void Initialize()
  {
    if (isInitialized)
      return;

    kernel32Dll = LoadLibrary("kernel32.dll");
    EZ_ASSERT(kernel32Dll != NULL, "StackTracer could not load kernel32.dll");
    if (kernel32Dll != NULL)
    {
      captureStackBackTrace = (CaptureStackBackTraceFunc)GetProcAddress(kernel32Dll, 
        "RtlCaptureStackBackTrace");
    }

    dbgHelpDll = LoadLibrary("dbghelp.dll");
    EZ_ASSERT(dbgHelpDll != NULL, "StackTracer could not load dbghelp.dll");
    if (dbgHelpDll != NULL)
    {
      symbolInitialize = (SymbolInitializeFunc)GetProcAddress(dbgHelpDll, "SymInitializeW");
      if (symbolInitialize == NULL)
        return;

      if (!(*symbolInitialize)(GetCurrentProcess(), NULL, TRUE))
      {
        EZ_ASSERT(false, "StackTracer could not initialize symbols");
        return;
      }

      symbolFromAddress = (SymbolFromAddressFunc)GetProcAddress(dbgHelpDll, "SymFromAddrW");
      lineFromAdress = (LineFromAddressFunc)GetProcAddress(dbgHelpDll, "SymGetLineFromAddrW64");
    }

    isInitialized = true;
  }
};

StackTracerImplementation g_implementation; 

//static
ezUInt32 ezStackTracer::GetStackTrace(ezArrayPtr<void*>& trace)
{
  g_implementation.Initialize();
  if (g_implementation.captureStackBackTrace != NULL)
  {
    const ezUInt32 uiSkip = 1;
    const ezUInt32 uiMaxNumTrace = ezMath::Min(62U, trace.GetCount() - uiSkip);
    ezInt32 iNumTraces = (*g_implementation.captureStackBackTrace)(uiSkip, uiMaxNumTrace, trace.GetPtr(), NULL);

    // skip the last three stackframes since they are useless
    return ezMath::Max(iNumTraces - 3, 0);
  }
  return 0;
}

//static
void ezStackTracer::DumpStackTrace(const ezArrayPtr<void*>& trace)
{
  g_implementation.Initialize();
  if (g_implementation.symbolFromAddress != NULL && g_implementation.lineFromAdress != NULL)
  {
    char buffer[1024];
    HANDLE currentProcess = GetCurrentProcess();

    const ezUInt32 uiNumTraceEntries = trace.GetCount();
    for (ezUInt32 i = 0; i < uiNumTraceEntries; i++)
    {
      DWORD64 pSymbolAddress = reinterpret_cast<UINT_PTR>(trace[i]);

      _SYMBOL_INFOW& symbolInfo = *(_SYMBOL_INFOW*)buffer;
      memset(&symbolInfo, 0, sizeof(_SYMBOL_INFOW));
      symbolInfo.SizeOfStruct = sizeof(_SYMBOL_INFOW);
      symbolInfo.MaxNameLen = (1024 - symbolInfo.SizeOfStruct) / sizeof(WCHAR);

      DWORD64 displacement = 0;
      BOOL result = (*g_implementation.symbolFromAddress)(currentProcess, pSymbolAddress, 
        &displacement, &symbolInfo);
      if (!result)
      {
        wcscpy(symbolInfo.Name, L"<Unknown>");
      }
      
      IMAGEHLP_LINEW64 lineInfo;
      memset(&lineInfo, 0, sizeof(lineInfo));
      lineInfo.SizeOfStruct = sizeof(lineInfo);
      (*g_implementation.lineFromAdress)(currentProcess, pSymbolAddress, &displacement, &lineInfo);

      wchar_t str[1024];
      swprintf_s(str, L"%s(%i):'%s'\n", lineInfo.FileName, lineInfo.LineNumber, symbolInfo.Name);
      OutputDebugStringW(str);
    }
  }
}


/// \endcond