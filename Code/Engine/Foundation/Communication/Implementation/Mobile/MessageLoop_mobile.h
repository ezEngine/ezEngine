#pragma once

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#include <Foundation/Basics.h>
#include <Foundation/Communication/Implementation/MessageLoop.h>

class EZ_FOUNDATION_DLL ezMessageLoop_mobile : public ezMessageLoop
{
public:
  ezMessageLoop_mobile();
  ~ezMessageLoop_mobile();

protected:
  virtual void WakeUp() override;
  virtual bool WaitForMessages(ezInt32 iTimeout, ezIpcChannel* pFilter) override;

private:
};

#endif
