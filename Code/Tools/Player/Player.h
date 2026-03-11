#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class ezPlayerApplication : public ezGameApplication
{
public:
  using SUPER = ezGameApplication;

  ezPlayerApplication();

protected:
  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;

private:
  void DetermineProjectPath();
};
