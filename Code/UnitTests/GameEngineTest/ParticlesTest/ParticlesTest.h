#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class ezGameEngineTestApplication_Particles : public ezGameEngineTestApplication
{
public:
  ezGameEngineTestApplication_Particles();

  void SetupSceneSubTest(const char* szFile);
  void SetupParticleSubTest(const char* szFile);
  ezTestAppRun ExecParticleSubTest(ezInt32 iCurFrame);

  /// Reloads the effect resource while the effect is running and checks that the effect switches over to the new content and keeps its particles.
  ezTestAppRun ExecResourceReloadSubTest(ezInt32 iCurFrame, const char* szEffect);

  /// Returns the number of particles of the effect on the object with the given name. Reports a test failure if there is no such object.
  ezUInt64 GetParticleCount(ezStringView sObjectName);

  /// Returns the center of the bounding volume of the effect on the object with the given name, in the local space of the effect.
  ezVec3 GetParticleBoundsCenter(ezStringView sObjectName);

  /// Checks the number of particles of the effect on the object with the given name.
  void CheckParticleCount(ezStringView sObjectName, ezUInt64 uiExpectedCount);

  ezUInt32 m_uiImageCompareThreshold = 110;

private:
  class ezParticleComponent* FindParticleComponent(ezStringView sObjectName);

  ezSharedPtr<const struct ezParticleEffectResourceDescriptor> m_pReloadedDescriptor;
  ezUInt64 m_uiParticlesBeforeReload = 0;
};

class ezGameEngineTestParticles : public ezGameEngineTest
{
  using SUPER = ezGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual ezGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    // tests that reference particle assets directly
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
    EffectRenderer,
    BoxPositionInitializer,
    SpherePositionInitializer,
    CylinderPositionInitializer,
    RandomColorInitializer,
    RandomSizeInitializer,
    RotationSpeedInitializer,
    VelocityConeInitializer,
    BurstEmitter,
    ContinuousEmitter,
    OnEventEmitter,
    QuadRotatingOrtho,
    QuadFixedEmDir,
    QuadAxisEmDir,
    SphereBounds,
    Turbulence,
    Expression,

    // tests that use a dedicated scene
    Billboards,
    PullAlongBehavior,
    DistanceEmitter,
    SharedInstances,
    EventReactionEffect,
    LocalSpaceSim,
    Lighting,
    Attractors,
    Fog,
    MeshPositionInitializer,
    WindBehavior,
    ParticleAreas,
    ResourceReload,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;
  ezUInt32 GetImageCompareThreshold(ezInt32 iIdentifier);

  ezInt32 m_iFrame = 0;
  ezGameEngineTestApplication_Particles* m_pOwnApplication = nullptr;
};
