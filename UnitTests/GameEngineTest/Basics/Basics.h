#pragma once

#include <PCH.h>

#include "../TestClass/TestClass.h"

class ezGameEngineTestApplication_Basics : public ezGameEngineTestApplication
{
public:
  ezGameEngineTestApplication_Basics();

  void SubTestTransformAssetsSetup();
  ezTestAppRun SubTestTransformAssetsExec(ezInt32 iCurFrame);

  void SubTestManyMeshesSetup();
  ezTestAppRun SubTestManyMeshesExec(ezInt32 iCurFrame);

  void SubTestSkyboxSetup();
  ezTestAppRun SubTestSkyboxExec(ezInt32 iCurFrame);

  void SubTestDebugRenderingSetup();
  ezTestAppRun SubTestDebugRenderingExec(ezInt32 iCurFrame);

  void SubTestLoadSceneSetup();
  ezTestAppRun SubTestLoadSceneExec(ezInt32 iCurFrame);
};

class ezGameEngineTestBasics : public ezGameEngineTest
{
public:
  virtual const char* GetTestName() const override;
  virtual ezGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    ST_TransformAssets,
    ST_ManyMeshes,
    ST_Skybox,
    ST_DebugRendering,
    ST_LoadScene,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier) override;

  ezInt32 m_iFrame;
  ezGameEngineTestApplication_Basics* m_pOwnApplication;
};
