#include <Foundation/PCH.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Memory/Policies/AlignedAllocation.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadLocalStorage.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Basics/Types/Variant.h>

#if EZ_ENABLED(EZ_USE_GUARDED_ALLOCATOR)
  typedef ezMemoryPolicies::ezGuardedBoundsChecking DefaultAllocatorGuarding;
#else
  typedef ezMemoryPolicies::ezNoBoundsChecking DefaultAllocatorGuarding;
#endif

#if EZ_ENABLED(EZ_USE_TRACE_ALLOCATOR)
  typedef ezMemoryPolicies::ezStackTracking DefaultAllocatorTracking;
#else
  typedef ezMemoryPolicies::ezSimpleTracking DefaultAllocatorTracking;
#endif

typedef ezAllocator<ezMemoryPolicies::ezHeapAllocation, ezMemoryPolicies::ezNoBoundsChecking, 
  ezMemoryPolicies::ezSimpleTracking, ezNoMutex> DebugHeapAllocator;

typedef ezAllocator<ezMemoryPolicies::ezHeapAllocation, DefaultAllocatorGuarding, 
  DefaultAllocatorTracking, ezMutex> DefaultHeapAllocator;

typedef ezAllocator<ezMemoryPolicies::ezAlignedAllocation<ezMemoryPolicies::ezHeapAllocation>, 
  DefaultAllocatorGuarding, DefaultAllocatorTracking, ezMutex> AlignedHeapAllocator;

enum { BUFFER_SIZE = sizeof(ezHeapAllocator) };
static ezUInt8 g_BaseAllocatorBuffer[BUFFER_SIZE];
static ezUInt8 g_StaticAllocatorBuffer[BUFFER_SIZE];

bool ezFoundation::s_bIsInitialized = false;
bool ezFoundation::s_bOwnsBaseAllocator = false;
ezIAllocator* ezFoundation::s_pBaseAllocator = NULL;
ezIAllocator* ezFoundation::s_pDebugAllocator = NULL;
ezIAllocator* ezFoundation::s_pDefaultAllocator = NULL;
ezIAllocator* ezFoundation::s_pAlignedAllocator = NULL;
ezIAllocator* ezFoundation::s_pStaticAllocator = NULL;

ezIAllocator* ezFoundation::s_pBaseAllocatorTemp = NULL;
ezIAllocator* ezFoundation::s_pDebugAllocatorTemp = NULL;
ezIAllocator* ezFoundation::s_pDefaultAllocatorTemp = NULL;
ezIAllocator* ezFoundation::s_pAlignedAllocatorTemp = NULL;

ezFoundation::Config::Config()
{
  ezMemoryUtils::ZeroFill(this);
}

ezFoundation::Config ezFoundation::s_Config;

void ezFoundation::Initialize()
{
  if (s_bIsInitialized)
    return;

  const Config& config = s_Config;

  s_bIsInitialized = true;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezMemoryUtils::ReserveLower4GBAddressSpace();
#endif
   
  if (config.pBaseAllocator == NULL)
  {
    s_bOwnsBaseAllocator = true;
    s_pBaseAllocator = new (g_BaseAllocatorBuffer) ezHeapAllocator("Base");
  }
  else
  {
    s_pBaseAllocator = config.pBaseAllocator;
  }

  if (config.pDebugAllocator == NULL)
    s_pDebugAllocator = EZ_NEW(s_pBaseAllocator, DebugHeapAllocator)("Debug");
  else
    s_pDebugAllocator = config.pDebugAllocator;

  if (config.pDefaultAllocator == NULL)
    s_pDefaultAllocator = EZ_NEW(s_pBaseAllocator, DefaultHeapAllocator)("DefaultHeap");
  else
    s_pDefaultAllocator = config.pDefaultAllocator;

  if (config.pAlignedAllocator == NULL)
    s_pAlignedAllocator = EZ_NEW(s_pBaseAllocator, AlignedHeapAllocator)("AlignedHeap");
  else
    s_pAlignedAllocator = config.pAlignedAllocator;
}

void ezFoundation::Shutdown()
{
  if (!s_bIsInitialized)
    return;

  EZ_DELETE(s_pBaseAllocator, s_pAlignedAllocator);
  EZ_DELETE(s_pBaseAllocator, s_pDefaultAllocator);
  EZ_DELETE(s_pBaseAllocator, s_pDebugAllocator);
  // the static Allocator must not be deleted, it might still be used during application shutdown
  
  if (s_bOwnsBaseAllocator)
  {
    s_pBaseAllocator->~ezIAllocator();
    s_pBaseAllocator = NULL;
  }

  s_bIsInitialized = false;
}

ezIAllocator* ezFoundation::GetStaticAllocator()
{
  if (s_pStaticAllocator == NULL)
  {
    s_pStaticAllocator = new (g_StaticAllocatorBuffer) ezHeapAllocator("Statics");
  }

  return s_pStaticAllocator;
}
