#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Implementation/MessageLoop.h>

class EZ_FOUNDATION_DLL ezMessageLoop_Fallback : public ezMessageLoop
{
public:
  ezMessageLoop_Fallback();
  ~ezMessageLoop_Fallback();

protected:
  virtual void WakeUp() override;
  virtual bool WaitForMessages(ezInt32 iTimeout, ezIpcChannel* pFilter) override;

private:
};
