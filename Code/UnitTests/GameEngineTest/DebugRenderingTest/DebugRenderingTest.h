#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include <GameEngineTest/TestClass/TestClass.h>

class ezGameEngineTestApplication_DebugRendering : public ezGameEngineTestApplication
{
public:
  ezGameEngineTestApplication_DebugRendering();

  void SubTestSetup();
  ezTestAppRun SubTestBasicsExec(ezInt32 iCurFrame);
  ezTestAppRun SubTestTextExec(ezInt32 iCurFrame);
  ezTestAppRun SubTestThickLinesExec(ezInt32 iCurFrame);
  ezTestAppRun SubTestShapesExec(ezInt32 iCurFrame);
};

class ezGameEngineTestDebugRendering : public ezGameEngineTest
{
  using SUPER = ezGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual ezGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    Basics,
    Text,
    ThickLines,
    Shapes,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  ezInt32 m_iFrame = 0;
  ezGameEngineTestApplication_DebugRendering* m_pOwnApplication = nullptr;
};
