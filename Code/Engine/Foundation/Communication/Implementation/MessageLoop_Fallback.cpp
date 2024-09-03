#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/MessageLoop_Fallback.h>
#include <Foundation/Communication/IpcChannel.h>

ezMessageLoop_Fallback::ezMessageLoop_Fallback() = default;

ezMessageLoop_Fallback::~ezMessageLoop_Fallback()
{
  StopUpdateThread();
}

void ezMessageLoop_Fallback::WakeUp()
{
  // nothing to do
}

bool ezMessageLoop_Fallback::WaitForMessages(ezInt32 iTimeout, ezIpcChannel* pFilter)
{
  EZ_IGNORE_UNUSED(pFilter);

  // nothing to do

  if (iTimeout < 0)
  {
    // if timeout is 'indefinite' wait a little
    ezThreadUtils::YieldTimeSlice();
  }

  return false;
}
