#include <Foundation/System/ProcessGroup.h>

struct ezProcessGroupImpl
{
  EZ_DECLARE_POD_TYPE();
};

ezProcessGroup::ezProcessGroup(const char* szGroupName /*= nullptr*/)
{
  /// \todo Implement ezProcessGroup on OSX
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezProcessGroup::~ezProcessGroup()
{
  /// \todo Implement ezProcessGroup on OSX
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezResult ezProcessGroup::Launch(const ezProcessOptions& opt)
{
  /// \todo Implement ezProcessGroup on OSX
  EZ_ASSERT_NOT_IMPLEMENTED;

  return EZ_FAILURE;
}

ezResult ezProcessGroup::WaitToFinish(ezTime timeout /*= ezTime::Zero()*/)
{
  /// \todo Implement ezProcessGroup on OSX
  EZ_ASSERT_NOT_IMPLEMENTED;

  return EZ_FAILURE;
}

ezResult ezProcessGroup::TerminateAll(ezInt32 iForcedExitCode /*= -2*/)
{
  /// \todo Implement ezProcessGroup on OSX
  EZ_ASSERT_NOT_IMPLEMENTED;

  return EZ_FAILURE;
}
