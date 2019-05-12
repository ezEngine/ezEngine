#include <Foundation/System/Process.h>

struct ezProcessImpl
{
  EZ_DECLARE_POD_TYPE();
};

  ezProcess::ezProcess()
{
  /// \todo Implement ezProcess::ezProcess on Posix
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezProcess::~ezProcess()
{
  /// \todo Implement ezProcess::~ezProcess on Posix
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezResult ezProcess::Execute(const ezProcessOptions& opt, ezInt32* out_iExitCode /*= nullptr*/)
{
  /// \todo Implement ezProcess::Execute on Posix

  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

ezResult ezProcess::Launch(const ezProcessOptions& opt, ezBitflags<ezProcessLaunchFlags> launchFlags /*= ezProcessLaunchFlags::None*/)
{
  /// \todo Implement ezProcess::Launch on Posix

  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

ezResult ezProcess::ResumeSuspended()
{
  /// \todo Implement ezProcess::ResumeSuspended on Posix

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

ezOsProcessHandle ezProcess::GetProcessHandle() const
{
  /// \todo Implement ezProcess::GetProcessHandle on Posix

  EZ_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
}

ezOsProcessID ezProcess::GetProcessID() const
{
  /// \todo Implement ezProcess::GetProcessID on Posix

  EZ_ASSERT_NOT_IMPLEMENTED;
  return 0;
}
