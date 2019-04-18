#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class ezPlayerApplication : public ezGameApplication
{
public:
  typedef ezGameApplication SUPER;

  ezPlayerApplication();


private:
  virtual ezResult BeforeCoreSystemsStartup() override;

  virtual void AfterCoreSystemsStartup() override;

  void SetupLevel();

  virtual void BeforeHighLevelSystemsShutdown() override;

  ezString m_sSceneFile;
  ezUniquePtr<ezWorld> m_pWorld;
};


