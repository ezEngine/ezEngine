#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/CommonAllocators.h>

#if EZ_ENABLED(EZ_ALLOC_GUARD_ALLOCATIONS)
using DefaultHeapType = ezGuardingAllocator;
using DefaultAlignedHeapType = ezGuardingAllocator;
using DefaultStaticsHeapType = ezAllocatorWithPolicy<ezAllocPolicyGuarding, ezAllocatorTrackingMode::AllocationStatsIgnoreLeaks>;
#else
using DefaultHeapType = ezHeapAllocator;
using DefaultAlignedHeapType = ezAlignedHeapAllocator;
using DefaultStaticsHeapType = ezAllocatorWithPolicy<ezAllocPolicyHeap, ezAllocatorTrackingMode::AllocationStatsIgnoreLeaks>;
#endif

enum
{
  HEAP_ALLOCATOR_BUFFER_SIZE = sizeof(DefaultHeapType),
  ALIGNED_ALLOCATOR_BUFFER_SIZE = sizeof(DefaultAlignedHeapType)
};

alignas(EZ_ALIGNMENT_MINIMUM) static ezUInt8 s_DefaultAllocatorBuffer[HEAP_ALLOCATOR_BUFFER_SIZE];
alignas(EZ_ALIGNMENT_MINIMUM) static ezUInt8 s_StaticAllocatorBuffer[HEAP_ALLOCATOR_BUFFER_SIZE];

alignas(EZ_ALIGNMENT_MINIMUM) static ezUInt8 s_AlignedAllocatorBuffer[ALIGNED_ALLOCATOR_BUFFER_SIZE];

bool ezFoundation::s_bIsInitialized = false;
ezAllocator* ezFoundation::s_pDefaultAllocator = nullptr;
ezAllocator* ezFoundation::s_pAlignedAllocator = nullptr;

void ezFoundation::Initialize()
{
  if (s_bIsInitialized)
    return;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezMemoryUtils::ReserveLower4GBAddressSpace();
#endif

  if (s_pDefaultAllocator == nullptr)
  {
    s_pDefaultAllocator = new (s_DefaultAllocatorBuffer) DefaultHeapType("DefaultHeap");
  }

  if (s_pAlignedAllocator == nullptr)
  {
    s_pAlignedAllocator = new (s_AlignedAllocatorBuffer) DefaultAlignedHeapType("AlignedHeap");
  }

  s_bIsInitialized = true;
}

#if defined(EZ_CUSTOM_STATIC_ALLOCATOR_FUNC)
extern ezAllocator* EZ_CUSTOM_STATIC_ALLOCATOR_FUNC();
#endif

ezAllocator* ezFoundation::GetStaticsAllocator()
{
  static ezAllocator* pStaticAllocator = nullptr;

  if (pStaticAllocator == nullptr)
  {
#if defined(EZ_CUSTOM_STATIC_ALLOCATOR_FUNC)

#  if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)

#    if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    using GetStaticAllocatorFunc = ezAllocator* (*)();

    HMODULE hThisModule = GetModuleHandle(nullptr);
    GetStaticAllocatorFunc func = (GetStaticAllocatorFunc)GetProcAddress(hThisModule, EZ_CUSTOM_STATIC_ALLOCATOR_FUNC);
    if (func != nullptr)
    {
      pStaticAllocator = (*func)();
      return pStaticAllocator;
    }
#    else
#      error "Customizing static allocator not implemented"
#    endif

#  else
    return EZ_CUSTOM_STATIC_ALLOCATOR_FUNC();
#  endif

#endif

    pStaticAllocator = new (s_StaticAllocatorBuffer) DefaultStaticsHeapType("Statics");
  }

  return pStaticAllocator;
}


