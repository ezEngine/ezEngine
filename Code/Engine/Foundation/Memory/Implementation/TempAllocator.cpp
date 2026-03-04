#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/AllocatorWithPolicy.h>
#include <Foundation/Memory/Policies/AllocPolicyStack.h>
#include <Foundation/Memory/TempAllocator.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, TempAllocator)

  ON_CORESYSTEMS_STARTUP
  {
    ezTempAllocator::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezTempAllocator::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezAllocator* ezTempAllocator::s_pAllocator;

// static
void ezTempAllocator::Startup()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  static constexpr bool OverwriteMemoryOnFree = true;
#else
  static constexpr bool OverwriteMemoryOnFree = false;
#endif
  using StackAllocatorType = ezAllocatorWithPolicy<ezAllocPolicyStack<OverwriteMemoryOnFree>, ezAllocatorTrackingMode::Basics>;

  s_pAllocator = EZ_DEFAULT_NEW(StackAllocatorType, "TempAllocator", ezFoundation::GetAlignedAllocator());
}

// static
void ezTempAllocator::Shutdown()
{
  EZ_DEFAULT_DELETE(s_pAllocator);
}


EZ_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_TempAllocator);
