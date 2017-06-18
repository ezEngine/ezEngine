#pragma once

#include <TestFramework/Framework/TestBaseClass.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <Foundation/Image/Image.h>

class ezGameEngineTestGameState : public ezFallbackGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameEngineTestGameState, ezFallbackGameState);

public:
  virtual void ProcessInput() override;
  virtual float CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const override;
  virtual void ConfigureInputActions() override;
};

class ezGameEngineTestApplication : public ezGameApplication
{
public:
  ezGameEngineTestApplication(const char* szProjectDirName);

  virtual ezString FindProjectDirectory() const final override;
  const ezImage& GetLastScreenshot() { return m_LastScreenshot; }

protected:
  virtual void BeforeCoreStartup() override;
  virtual void AfterCoreStartup() override;
  virtual void BeforeCoreShutdown() override;
  virtual void SetupWorld() = 0;
  virtual void DoSaveScreenshot(ezImage& image) override;
  virtual void DoSetupDataDirectories() override;
  virtual ezGameState* CreateCustomGameStateForWorld(ezWorld* pWorld) override;

  ezString m_sProjectDirName;
  ezWorld* m_pWorld = nullptr;
  ezImage m_LastScreenshot;
};

class ezGameEngineTest : public ezTestBaseClass
{
public:
  ezGameEngineTest();

  virtual ezResult GetImage(ezImage& img) override;
  virtual ezGameEngineTestApplication* CreateApplication() = 0;

protected:
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;

  ezGameEngineTestApplication* m_pApplication = nullptr;
};


