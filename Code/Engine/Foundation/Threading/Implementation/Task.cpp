#include <Foundation/FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Implementation/Task.h>

ezTask::ezTask() = default;
ezTask::~ezTask() = default;

void ezTask::Reset()
{
  m_iRemainingRuns = (int)ezMath::Max(1u, m_uiMultiplicity);
  m_bCancelExecution = false;
  m_bTaskIsScheduled = false;
  m_bUsesMultiplicity = m_uiMultiplicity > 0;
}

void ezTask::ConfigureTask(const char* szTaskName, ezTaskNesting nestingMode, ezOnTaskFinishedCallback callback /*= ezOnTaskFinishedCallback()*/)
{
  EZ_ASSERT_DEV(IsTaskFinished(), "This function must be called before the task is started.");

  m_sTaskName = szTaskName;
  m_NestingMode = nestingMode;
  m_OnTaskFinished = callback;
}

void ezTask::SetMultiplicity(ezUInt32 uiMultiplicity)
{
  m_uiMultiplicity = uiMultiplicity;
  m_bUsesMultiplicity = m_uiMultiplicity > 0;
}

void ezTask::Run(ezUInt32 uiInvocation)
{
  // actually this should not be possible to happen
  if (m_iRemainingRuns == 0 || m_bCancelExecution)
  {
    m_iRemainingRuns = 0;
    return;
  }

  {
    ezStringBuilder scopeName("Task: ", m_sTaskName);

    if (m_bUsesMultiplicity)
      scopeName.AppendFormat("-{}", uiInvocation);

    EZ_PROFILE_SCOPE(scopeName.GetData());

    if (m_bUsesMultiplicity)
    {
      ExecuteWithMultiplicity(uiInvocation);
    }
    else
    {
      Execute();
    }
  }

  m_iRemainingRuns.Decrement();
}
