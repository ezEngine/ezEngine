#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX) && EZ_ENABLED(EZ_SUPPORTS_PROCESSES)

#  include <Foundation/System/ProcessGroup.h>

namespace ezInternal
{
  bool SetProcessLaunchParentDeathSignal(bool bEnable);
}

struct ezProcessGroupImpl
{
  EZ_DECLARE_POD_TYPE();
};

ezProcessGroup::ezProcessGroup(ezStringView sGroupName)
{
  EZ_IGNORE_UNUSED(sGroupName);
}

ezProcessGroup::~ezProcessGroup()
{
  TerminateAll().IgnoreResult();
}

ezResult ezProcessGroup::Launch(const ezProcessOptions& opt)
{
  ezProcess& process = m_Processes.ExpandAndGetRef();

  const bool bPreviousValue = ezInternal::SetProcessLaunchParentDeathSignal(true);
  ezResult result = process.Launch(opt);
  ezInternal::SetProcessLaunchParentDeathSignal(bPreviousValue);

  return result;
}

ezResult ezProcessGroup::WaitToFinish(ezTime timeout /*= ezTime::MakeZero()*/)
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
  EZ_IGNORE_UNUSED(iForcedExitCode);

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
#endif
