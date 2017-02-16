#include <PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Profiling/Profiling.h>

#if EZ_ENABLED(EZ_USE_PROFILING)

EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ProfilingSystem)

  // no dependencies

  ON_BASE_STARTUP
  {
    ezProfilingSystem::Initialize();
  }

EZ_END_SUBSYSTEM_DECLARATION

// Include inline file
#include <Foundation/Profiling/Implementation/Profiling_EZ_inl.h>

//static
void ezProfilingSystem::Initialize()
{
  SetThreadName("Main Thread");
}

#else

//static
void ezProfilingSystem::SetThreadName(const char* szThreadName)
{
}

void ezProfilingSystem::Initialize()
{
}

void ezProfilingSystem::Capture(ezStreamWriter& outputStream)
{
}

#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Profiling_Implementation_Profiling);

