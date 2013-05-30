#include <Foundation/PCH.h>

void ezIAllocator::DumpMemoryLeaks() const
{
  Stats stats;
  GetStats(stats);

  if (stats.m_uiNumLiveAllocations != 0 || stats.m_uiAllocationSize != 0)
  {
    EZ_REPORT_FAILURE("Memory leaks found");
  
    #if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

      // todo: make this platform independent
      wchar_t szName[32];
      mbstowcs(szName, m_szName, 32);
      wchar_t szBuffer[512];
      swprintf_s(szBuffer,
        L"\n--------------------------------------------------------------------\n"
        L"Allocator '%s' has leaks: %llu bytes in %llu allocation(s)"
        L"\n--------------------------------------------------------------------\n\n",
        szName, stats.m_uiAllocationSize, stats.m_uiNumLiveAllocations);
    
      OutputDebugStringW(szBuffer);
  
    #endif
  }
}
