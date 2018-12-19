#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class ezPlayerApplication : public ezGameApplication
{
public:
  ezPlayerApplication();


private:
  virtual void BeforeCoreStartup() override;

  virtual void AfterCoreStartup() override;

  void SetupLevel();

  virtual void BeforeCoreShutdown() override;

  ezString m_sSceneFile;
  ezUniquePtr<ezWorld> m_pWorld;
};


