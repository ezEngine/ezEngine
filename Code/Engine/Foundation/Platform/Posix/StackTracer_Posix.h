#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/StackTracer.h>

#include <Foundation/Math/Math.h>

#if __has_include(<execinfo.h>)
#  include <execinfo.h>
#  define HAS_EXECINFO 1
#endif

#if __has_include(<dlfcn.h>)
#  include <dlfcn.h>
#  define HAS_DLFCN 1
#endif
void ezStackTracer::OnPluginEvent(const ezPluginEvent& e)
{
}

// static
ezUInt32 ezStackTracer::GetStackTrace(ezArrayPtr<void*>& trace, void* pContext)
{
#if HAS_EXECINFO
  return backtrace(trace.GetPtr(), trace.GetCount());
#else
  return 0;
#endif
}

// static
void ezStackTracer::ResolveStackTrace(const ezArrayPtr<void*>& trace, PrintFunc printFunc)
{
#if HAS_EXECINFO
  char szBuffer[512] = {0};

  char** ppSymbols = backtrace_symbols(trace.GetPtr(), trace.GetCount());

  // Demangle if possible, otherwise fallback to backtrace_symbols output.
  if (ppSymbols != nullptr)
  {
    for (ezUInt32 i = 0; i < trace.GetCount(); i++)
    {
#  if HAS_DLFCN
      Dl_info info{0};
      if (dladdr(trace[i], &info))
      {
        int iStatus = 0;
        char* szDemangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &iStatus);
        EZ_SCOPE_EXIT(free(szDemangled));
        if (szDemangled != nullptr)
        {
          ezUInt32 uiOffset = static_cast<ezUInt32>((char*)trace[i] - (char*)info.dli_saddr);
          ezStringUtils::snprintf(szBuffer, EZ_ARRAY_SIZE(szBuffer), "%s(%s+0x%x) [0x%llx]\n", info.dli_fname, szDemangled, uiOffset, (ezUInt64)trace[i]);
          printFunc(szBuffer);
          continue;
        }
      }
#  endif
      ezStringUtils::snprintf(szBuffer, EZ_ARRAY_SIZE(szBuffer), "%s\n", ppSymbols[i]);
      printFunc(szBuffer);
    }
#  if HAS_DLFCN
    // Addr2line commands:
    printFunc("*** Run in terminal to resolve file and line callstack: ***");
    for (ezUInt32 i = 0; i < trace.GetCount(); i++)
    {
      Dl_info info{0};
      if (dladdr(trace[i], &info))
      {
        ptrdiff_t offset = (char*)trace[i] - (char*)info.dli_fbase;
        ezStringUtils::snprintf(szBuffer, EZ_ARRAY_SIZE(szBuffer), "addr2line -e %s -C -f -s -p 0x%llx\n", info.dli_fname, (ezUInt64)offset);
        printFunc(szBuffer);
      }
    }
  }
#  endif
  free(ppSymbols);

#else
  printFunc("Could not record stack trace on this Linux system, because execinfo.h is not available.");
#endif
}
