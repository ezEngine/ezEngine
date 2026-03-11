#pragma once

#include <Foundation/Memory/Allocator.h>

/// \brief Stack-based allocator for temporary allocations.
///
/// This allocator is designed for short-lived allocations that ideally follow a LIFO pattern but can also handle out-of-order deallocations.
class EZ_FOUNDATION_DLL ezTempAllocator
{
public:
  EZ_ALWAYS_INLINE static ezAllocator* Get() { return s_pAllocator; }

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, TempAllocator);

  static void Startup();
  static void Shutdown();

  static ezAllocator* s_pAllocator;
};

/// \brief Wrapper for the allocator that is used for temporary allocations.
struct ezTempAllocatorWrapper
{
  EZ_ALWAYS_INLINE static ezAllocator* GetAllocator() { return ezTempAllocator::Get(); }
};
