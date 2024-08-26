#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include <GameEngineTest/TestClass/TestClass.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

class ezStereoTestGameState : public ezGameEngineTestGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStereoTestGameState, ezGameEngineTestGameState);

public:
  void OverrideRenderPipeline(ezTypedResourceHandle<ezRenderPipelineResource> hPipeline);
};

class ezStereoTestApplication : public ezGameEngineTestApplication
{
public:
  using SUPER = ezGameEngineTestApplication;

  ezStereoTestApplication(const char* szProjectDirName);
  ezPlatformProfile& GetPlatformProfile() { return m_PlatformProfile; }

protected:
  virtual ezUniquePtr<ezGameStateBase> CreateGameState() override;
};


class ezStereoTest : public ezGameEngineTest
{
  using SUPER = ezGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual ezGameEngineTestApplication* CreateApplication() override;

protected:
  enum SubTests
  {
    HoloLensPipeline,
    DefaultPipeline
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  ezInt32 m_iFrame = 0;
  ezStereoTestApplication* m_pOwnApplication = nullptr;

  ezUInt32 m_uiImgCompIdx = 0;
  ezHybridArray<ezUInt32, 8> m_ImgCompFrames;
};

#endif
