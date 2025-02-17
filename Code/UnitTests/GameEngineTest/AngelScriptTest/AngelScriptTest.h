#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class ezGameEngineTestApplication_AngelScript : public ezGameEngineTestApplication
{
public:
  ezGameEngineTestApplication_AngelScript();

  void SubTestBasicsSetup();
  ezTestAppRun SubTestBasisExec(const char* szSubTestName);
};

class ezGameEngineTestAngelScript : public ezGameEngineTest
{
  using SUPER = ezGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual ezGameEngineTestApplication* CreateApplication() override;

  enum SubTests
  {
    Types,
    EntryPoints,
    World,
    Strings,
    Messaging,
    GameObject,
  };

private:
  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  ezGameEngineTestApplication_AngelScript* m_pOwnApplication = nullptr;
};
