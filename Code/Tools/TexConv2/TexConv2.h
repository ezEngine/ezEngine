#pragma once

#include <Core/Application/Application.h>

class ezTexConv2 : public ezApplication
{
public:
  typedef ezApplication SUPER;

  ezTexConv2();

  virtual ApplicationExecution Run() override;
  virtual void BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
};
