#pragma once

#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <TestFramework/Framework/TestBaseClass.h>
#include <Texture/Image/Image.h>

class ezGameEngineTestGameState : public ezGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameEngineTestGameState, ezGameState);

public:
  virtual void ProcessInput() override;
  virtual void ConfigureInputActions() override;
};

class ezGameEngineTestApplication : public ezGameApplication
{
public:
  using SUPER = ezGameApplication;

  ezGameEngineTestApplication(const char* szProjectDirName);

  virtual ezString FindProjectDirectory() const final override;
  virtual ezString GetProjectDataDirectoryPath() const final override;
  const ezImage& GetLastScreenshot() { return m_LastScreenshot; }

  ezResult LoadScene(const char* szSceneFile);
  ezWorld* GetWorld() const { return m_pWorld.Borrow(); }

protected:
  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeHighLevelSystemsShutdown() override;
  virtual void StoreScreenshot(ezImage&& image, ezStringView sContext) override;
  virtual void Init_FileSystem_ConfigureDataDirs() override;
  virtual ezUniquePtr<ezGameStateBase> CreateGameState() override;

  ezString m_sProjectDirName;
  ezUniquePtr<ezWorld> m_pWorld;
  ezImage m_LastScreenshot;
};

class ezGameEngineTest : public ezTestBaseClass
{
  using SUPER = ezTestBaseClass;

public:
  ezGameEngineTest();
  ~ezGameEngineTest();

  virtual ezResult GetImage(ezImage& ref_img, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber) override;
  virtual ezGameEngineTestApplication* CreateApplication() = 0;

protected:
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;

  ezGameEngineTestApplication* m_pApplication = nullptr;
};
