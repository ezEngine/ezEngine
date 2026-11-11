#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class ezRasterizerExplorerApp : public ezGameApplication
{
public:
  using SUPER = ezGameApplication;

  ezRasterizerExplorerApp();

protected:
  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
};
