#pragma once

#include <GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class ezGameEngineTestApplication_Particles : public ezGameEngineTestApplication
{
public:
  ezGameEngineTestApplication_Particles();

  void SetupSceneSubTest(const char* szFile);
  void SetupParticleSubTest(const char* szFile);
  ezTestAppRun ExecParticleSubTest(ezInt32 iCurFrame);
};

class ezGameEngineTestParticles : public ezGameEngineTest
{
public:
  virtual const char* GetTestName() const override;
  virtual ezGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    BillboardRenderer,
    ColorGradientBehavior,
    FliesBehavior,
    GravityBehavior,
    LightRenderer,
    MeshRenderer,
    RaycastBehavior,
    SizeCurveBehavior,
    TrailRenderer,
    VelocityBehavior,
    //EffectRenderer,
    BoxPositionInitializer,
    SpherePositionInitializer,
    CylinderPositionInitializer,
    RandomColorInitializer,
    RandomSizeInitializer,
    RotationSpeedInitializer,
    VelocityConeInitializer,

    Billboards,
    PullAlongBehavior,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  ezInt32 m_iFrame = 0;
  ezGameEngineTestApplication_Particles* m_pOwnApplication = nullptr;
};


