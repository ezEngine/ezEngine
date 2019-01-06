#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class ezPlayerApplication : public ezGameApplication
{
public:
  ezPlayerApplication();


private:
  virtual void BeforeCoreSystemsStartup() override;

  virtual void AfterCoreSystemsStartup() override;

  void SetupLevel();

  virtual void BeforeCoreSystemsShutdown() override;

  ezString m_sSceneFile;
  ezUniquePtr<ezWorld> m_pWorld;
};


