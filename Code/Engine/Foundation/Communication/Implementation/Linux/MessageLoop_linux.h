
#pragma once

#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#if EZ_ENABLED(EZ_PLATFORM_LINUX)

#  include <Foundation/Basics.h>
#  include <Foundation/Communication/Implementation/MessageLoop.h>

class ezIpcChannel;
struct IOContext;

class EZ_FOUNDATION_DLL ezMessageLoop_linux : public ezMessageLoop
{
public:
  ezMessageLoop_linux();
  ~ezMessageLoop_linux();

protected:
  virtual void WakeUp() override;
  virtual bool WaitForMessages(ezInt32 iTimeout, ezIpcChannel* pFilter) override;

private:
};

#endif
