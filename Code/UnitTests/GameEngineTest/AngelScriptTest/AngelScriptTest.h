#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class ezGameEngineTestApplication_AngelScript : public ezGameEngineTestApplication
{
public:
  ezGameEngineTestApplication_AngelScript();

  void SubTestBasicsSetup();
  ezTestAppRun SubTestBasisExec(const char* szSubTestName);
  /// \brief Creates a module out of the given code and calls the function `void ExecuteTests()` in it.
  void RunTestScript(ezStringView sScriptPath);
  void TestScriptExceptionCallback(asIScriptContext* pContext);

private:
  ezStringBuilder m_sCode;
  ezDynamicArray<ezStringView> m_Lines;
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
    Strings,
    Arrays,
    EntryPoints,
    World,
    Messaging,
    GameObject,
    Physics,
    Misc,
  };

private:
  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  ezGameEngineTestApplication_AngelScript* m_pOwnApplication = nullptr;
};
