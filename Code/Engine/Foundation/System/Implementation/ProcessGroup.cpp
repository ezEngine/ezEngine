#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
// Include inline file
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#    include <Foundation/System/Implementation/Win/ProcessGroup_win.h>
#  else
#    include <Foundation/System/Implementation/ez/ProcessGroup_ez.h>
#  endif

const ezHybridArray<ezProcess, 8>& ezProcessGroup::GetProcesses() const
{
  return m_Processes;
}

#endif

EZ_STATICLINK_FILE(Foundation, Foundation_System_Implementation_ProcessGroup);
