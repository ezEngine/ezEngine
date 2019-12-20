#pragma once

#include <GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class ezGameEngineTestApplication_TypeScript : public ezGameEngineTestApplication
{
public:
  ezGameEngineTestApplication_TypeScript();

  void SubTestBasicsSetup();
  ezTestAppRun SubTestBasisExec(ezInt32 iCurFrame);
};

class ezGameEngineTestTypeScript : public ezGameEngineTest
{
public:
  virtual const char* GetTestName() const override;
  virtual ezGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    Basics,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  ezInt32 m_iFrame = 0;
  ezGameEngineTestApplication_TypeScript* m_pOwnApplication = nullptr;
};


