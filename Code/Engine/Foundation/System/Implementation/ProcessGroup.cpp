#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)

#  include <Foundation/System/ProcessGroup.h>

const ezHybridArray<ezProcess, 8>& ezProcessGroup::GetProcesses() const
{
  return m_Processes;
}

#endif
