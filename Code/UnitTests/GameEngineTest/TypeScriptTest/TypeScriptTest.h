#pragma once

#include <GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class ezGameEngineTestApplication_TypeScript : public ezGameEngineTestApplication
{
public:
  ezGameEngineTestApplication_TypeScript();

  void SubTestBasicsSetup();
  ezTestAppRun SubTestBasisExec(ezInt32 iIdentifier);
};

class ezGameEngineTestTypeScript : public ezGameEngineTest
{
public:
  virtual const char* GetTestName() const override;
  virtual ezGameEngineTestApplication* CreateApplication() override;

  enum SubTests
  {
    Vec2,
    Vec3,
    Quat,
    Mat3,
    Mat4,
    Transform,
    Color,
    Debug,
    GameObject,
    Component,
  };

private:
  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  ezGameEngineTestApplication_TypeScript* m_pOwnApplication = nullptr;
};


