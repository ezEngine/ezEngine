#include <Foundation/PCH.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Communication/GlobalEvent.h>

enum 
{ 
  HEAP_ALLOCATOR_BUFFER_SIZE = sizeof(ezHeapAllocator),
  ALIGNED_ALLOCATOR_BUFFER_SIZE = sizeof(ezAlignedHeapAllocator)
};

static ezUInt8 s_DefaultAllocatorBuffer[HEAP_ALLOCATOR_BUFFER_SIZE];
static ezUInt8 s_StaticAllocatorBuffer[HEAP_ALLOCATOR_BUFFER_SIZE];

static ezUInt8 s_AlignedAllocatorBuffer[ALIGNED_ALLOCATOR_BUFFER_SIZE];

bool ezFoundation::s_bIsInitialized = false;
ezAllocatorBase* ezFoundation::s_pDefaultAllocator = nullptr;
ezAllocatorBase* ezFoundation::s_pAlignedAllocator = nullptr;

void ezFoundation::Initialize()
{
  if (s_bIsInitialized)
    return;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezMemoryUtils::ReserveLower4GBAddressSpace();
#endif
   
  if (s_pDefaultAllocator == nullptr)
  {
    s_pDefaultAllocator = new (s_DefaultAllocatorBuffer) ezHeapAllocator("DefaultHeap");
  }

  if (s_pAlignedAllocator == nullptr)
  {
    s_pAlignedAllocator = new (s_AlignedAllocatorBuffer) ezAlignedHeapAllocator("AlignedHeap");
  }

  s_bIsInitialized = true;
}

#if defined(EZ_CUSTOM_STATIC_ALLOCATOR_FUNC)
  extern ezAllocatorBase* EZ_CUSTOM_STATIC_ALLOCATOR_FUNC();
#endif

ezAllocatorBase* ezFoundation::GetStaticAllocator()
{
  static ezAllocatorBase* pStaticAllocator = nullptr;

  if (pStaticAllocator == nullptr)
  {
#if defined(EZ_CUSTOM_STATIC_ALLOCATOR_FUNC)

#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
    
  #if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    typedef ezAllocatorBase* (*GetStaticAllocatorFunc)();

    HMODULE hThisModule = GetModuleHandle(nullptr);
    GetStaticAllocatorFunc func = (GetStaticAllocatorFunc)GetProcAddress(hThisModule, EZ_CUSTOM_STATIC_ALLOCATOR_FUNC);
    if (func != nullptr)
    {
      pStaticAllocator = (*func)();
      return pStaticAllocator;
    }
  #else
    #error "Customizing static allocator not implemented"
  #endif

#else
    return EZ_CUSTOM_STATIC_ALLOCATOR_FUNC();
#endif

#endif

    pStaticAllocator = new (s_StaticAllocatorBuffer) ezHeapAllocator(EZ_STATIC_ALLOCATOR_NAME);
  }

  return pStaticAllocator;
}




EZ_STATICLINK_FILE(Foundation, Foundation_Basics_Basics);

