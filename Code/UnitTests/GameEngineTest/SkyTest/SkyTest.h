#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

/// Tests for the sky components (ezSkyBoxComponent, ezAtmosphericScatteringComponent).
///
/// The worlds are set up in code, only assets from the Base data directory and the shared test data are used.
class ezGameEngineTestSky : public ezGameEngineTest
{
  using SUPER = ezGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual ezGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    Skybox,
    SkyboxFog,
    AtmosphericScattering,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  void SetupSkybox();
  ezTestAppRun RunSkybox();

  void SetupSkyboxFog();
  ezTestAppRun RunSkyboxFog();

  void SetupAtmosphericScattering();
  ezTestAppRun RunAtmosphericScattering();

  ezCamera* GetCamera() const;

  ezInt32 m_iFrame = 0;
  ezGameEngineTestApplication* m_pOwnApplication = nullptr;
  ezGameObjectHandle m_hSunObject;
};
