#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>
#include <TestFramework/Utilities/TestLogInterface.h>

#include "../TestClass/TestClass.h"

class ezGameEngineTestVisualScript : public ezGameEngineTest
{
  using SUPER = ezGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual ezGameEngineTestApplication* CreateApplication() override;

protected:
  enum SubTests
  {
    Variables,
    Coroutines,
    Messages,
    EnumsAndSwitch,
    Blackboard,
    Loops,
    Loops2,
    Loops3,
    Properties,
    Arrays,
    Maps,
    Expressions,
    Physics,
    Misc,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  ezInt32 m_iFrame = 0;
  ezGameEngineTestApplication* m_pOwnApplication = nullptr;

  ezUInt32 m_uiImgCompIdx = 0;
  ezHybridArray<ezUInt32, 8> m_ImgCompFrames;

  struct TestLog
  {
    ezTestLogInterface m_Interface;
    ezTestLogSystemScope m_Scope;

    TestLog()
      : m_Scope(&m_Interface, true)
    {
    }
  };

  ezUniquePtr<TestLog> m_pTestLog;
};
