#include <Foundation/System/Process.h>

ezProcess::ezProcess()
{
  /// \todo Implement ezProcess::Execute on Posix
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezProcess::~ezProcess()
{
  /// \todo Implement ezProcess::Execute on Posix
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezResult ezProcess::Launch()
{
  /// \todo Implement ezProcess::Execute on Posix

  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

ezResult ezProcess::LaunchAsync()
{
  /// \todo Implement ezProcess::LaunchDetached on Posix

  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

ezResult ezProcess::WaitToFinish(ezTime timeout /*= ezTime::Zero()*/)
{
  /// \todo Implement ezProcess::WaitToFinish on Posix

  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

ezResult ezProcess::Terminate()
{
  /// \todo Implement ezProcess::Terminate on Posix

  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}


ezProcessState ezProcess::GetState() const
{
  /// \todo Implement ezProcess::GetState on Posix

  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezProcessState::NotStarted;
}

void ezProcess::Detach()
{
  /// \todo Implement ezProcess::Detach on Posix

  EZ_ASSERT_NOT_IMPLEMENTED;
}
