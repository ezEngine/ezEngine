#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class CppProjectGame : public ezGameApplication
{
public:
  using SUPER = ezGameApplication;

  CppProjectGame();

protected:
  virtual void Run_InputUpdate() override;
  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual ezUniquePtr<ezGameStateBase> CreateGameState() override;

private:
  ezResult TryProjectFolder(ezStringView sPath);
  void DetermineProjectPath();
};
