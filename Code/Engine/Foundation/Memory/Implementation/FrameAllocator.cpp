#include <Foundation/PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, FrameAllocator)

  ON_CORE_STARTUP
  {
    ezFrameAllocator::Startup();
  }

  ON_CORE_SHUTDOWN
  {
    ezFrameAllocator::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION

namespace
{
  ezFrameAllocator::StackAllocatorType* g_Allocators[2];
  ezProfilingId g_SwapProfilingId = ezProfilingSystem::CreateId("FrameAllocator.Swap");
}

ezFrameAllocator::StackAllocatorType* ezFrameAllocator::s_pCurrentAllocator;

// static 
void ezFrameAllocator::Swap()
{
  EZ_PROFILE(g_SwapProfilingId);

  s_pCurrentAllocator = (s_pCurrentAllocator == g_Allocators[0]) ? g_Allocators[1] : g_Allocators[0];
  s_pCurrentAllocator->Reset();
}

// static 
void ezFrameAllocator::Reset()
{
  g_Allocators[0]->Reset();
  g_Allocators[1]->Reset();
}

// static
void ezFrameAllocator::Startup()
{
  g_Allocators[0] = EZ_DEFAULT_NEW(StackAllocatorType, "FrameAllocator0", ezFoundation::GetDefaultAllocator());
  g_Allocators[1] = EZ_DEFAULT_NEW(StackAllocatorType, "FrameAllocator1", ezFoundation::GetDefaultAllocator());

  s_pCurrentAllocator = g_Allocators[0];
}

// static 
void ezFrameAllocator::Shutdown()
{
  g_Allocators[0]->Reset();
  g_Allocators[1]->Reset();

  EZ_DEFAULT_DELETE(g_Allocators[0]);
  EZ_DEFAULT_DELETE(g_Allocators[1]);

  s_pCurrentAllocator = nullptr;
}
