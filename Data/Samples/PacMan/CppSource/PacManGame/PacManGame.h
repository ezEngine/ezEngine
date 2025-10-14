#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class ezPacManGame : public ezGameApplication
{
public:
  using SUPER = ezGameApplication;

  ezPacManGame();

protected:
  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual ezUniquePtr<ezGameStateBase> CreateGameState() override;

private:
  ezResult TryProjectFolder(ezStringView sPath);
  void DetermineProjectPath();
};
