#include <FoundationPCH.h>

#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
// Include inline file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/System/Implementation/Win/ProcessGroup_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
#  include <Foundation/System/Implementation/OSX/ProcessGroup_OSX.h>
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <Foundation/System/Implementation/Posix/ProcessGroup_posix.h>
#else
#  error "ProcessGroup functions are not implemented on current platform"
#endif

const ezHybridArray<ezProcess, 8>& ezProcessGroup::GetProcesses() const
{
  return m_Processes;
}

#endif

EZ_STATICLINK_FILE(Foundation, Foundation_System_Implementation_ProcessGroup);
