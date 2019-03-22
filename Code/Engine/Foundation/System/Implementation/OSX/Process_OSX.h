#include <Foundation/System/Process.h>

ezProcess::ezProcess()
{
  /// \todo Implement ezProcess::ezProcess on OSX
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezProcess::~ezProcess()
{
  /// \todo Implement ~ezProcess::ezProcess on OSX
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezResult ezProcess::Launch()
{
  /// \todo Implement ezProcess::Execute on OSX

  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

ezResult ezProcess::LaunchAsync()
{
  /// \todo Implement ezProcess::LaunchDetached on OSX

  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

ezResult ezProcess::WaitToFinish(ezTime timeout /*= ezTime::Zero()*/)
{
  /// \todo Implement ezProcess::WaitToFinish on OSX

  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

ezResult ezProcess::Terminate()
{
  /// \todo Implement ezProcess::Terminate on OSX

  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

ezProcessState ezProcess::GetState() const
{
  /// \todo Implement ezProcess::GetState on OSX

  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezProcessState::NotStarted;
}

void ezProcess::Detach()
{
  /// \todo Implement ezProcess::Detach on OSX

  EZ_ASSERT_NOT_IMPLEMENTED;
}
