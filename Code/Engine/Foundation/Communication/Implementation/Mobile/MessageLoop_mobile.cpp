#include <PCH.h>

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#include <Foundation/Communication/Implementation/Mobile/MessageLoop_mobile.h>
#include <Foundation/Communication/IpcChannel.h>

ezMessageLoop_mobile::ezMessageLoop_mobile()
{
}

ezMessageLoop_mobile::~ezMessageLoop_mobile()
{
  StopUpdateThread();
}

void ezMessageLoop_mobile::WakeUp()
{
  // nothing to do
}

bool ezMessageLoop_mobile::WaitForMessages(ezInt32 iTimeout, ezIpcChannel* pFilter)
{
  // nothing to do

  if (iTimeout < 0)
  {
    // if timeout is 'indefinite' wait a little
    ezThreadUtils::YieldTimeSlice();
  }

  return false;
}

#endif



EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Mobile_MessageLoop_mobile);

