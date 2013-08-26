#include <Foundation/PCH.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezIAllocator);

ezIAllocator::Stats operator+ (const ezIAllocator::Stats& lhs, const ezIAllocator::Stats& rhs)
{
  ezIAllocator::Stats res;
  res.m_uiAllocationSize      = lhs.m_uiAllocationSize      + rhs.m_uiAllocationSize;
  res.m_uiNumAllocations      = lhs.m_uiNumAllocations      + rhs.m_uiNumAllocations;
  res.m_uiNumDeallocations    = lhs.m_uiNumDeallocations    + rhs.m_uiNumDeallocations;
  res.m_uiNumLiveAllocations  = lhs.m_uiNumLiveAllocations  + rhs.m_uiNumLiveAllocations;
  res.m_uiUsedMemorySize      = lhs.m_uiUsedMemorySize      + rhs.m_uiUsedMemorySize;

  return res;
}

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
