#pragma once

#include <TestFramework/Framework/TestBaseClass.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <Texture/Image/Image.h>

class ezGameEngineTestGameState : public ezFallbackGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameEngineTestGameState, ezFallbackGameState);

public:
  virtual void ProcessInput() override;
  virtual ezGameStatePriority DeterminePriority(ezWorld* pWorld) const override;
  virtual void ConfigureInputActions() override;
};

class ezGameEngineTestApplication : public ezGameApplication
{
public:
  typedef ezGameApplication SUPER;

  ezGameEngineTestApplication(const char* szProjectDirName);

  virtual ezString FindProjectDirectory() const final override;
  const ezImage& GetLastScreenshot() { return m_LastScreenshot; }

  ezResult LoadScene(const char* szSceneFile);

protected:
  virtual void BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeHighLevelSystemsShutdown() override;
  virtual void StoreScreenshot(ezImage&& image, const char* szContext) override;
  virtual void Init_FileSystem_ConfigureDataDirs() override;
  virtual ezUniquePtr<ezGameStateBase> CreateGameState(ezWorld* pWorld) override;

  ezString m_sProjectDirName;
  ezUniquePtr<ezWorld> m_pWorld;
  ezImage m_LastScreenshot;
};

class ezGameEngineTest : public ezTestBaseClass
{
public:
  ezGameEngineTest();
  ~ezGameEngineTest();

  virtual ezResult GetImage(ezImage& img) override;
  virtual ezGameEngineTestApplication* CreateApplication() = 0;

protected:
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;

  ezGameEngineTestApplication* m_pApplication = nullptr;
};


