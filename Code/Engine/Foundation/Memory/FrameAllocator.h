#pragma once

#include <Foundation/Memory/StackAllocator.h>

/// \brief A double buffered stack allocator
class EZ_FOUNDATION_DLL ezFrameAllocator
{
public:
  typedef ezStackAllocator<ezMemoryTrackingFlags::None> StackAllocatorType;

  EZ_FORCE_INLINE static ezAllocatorBase* GetCurrentAllocator()
  {
    return s_pCurrentAllocator;
  }

  static void Swap();

  static void Reset(); 

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, FrameAllocator);

  static void Startup();
  static void Shutdown();

  static StackAllocatorType* s_pCurrentAllocator;
};
