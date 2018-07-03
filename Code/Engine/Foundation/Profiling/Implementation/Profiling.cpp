#include <PCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Profiling/Profiling.h>

#if EZ_ENABLED(EZ_USE_PROFILING)

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ProfilingSystem)

  // no dependencies

  ON_BASE_STARTUP
  {
    ezProfilingSystem::Initialize();
  }
  ON_CORE_SHUTDOWN
  {
    ezProfilingSystem::Reset();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

// Include inline file
#include <Foundation/Profiling/Implementation/Profiling_EZ_inl.h>

// static
void ezProfilingSystem::Initialize()
{
  SetThreadName("Main Thread");
}

#else

// static
void ezProfilingSystem::SetThreadName(const char* szThreadName) {}

void ezProfilingSystem::Initialize() {}

void ezProfilingSystem::Reset() {}

void ezProfilingSystem::Capture(ezStreamWriter& outputStream) {}

void ezProfilingSystem::RemoveThread() {}

#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Profiling_Implementation_Profiling);
