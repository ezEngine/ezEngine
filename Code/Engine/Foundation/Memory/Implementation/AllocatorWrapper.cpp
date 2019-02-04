#include <PCH.h>

#include <Foundation/Memory/AllocatorWrapper.h>

static thread_local ezAllocatorBase* s_pAllocator = nullptr;

ezLocalAllocatorWrapper::ezLocalAllocatorWrapper(ezAllocatorBase* pAllocator)
{
  s_pAllocator = pAllocator;
}

void ezLocalAllocatorWrapper::Reset()
{
  s_pAllocator = nullptr;
}

ezAllocatorBase* ezLocalAllocatorWrapper::GetAllocator()
{
  return s_pAllocator;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_AllocatorWrapper);

