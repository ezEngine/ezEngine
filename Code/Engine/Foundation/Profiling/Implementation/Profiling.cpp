#include <Foundation/PCH.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Configuration/Startup.h>


EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ProfilingSystem)

  // no dependencies

  ON_BASE_STARTUP
  {
    ezProfilingSystem::Initialize();
  }

  ON_BASE_SHUTDOWN
  {
    ezProfilingSystem::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION



// For now there are only stubs: (remove when starting to implement the profiling system)


ezProfilingScope::ezProfilingScope(const char* pszSampleName, const char* pszFileName, const char* pszFunctionName, const ezUInt32 uiLineNumber)
{
  // This function has to be in the specific implementation (e.g. Profiling_GPA_inl.h)
}

ezProfilingScope::~ezProfilingScope()
{
  // This function has to be in the specific implementation (e.g. Profiling_GPA_inl.h)
}


void ezProfilingSystem::Initialize()
{
  // This function has to be in the specific implementation (e.g. Profiling_GPA_inl.h)
}

void ezProfilingSystem::Shutdown()
{
  // This function has to be in the specific implementation (e.g. Profiling_GPA_inl.h)
}

/*
// Include inline file
#if EZ_PLATFORM_WINDOWS
#include <Foundation/Profiling/Implementation/Win/OSThread_win.h>
#else
#error "Thread functions are not implemented on current platform"
#endif
*/


