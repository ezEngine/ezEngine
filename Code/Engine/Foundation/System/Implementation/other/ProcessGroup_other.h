
///
/// Implements ezProcessGroup by using ezProcess
///

#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/ProcessGroup.h>

struct ezProcessGroupImpl
{
  EZ_DECLARE_POD_TYPE();
};

ezProcessGroup::ezProcessGroup(ezStringView sGroupName)
{
}

ezProcessGroup::~ezProcessGroup()
{
  TerminateAll().IgnoreResult();
}

ezResult ezProcessGroup::Launch(const ezProcessOptions& opt)
{
  ezProcess& process = m_Processes.ExpandAndGetRef();
  return process.Launch(opt);
}

ezResult ezProcessGroup::WaitToFinish(ezTime timeout /*= ezTime::Zero()*/)
{
  for (auto& process : m_Processes)
  {
    if (process.GetState() != ezProcessState::Finished && process.WaitToFinish(timeout).Failed())
    {
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezProcessGroup::TerminateAll(ezInt32 iForcedExitCode /*= -2*/)
{
  auto result = EZ_SUCCESS;
  for (auto& process : m_Processes)
  {
    if (process.GetState() == ezProcessState::Running && process.Terminate().Failed())
    {
      result = EZ_FAILURE;
    }
  }

  return result;
}
