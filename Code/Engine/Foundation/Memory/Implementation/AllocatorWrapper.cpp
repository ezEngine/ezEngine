#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/AllocatorWrapper.h>

static thread_local ezAllocator* s_pAllocator = nullptr;

ezLocalAllocatorWrapper::ezLocalAllocatorWrapper(ezAllocator* pAllocator)
{
  s_pAllocator = pAllocator;
}

void ezLocalAllocatorWrapper::Reset()
{
  s_pAllocator = nullptr;
}

ezAllocator* ezLocalAllocatorWrapper::GetAllocator()
{
  return s_pAllocator;
}
