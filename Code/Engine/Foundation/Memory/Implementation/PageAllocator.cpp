#include <FoundationPCH.h>

#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Memory/PageAllocator.h>
#include <Foundation/System/SystemInformation.h>

static ezAllocatorId GetPageAllocatorId()
{
  static ezAllocatorId id;

  if (id.IsInvalidated())
  {
    id = ezMemoryTracker::RegisterAllocator("Page", ezMemoryTrackingFlags::Default, ezAllocatorId());
  }

  return id;
}

ezAllocatorId ezPageAllocator::GetId()
{
  return GetPageAllocatorId();
}

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Memory/Implementation/Win/PageAllocator_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/Memory/Implementation/Posix/PageAllocator_posix.h>
#else
#  error "ezPageAllocator is not implemented on current platform"
#endif

EZ_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_PageAllocator);
